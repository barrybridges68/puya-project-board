#ifndef COMMAND_LINE_H
#define COMMAND_LINE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <errno.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define MAX_CMD_LINE_LENGTH 		64	/* Total input line length */
#define CMD_LINE_NONE 				-1  /* Empty command line */
#define CMD_LINE_MAX_ARGS 			4   /* Max number of command line args including the command */
#define CMD_LINE_MAX_ARG_LEN		32  /* Max length of a single command or argument */
#define CMD_LINE_MAX_ARG_HELP_LEN	128  /* Max length of a single command or argument */
#define CMD_OUTPUT_BUFFER_LEN		256 /* Max length of a single command or argument */

#define MAX_LOG_LEVEL 3 /* Move to log file */

/* Character definitions */
#define SPACE      	0x20
#define CRCHAR      0x0D
#define BACKSPACE  	0x08
#define QUOTE      	'\"'
/* Bases and ranges */
#define BASE_10    	10
#define MAX_UINT8  	255
#define MIN_INT8   	-128
#define MAX_INT8   	127
#define MAX_INT16  	32767
#define MIN_INT16  	-32768
#define MAX_UINT16 	65535

/**
 * @brief Parameter types used
 *
 */
typedef enum {
    E_PARAM_STRING,
    E_PARAM_FLOAT,
    E_PARAM_INT32,
    E_PARAM_UINT32,
    E_PARAM_INT16,
    E_PARAM_UINT16,
    E_PARAM_INT8,
    E_PARAM_UINT8
}E_PARAM_TYPE;

/**
 * @brief Return status values.
 *
 */
typedef enum {
    E_CMD_NONE,
    E_CMD_EOL,
    E_CMD_LINE_TOO_LONG,
    E_CMD_ARG_TOO_LONG,
    E_CMD_BACKSPACE,
    E_CMD_TOO_MANY_ARGS,
    E_CMD_QUTOES,
    E_CMD_INVALID_OPTION,
    E_CMD_OK,
    E_CMD_ERROR,
    E_CMD_NOT_ENOUGH_ARGS,
	E_CMD_FILE_NOT_FOUND,
	E_CMD_INVALID_FORMAT,
} E_CMD_RESULTS;

/**
 * @brief Container for parmeters used.
 *
 */
typedef union {
    char     arg_str[CMD_LINE_MAX_ARG_LEN];
    float    arg_float;
    int32_t  arg_int32;
    uint32_t arg_uint32;
    int16_t  arg_int16;
    uint16_t arg_uint16;
    int8_t   arg_int8;
    uint8_t  arg_uint8;
} CMD_LINE_PARAM;

/**
 * @fn void command_line_report(E_CMD_RESULTS report)
 * @brief Report state
 *
 * @return None
 */
void command_line_report(E_CMD_RESULTS report);

/**
 * @fn void command_line_reset(void)
 * @brief Resets all the command line variables between processing
 *
 * @return None
 */
void command_line_reset(void);

/**
 * @fn bool command_line_parameter_convert(CMD_LINE_PARAM *parameter, E_PARAM_TYPE p_type)
 * @brief Converts the string parameter to a type of choise
 *
 * @return True if converstion successful
 */
bool command_line_parameter_convert(CMD_LINE_PARAM *parameter, E_PARAM_TYPE p_type);

/**
 * @fn E_CMD_RESULTS command_line_parse(void)
 * @brief
 *
 * @return E_CMD_RESULTS CMD_LINE_NONE command processed ok.
 */
E_CMD_RESULTS command_line_parse(void);

/**
 * @fn E_CMD_RESULTS command_line_get_line(void)
 * @brief Gets and cleans the parameters.
 *
 * @return E_CMD_RESULTS - E_CMD_NONE is successful
 */
E_CMD_RESULTS command_line_get_parameters(void);

/**
 * @fn E_CMD_RESULTS command_line_get_line(void)
 * @brief Gets and cleans the command line.
 *
 * @return E_CMD_RESULTS - E_CMD_NONE is successful
 */
E_CMD_RESULTS command_line_get_line(void);


void debug_console(void);
#endif
