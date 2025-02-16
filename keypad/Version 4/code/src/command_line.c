/**
 * @file casella_command_line.c
 * @brief Handles commandline input from serial debug port
 * @details
 *   This module called from the main command/control task:
 *   - Processes the content of the debug port messages
 *
 * @author Barry Bridges
 * @version V1.0
 * @date 03 Dec 2024
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <errno.h>

/* Project includes */
#include "py32f0xx.h"
#include "globals.h"

/* Module includes */
#include "lfs_interface.h"
#include "rtc.h"
#include "lfs.h"
#include "command_line.h"
#include "rtc.h"
#include "uart.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/**
 * @fn int16_t get_char_from_serial(void)
 * @brief get character from serial port.
 *
 * @return character received or -1 if none.
 */
#define CHECK_ACCESS_LEVEL(required_access_level) \
	if (access_level < required_access_level) return false;

/**
 * @brief Console access level.
 *
 */
typedef enum {
	E_ACCESS_LEVEL_NONE,
	E_ACCESS_LEVEL_USER,
	E_ACCESS_LEVEL_ADMIN,
	E_ACCESS_LEVEL_DISABLED,
} access_level_t;

/**
 * @brief Commands for parsing the command.
 *
 */
typedef struct {
    char command[CMD_LINE_MAX_ARG_LEN];
    uint8_t num_param;
    bool (*func)();
    char help_text[CMD_LINE_MAX_ARG_HELP_LEN];
} Commands_t;

/*******************************************************************************
 * Local variables
 ******************************************************************************/

/* Buffers */
static CMD_LINE_PARAM cmd_line_args[CMD_LINE_MAX_ARGS];
static char cmd_line_input[MAX_CMD_LINE_LENGTH];
static char output_buffer[CMD_OUTPUT_BUFFER_LEN];

/* Module level scope variables for the positions and indexing of the command line */
static uint8_t arg       = 0;
static uint8_t arg_pos   = 0;
static uint8_t line_pos  = 0;

/**
 * @brief Console access level.
 *
 */
access_level_t access_level = E_ACCESS_LEVEL_ADMIN; /* Always admin at the moment */

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/**
 * @brief Function prototypes..
 *
 */
static void do_system_reset(void);
static int16_t get_char_from_serial(void);
static char * command_line_strtolower(char * string);
void debug_send_usb(uint8_t* data, uint32_t len);
void uart_send_string(char* data);
//static bool dump_file(char* filename);
//static bool command_dump(void);
static bool command_delete(void);
//static bool command_erase(void);
static bool command_format(void);
static bool command_reboot(void);
static bool command_help(void);
static bool command_clear(void);
//static bool command_login(void);
static bool command_dir(void);
static bool command_date(void);
static bool command_time(void);

/**
 * @brief Commands, max number of parms, the handler function and help text.
 *
 */
static const Commands_t command_table[] = {
/* 	Command      	max parameters	handler function	Help text */
    {"help",     	0, 				command_help,     	"Show this help text"},
    {"clear",    	0, 				command_clear,   	"Clear screen"},
//    {"dump",     	3, 				command_dump,     	"\033[35m<filename>\x1b[0m Dump the content of a file"},
    {"reboot",   	1, 				command_reboot,   	"Reboot the device"},
    {"dir",     	0, 				command_dir,     	"List all files stored in flash"},
    {"del",   	    1, 				command_delete,   	"\033[35m<filename>\x1b[0m Delete a file"},
//    {"erase",    	0, 				command_erase,    	"\033[35m<filename>\x1b[0m Erase survey data"},
    {"format",    	0, 				command_format,    	"Re-formats the flash filing system"},
    {"date",     	1, 				command_date,     	"\033[35m<YY-MM-DD>\x1b[0m Set date"},
    {"time",     	1, 				command_time,     	"\033[35m<hh:mm:ss>\x1b[0m Set time"},
    {"",         	0,				NULL,             	""},
};


/* Private function prototypes */
/**
 * @fn static void debug_console(void)
 * @brief Debug console task
 *
 * Task for the debug console
 */
void debug_console(void)
{
    E_CMD_RESULTS result;

    command_line_reset();

    while(true)
    {
        result = command_line_get_line();
        if (result == E_CMD_EOL)
        {
            /* Get the parameters from the complete command line */
            result = command_line_get_parameters();
            if (result != E_CMD_NONE)
            {
                command_line_report(result);
            }

            /* Process the command and parameters */
            command_line_parse();

            /* Both the same but may separate in future */
            if (result != E_CMD_NONE)
            {
                command_line_report(result);
            }

            /* Reset ready for the next command */
            command_line_reset();
            //sprintf(buffer, "\n\r>");
            //debug_send_usb(buffer, strlen(buffer));
        }
    }
}


/**
 * @fn static int16_t get_char_from_serial(void)
 * @brief get character from serial port.
 *
 * @return character received or -1 if none.
 */
static int16_t get_char_from_serial(void)
{
	uint8_t rx_byte;
    HAL_StatusTypeDef status;


    status = HAL_UART_Receive(&UartHandle, &rx_byte, 1, 1);
    if (status == HAL_OK)
    {
        return rx_byte;
    }

	return CMD_LINE_NONE;
}

/**
 * @fn bool action_confirmed(void)
 * @brief Get (Y/N) confirmation from user
 *
 * @return true if confirmed, otherwise false.
 */
bool action_confirmed(void)
{
	uint8_t rx_byte = CMD_LINE_NONE;
	sprintf(output_buffer, "\n\rAre you sure (Y/N)?\n\r");
	uart_send_string(output_buffer);

	/* Wait for a response */
	while (rx_byte != 'y' && rx_byte != 'Y' && rx_byte != 'N' && rx_byte != 'n')
	{
		rx_byte = get_char_from_serial();
	}

	if (rx_byte == 'Y' || rx_byte == 'y')
	{
		return true;
	}
	return false;
}

/**
 * @fn static bool help(void)
 * @brief Print out help text
 *
 * @return always true
 */
static bool help(void)
{
	uint8_t command = 0;

	sprintf(output_buffer, "\033[2J\n\rAvailable commands are:");
	uart_send_string(output_buffer);

	sprintf(output_buffer, "\n\r============================================\n\r");
	uart_send_string(output_buffer);

	while(command_table[command].func != NULL)
	{
		sprintf(output_buffer, "\n\r\033[31m[%02d] \033[36m %s \x1b[0m %s", command, command_table[command].command, command_table[command].help_text);
		uart_send_string(output_buffer);
		command++;
	}

	sprintf(output_buffer, "\n\r\n\r============================================\n\r");
	uart_send_string(output_buffer);
	return true;
}

/**
 * @fn void command_line_report(E_CMD_RESULTS report)
 * @brief Report state
 *
 * @return None
 */
void command_line_report(E_CMD_RESULTS report)
{
	output_buffer[0] = 0;

    switch(report)
    {
    case E_CMD_LINE_TOO_LONG:
        sprintf(output_buffer, "\n\rLine too long\n\r");
        break;
    case E_CMD_ARG_TOO_LONG:
        sprintf(output_buffer, "\n\rArgument too long\n\r");
        break;
    case E_CMD_TOO_MANY_ARGS:
        sprintf(output_buffer, "\n\rToo many arguments\n\r");
        break;
    case E_CMD_QUTOES:
        sprintf(output_buffer, "\n\rUnbalanced quotations\n\r");
        break;
    case E_CMD_INVALID_OPTION:
        sprintf(output_buffer, "\n\rInvalid option(s)\n\r");
        break;
    case E_CMD_OK:
        sprintf(output_buffer, "\n\rOK\n\r");
        break;
    case E_CMD_ERROR:
    	sprintf(output_buffer, "\n\rERROR\n\r");
        break;
    case E_CMD_FILE_NOT_FOUND:
    	sprintf(output_buffer, "\n\rFile not found\n\r");
    	break;
    case E_CMD_INVALID_FORMAT:
    	sprintf(output_buffer, "\n\rInvalid format\n\r");
    	break;
    case E_CMD_NONE:
    	sprintf(output_buffer, "\n\rOK\n\r");
    case E_CMD_EOL:
    case E_CMD_BACKSPACE:
    default:
        break;
    }

    /* If there is a responce then pront the string */
    if (strlen(output_buffer))
    {
    	uart_send_string(output_buffer);
    }
}

/**
 * @fn void command_line_reset(void)
 * @brief Resets all the command line variables between processing
 *
 * @return None
 */
void command_line_reset(void)
{
    /* These variables have to be reset to 0 after processing each line */
    memset(&cmd_line_args, 0, sizeof(cmd_line_args));
    memset(&cmd_line_input, 0, sizeof(cmd_line_input));
    arg = 0;
    arg_pos = 0;
    line_pos = 0;
}

/**
 * @fn bool command_line_parameter_convert(CMD_LINE_PARAM *parameter, E_PARAM_TYPE p_type)
 * @brief Converts the string parameter to a type of choise
 *
 * @return True if converstion successful
 */
bool command_line_parameter_convert(CMD_LINE_PARAM *parameter, E_PARAM_TYPE p_type)
{
    CMD_LINE_PARAM result;
    char *end;

    switch(p_type)
    {
    /* Already a string so don't convert */
    case E_PARAM_STRING:
        return true;

    /* Convert a string to a float with error checking */
    case E_PARAM_FLOAT:
        result.arg_float = strtof(parameter->arg_str, &end);
        if (errno == ERANGE)
        {
            errno = 0;
            parameter->arg_float = 0;
            return false;
        }
        parameter->arg_float = result.arg_float;
        return true;

    /* Convert a string to INT32 */
    case E_PARAM_INT32:
        result.arg_int32 = (int32_t)strtol(parameter->arg_str, &end, BASE_10);
        if (errno == ERANGE)
        {
            errno = 0;
            parameter->arg_int32 = 0;
            return false;
        }
        parameter->arg_int32 = result.arg_int32;
        return true;

    /* Convert to a UINT32 */
    case E_PARAM_UINT32:
        result.arg_uint32 = (uint32_t)strtol(parameter->arg_str, &end, BASE_10);
        if (errno == ERANGE)
        {
            errno = 0;
            parameter->arg_uint32 = 0;
            return false;
        }
        parameter->arg_uint32 = result.arg_uint32;
        return true;

    /* Convert to an INT16 */
    case E_PARAM_INT16:
        result.arg_int32 = (int32_t)strtol(parameter->arg_str, &end, BASE_10);
        if (errno == ERANGE || result.arg_int32 > MAX_INT16 || result.arg_int32 < MIN_INT16)
        {
            errno = 0;
            parameter->arg_int16 = 0;
            return false;
        }
        parameter->arg_int16 = (int16_t)result.arg_int32;
        return true;

    /* Convert to UINT16 */
    case E_PARAM_UINT16:
        result.arg_uint32 = (uint32_t)strtol(parameter->arg_str, &end, BASE_10);
        if (errno == ERANGE || result.arg_uint32 > 65535)
        {
            errno = 0;
            parameter->arg_uint16 = 0;
            return false;
        }
        parameter->arg_uint16 = (uint16_t)result.arg_uint32;
        return true;

    /* Convert to INT8 */
    case E_PARAM_INT8:
        result.arg_int32 = (int32_t)strtol(parameter->arg_str, &end, BASE_10);
        if (errno == ERANGE || result.arg_int32 > MAX_INT8 || result.arg_int32 < MIN_INT8)
        {
            errno = 0;
            parameter->arg_uint8 = 0;
            return false;
        }
        parameter->arg_int8 = (int8_t)result.arg_int32;
        return true;

    /* Convert to UINT8 */
    case E_PARAM_UINT8:
        result.arg_uint32 = (uint32_t)strtol(parameter->arg_str, &end, BASE_10);
        if (errno == ERANGE || result.arg_uint32 > MAX_UINT8)
        {
            errno = 0;
            parameter->arg_uint8 = 0;
            return false;
        }
        parameter->arg_uint8 = (uint8_t)result.arg_uint32;
        return true;

    /* Something wrong so go */
    default:
        return false;
    }
    return false;
}

/**
 * @fn E_CMD_RESULTS command_line_parse(void)
 * @brief
 *
 * @return E_CMD_RESULTS CMD_LINE_NONE command processed ok.
 */
E_CMD_RESULTS command_line_parse(void)
{
    uint32_t i = 0;
    bool success = false;

    /* Scan for the command */
    do
    {
        if (strcmp(cmd_line_args[0].arg_str, command_table[i].command) == 0)
        {
            /* Check on the number of parameters */
            if (command_table[i].num_param < arg)
            {
                command_line_report(E_CMD_TOO_MANY_ARGS);
            }
            else if (command_table[i].num_param < arg)
            {
                command_line_report(E_CMD_NOT_ENOUGH_ARGS);
            }

            /* Call the required function */
            success = (*command_table[i].func)();
            if (success)
            {
                //command_line_report(E_CMD_OK);
            }
            else
            {
                command_line_report(E_CMD_ERROR);
            }
            break;
        }
    }
    while (command_table[++i].func != NULL);

    return E_CMD_NONE;
}

/**
 * @fn E_CMD_RESULTS command_line_get_line(void)
 * @brief Gets and cleans the parameters.
 *
 * @return E_CMD_RESULTS - E_CMD_NONE is successful
 */
E_CMD_RESULTS command_line_get_parameters(void)
{
	CHECK_ACCESS_LEVEL(E_ACCESS_LEVEL_ADMIN);
    bool inquotes = false;

    line_pos = 0;

    /* Loop until null */
    while(cmd_line_input[line_pos])
    {
        /* Argument too long? */
        if (arg_pos > CMD_LINE_MAX_ARG_LEN - 1)
        {
            return E_CMD_ARG_TOO_LONG;
        }

        /* Handle quotes */
        if (cmd_line_input[line_pos] == QUOTE )
        {
            /* If in quotes the skip it and flag it */
            inquotes = !inquotes;
            line_pos ++;
            continue;
        }

        /* Handle spaces differently if in quotes */
        if (!inquotes)
        {
            /* If not in quotes then space is used as a parameter delimiter */
            if (cmd_line_input[line_pos] == SPACE)
            {
                if (++arg > (CMD_LINE_MAX_ARGS - 1))
                {
                    arg = 0;
                    return E_CMD_TOO_MANY_ARGS;
                }
                arg_pos = 0;
                line_pos++;
            }
            else
            {
                cmd_line_args[arg].arg_str[arg_pos++] = cmd_line_input[line_pos++];
            }
        }
        else
        {
            cmd_line_args[arg].arg_str[arg_pos++] = cmd_line_input[line_pos++];
        }
    }

    /* Convert the command (first parameter) to lower case */
    char * command = cmd_line_args[0].arg_str;
    while(*command)
    {
        int test = *command;
        *command = (char)tolower(test);
        command++;
    }
    return E_CMD_NONE;
}

/**
 * @fn static char * command_line_strtolower(char * string)
 * @brief Converts a string to lowercase (in place)
 *
 * @return Pointer to the converted string.
 */
static char * command_line_strtolower(char * string)
{
	CHECK_ACCESS_LEVEL(E_ACCESS_LEVEL_ADMIN);
    /* For convenience we return the string pointer */
    char * string_ptr = string;

    /* Convert the command (first parameter) to lower case */
    while(*string)
    {
        *string = (char)tolower((int)*string);
        string++;
    }

    return string_ptr;
}

/**
 * @fn E_CMD_RESULTS command_line_get_line(void)
 * @brief Gets and cleans the command line.
 *
 * @return E_CMD_RESULTS - E_CMD_NONE is successful
 */
E_CMD_RESULTS command_line_get_line(void)
{
	CHECK_ACCESS_LEVEL(E_ACCESS_LEVEL_ADMIN);
    int16_t new_char;
    static bool last_char_space = false; /* Last char was a space */
    static bool inside_quotes   = false; /* Inside quotations */

    /* Is there a character to process */
    if ((new_char = get_char_from_serial()) != CMD_LINE_NONE)
    {
        /* End of line */
        if (new_char == CRCHAR)
        {
            /* Remove trailing space */
            if (line_pos > 0)
            {
                if (isspace((int)cmd_line_input[line_pos-1]))
                {
                    line_pos--;
                }
            }

            /* Reset and go */
            cmd_line_input[line_pos] = 0;
            last_char_space = false;

            /* End with unbalanced quotes? */
            if (inside_quotes)
            {
                inside_quotes = false;
                return E_CMD_QUTOES;
            }

            inside_quotes = false;
            return E_CMD_EOL;
        }

        /* Handle back spaces */
        if (new_char == BACKSPACE)
        {
            if (line_pos > 0)
            {
               line_pos--;
               /* Let the calling function know it was
                * a backspace so it can back space on the terminal.
                */
               return E_CMD_BACKSPACE;
            }
            return E_CMD_NONE;
        }

        /* Check the input lengths, don't process any more but allow for the CR to be entered */
        if (line_pos >= MAX_CMD_LINE_LENGTH - 2) /* -2 to allow for a terminator */
        {
            return E_CMD_LINE_TOO_LONG;
        }

        /* Flag when inside quotes so as to allow for more spaces etc. */
        if (new_char == QUOTE)
        {
            /* Flag quotes */
            inside_quotes = !inside_quotes;
            cmd_line_input[line_pos++] = (char)new_char;

            /* Keep quotes within string */
            return E_CMD_NONE;
        }

        /* skip if more than one space */
        if (isspace(new_char))
        {
            if (!inside_quotes)
            {
                if (last_char_space == false && line_pos != 0)
                {
                    /* Flag it for next time */
                    last_char_space = true;
                    cmd_line_input[line_pos++] = SPACE;
                }
            }
            else
            {
                cmd_line_input[line_pos++] = SPACE;
            }

            return E_CMD_NONE;
        }

        /* Sanitise the incoming data now we have removed backspaces and CR */
        if (isdigit(new_char) == 0 && isalpha(new_char) == 0 && new_char != '.' && new_char != '-' && new_char != ':')
        {
            /* Just skip over */
        }
        else
        {
            /* Store a standard char */
            last_char_space = false;
            cmd_line_input[line_pos++] = (char)new_char;
        }
    }

    return E_CMD_NONE;
}

/**
 * @fn static bool command_dump(void)
 * @brief Process the 'dump' command. Valid options are 'logs' or 'settings'
 *
 */
// static bool command_dump(void)
// {
// 	// CHECK_ACCESS_LEVEL(E_ACCESS_LEVEL_ADMIN);
// 	// char file_extension[4] = {0};
// 	// char *ptr;

//     // command_line_strtolower(cmd_line_args[1].arg_str);

//     // /* Start looking for the extension on the file name */
//     // ptr = strchr(cmd_line_args[1].arg_str, '.');
//     // if (ptr == NULL)
//     // {
//     // 	return false;
//     // }

//     // /* Get the extension */
//     // strcpy(file_extension, (const char*)++ptr);

//     // if (strcmp(file_extension,"db") == 0)
//     // {
//     // 	if (dump_kvf(cmd_line_args[1].arg_str))
//     // 	{

//     // 	}
// 	// 	else
// 	// 	{
// 	//         command_line_report(E_CMD_FILE_NOT_FOUND);
// 	//         return false;
// 	// 	}
//     // }
//     // else if (strcmp(file_extension,"settings") == 0)
//     // {
//     //     // settings_dump();
//     // }
//     // else if (strcmp(file_extension,"profile") == 0)
//     // {
//     //     // profile_dump();
//     // }
//     // else if (strcmp(file_extension,"run") == 0)
//     // {
//     //     // run_dump();
//     // }
//     // else if (strcmp(file_extension,"file") == 0)
//     // {
//     // 	/* Dump the contents of a file, assuming its text based */
//     // 	// uint8_t len = strlen(cmd_line_args[2].arg_str);
//     // 	// if ( len > 0 && len < FAT_FILENAME_SIZE )
//     // 	// {
//     // 		if(dump_file(cmd_line_args[2].arg_str))
//     // 		{

//     // 		}
//     // 		else
//     // 		{
//     // 	        command_line_report(E_CMD_FILE_NOT_FOUND);
//     // 	        return false;
//     // 		}
//     // 	// }
//     // }
//     // else
//     // {
//     //     command_line_report(E_CMD_INVALID_OPTION);
//     //     return false;
//     // }

//     // command_line_report(E_CMD_OK);
//     return true;
// }





/**
 * @fn static bool command_delete(void)
 * @brief Process the 'delete' command to delete an available measurment run.
 *        No parameters
 */
static bool command_delete(void)
{
//	fat_status_t result;
//	CHECK_ACCESS_LEVEL(E_ACCESS_LEVEL_ADMIN);
//	if (action_confirmed())
//	{
//		/* Do the deletion */
//		result = fat_fdelete(command_line_strtolower(cmd_line_args[1].arg_str));
//		if (E_FAT_STATUS_OK == result)
//		{
//			return true;
//		}
//		else
//		{
//			return false;
//		}
//	}
    return true;
}

/**
 * @fn static bool command_reboot(void)
 * @brief Process the 'reboot' command to reboot the device.
 *        No parameters
 */
static bool command_reboot(void)
{
	CHECK_ACCESS_LEVEL(E_ACCESS_LEVEL_ADMIN);
	if (action_confirmed())
	{
		do_system_reset();
	}

    return true;
}



/**
 * @fn static bool command_help(void)
 * @brief Process the 'help' command to display the help text.
 *        No parameters
 */
static bool command_help(void)
{
	CHECK_ACCESS_LEVEL(E_ACCESS_LEVEL_ADMIN);
	return help();
}



/**
 * @fn static bool command_help(void)
 * @brief Process the 'help' command to display the help text.
 *        No parameters
 */
static bool command_clear(void)
{
	CHECK_ACCESS_LEVEL(E_ACCESS_LEVEL_ADMIN);
	sprintf(output_buffer, "\033[2J");
	uart_send_string(output_buffer);
	return true;
}



/**
 * @fn static bool command_login(void)
 * @brief Process the 'login' command to start a self test.
 *        No parameters
 */
// static bool command_login(void)
// {
// 	    return true;
// }

/**
 * @fn static bool command_dir(void)
 * @brief List all files stored in flash
 *        No parameters
 */
static bool command_dir(void)
{
	CHECK_ACCESS_LEVEL(E_ACCESS_LEVEL_ADMIN);

	int lfs_error;
	lfs_dir_t lfs_dir;
	const char *path = "/";
	struct lfs_info info;

	lfs_error = lfs_dir_open(&lfs, &lfs_dir, path);
	if (lfs_error < 0)
	{
		uart_send_string("\n\rDisk error reading directory content\n\r");
	}

    // Read directory entries
    while (true)
    {
        int res = lfs_dir_read(&lfs, &lfs_dir, &info);
        if (res < 0) {
            sprintf(output_buffer, "Error reading directory: %d\n", res);
            uart_send_string(output_buffer);
            break;
        }

        // Check if we've reached the end
        if (res == 0) {
            break;
        }

        // Skip . and .. entries
        // if (strcmp(info.name, ".") == 0 || strcmp(info.name, "..") == 0)
        // {
        //     continue;
        // }

        // Print file info
        sprintf(output_buffer, "%s", info.name);
        uart_send_string(output_buffer);
        if (info.type == LFS_TYPE_REG)
        {
            sprintf(output_buffer, " Size: %ld bytes", info.size);
            uart_send_string(output_buffer);
        }
        else if (info.type == LFS_TYPE_DIR)
        {
        	uart_send_string(" \t\t[DIR]");
        }
        uart_send_string("\n\r");
    }
	lfs_error = lfs_dir_close(&lfs, &lfs_dir);
    return true;
}

/**
 * @fn uint8_t decimal_to_bcd(uint8_t decimal)
 * @brief Converts decimal to BCD
 */
uint8_t decimal_to_bcd(uint8_t decimal)
{
    uint8_t tens = decimal / 10;
    uint8_t ones = decimal % 10;
    return (tens << 4) | ones;
}

/**
 * @fn static bool command_date(void)
 * @brief Sets the date from the command line.
 *        No parameters
 */
static bool command_date(void)
{
	CHECK_ACCESS_LEVEL(E_ACCESS_LEVEL_ADMIN);

	uint8_t day;
	uint8_t month;
	uint8_t year;
	char date_time_str[32];

	/* If no parameters then show the current date */
	if (strlen(command_line_strtolower(cmd_line_args[1].arg_str)) == 0)
	{
		/* Report the current time */
		uint64_t sec = rtc_get_seconds();
		rtc_sec_to_date_time_str(sec, date_time_str);

		/* Only interested in the date */
		sprintf(output_buffer, "\n\rCurrent date %s\n\r", &date_time_str[9]);
		uart_send_string(output_buffer);
		return true;
	}

	/* Check the format */
    if (cmd_line_args[1].arg_str[2] != '-' || cmd_line_args[1].arg_str[5] != '-')
    {
    	command_line_report(E_CMD_INVALID_FORMAT);
    	return false;
    }

    /* Quick and dirty parse of date value */
    cmd_line_args[1].arg_str[2] = 0;
    cmd_line_args[1].arg_str[5] = 0;

    year  = atoi((const char *)&cmd_line_args[1].arg_str[0]);
    month = atoi((const char *)&cmd_line_args[1].arg_str[3]);
    day   = atoi((const char *)&cmd_line_args[1].arg_str[6]);

    /* Range check the input */
    if ((year > 99) || (month > 12) || (day > 31) || (month == 0) || (day == 0) || (year < 25))
    {
    	command_line_report(E_CMD_INVALID_FORMAT);
    	return false;
    }

    /* Convert them both to BCD for the RTC */
    year  = decimal_to_bcd(year);
    month = decimal_to_bcd(month);
    day   = decimal_to_bcd(day);

    /* Update the RTC */
	sprintf(output_buffer, "\n\rUpdating current date to %02X-%02X-%02X\n\r", year, month, day);
	uart_send_string(output_buffer);
	if (action_confirmed() == false)
	{
		uart_send_string("\n\rDate not changed\n\r");
		return true;
	}

    /* Set the date time registers */
	bool success = rtc_update_date(year, month, day);
	if (!success)
	{
		uart_send_string("\n\rFailed to update date\n\r");
		return false;
	}

	return true;
}

/**
 * @fn static bool command_time(void)
 * @brief Sets the time from the command line.
 *        No parameters
 */
static bool command_time(void)
{
	// CHECK_ACCESS_LEVEL(E_ACCESS_LEVEL_ADMIN);
	// uint8_t hours;
	// uint8_t mins;
	// uint8_t secs;
	// char date_time_str[32];

	// /* If no parameters then show the current time */
	// if (strlen(command_line_strtolower(cmd_line_args[1].arg_str)) == 0)
	// {
	// 	/* Report the current time */
	// 	uint64_t sec = rtc_get_seconds();
	// 	rtc_sec_to_date_time_str(sec, date_time_str);

	// 	/* Only interested in the time */
	// 	date_time_str[9] = 0;
	// 	sprintf(output_buffer, "\n\rCurrent time %s\n\r", date_time_str);
	// 	uart_send_string(output_buffer);
	// 	return true;
	// }

	// /* Check the format */
    // if (cmd_line_args[1].arg_str[2] != ':' || cmd_line_args[1].arg_str[5] != ':')
    // {
    // 	command_line_report(E_CMD_INVALID_FORMAT);
    // 	return false;
    // }

    // /* Quick and dirty parse of time value */
    // cmd_line_args[1].arg_str[2] = 0;
    // cmd_line_args[1].arg_str[5] = 0;
    // hours = atoi((const char *)&cmd_line_args[1].arg_str[0]);
    // mins  = atoi((const char *)&cmd_line_args[1].arg_str[3]);
    // secs  = atoi((const char *)&cmd_line_args[1].arg_str[6]);

    // /* Range check the input */
    // if (hours > 23 || mins >59 || secs > 59)
    // {
    // 	command_line_report(E_CMD_INVALID_FORMAT);
    // 	return false;
    // }

    // /* Convert them both to BCD for the RTC */
    // hours = decimal_to_bcd(hours);
    // mins  = decimal_to_bcd(mins);
    // secs  = decimal_to_bcd(secs);

    // /* Update the RTC */
	// sprintf(output_buffer, "\n\rUpdating current time to %02X:%02X:%02X\n\r", hours, mins, secs);
	// uart_send_string(output_buffer);
	// if (action_confirmed() == false)
	// {
	// 	uart_send_string("\n\rTime not changed\n\r");
	// 	return true;
	// }

    // /* Set the date time registers */
	// //bool success = rtc_update_time(hours, mins, secs);
	// if (!success)
	// {
	// 	uart_send_string("\n\rFailed to update time\n\r");
	// 	return false;
	// }

	return true;
}

/**
 * @fn static bool dump_file(char* filename)
 * @brief Attempts to dump the content of a file to the console.
 *
 */
// static bool dump_file(char* filename)
// {
// //	uint8_t data;
// 	//fat_handle_t *handle;

// //	uart_send_string("\n\r");
// //	/* Open file for reading */
// //	if ((handle = fat_fopen(E_FAT_MODE_READ, filename)) != NULL)
// //	{
// //		while(fat_fread(handle, &data, 1))
// //		{
// //			debug_send_usb(&data, 1);
// //		}
// //		return true;
// //	}
// 	return false;
// }

/**
 * @fn static bool command_format(void)
 * @brief   Format the filing system.
 * @details Stops runs, flushes logs and settings, unmounts the filing
 *          and formats. Then resboots.
 */
static bool command_format(void)
{
	// int err;
	// /* Need so protection here to stop a rug pull on other tasks */
	// if (action_confirmed() == false)
	// {
	// 	uart_send_string("\n\rFiling system NOT formated\n\r");
	// 	return true;
	// }

	/* Stop any surveys that are running */

	/* Flush the settings */

	/* Close off any logs */
	// logging_close();

	// lfs_unmount(&lfs);
	// err = lfs_format(&lfs, &cfg);
	// vTaskDelay(1000); /* 5 Seconds */
	// do_system_reset();
    return true;
}

/**
 * @fn void do_system_reset(void)
 * @brief   System Reset
 * @details Initiates a system reset request to reset the MCU. RAM based duplication
 */
static void do_system_reset(void)
{

}

