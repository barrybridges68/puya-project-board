
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "packets.h"
#include "py32f0xx.h"
#include "py32f0xx_hal.h"
#include "py32f0xx_ll_gpio.h"
#include "py32f0xx_hal_spi.h"
#include "py32f0xx_ll_utils.h"
#include "py32f0xx_ll_spi.h"
#include "py32f0xx_ll_bus.h"

uint8_t wide_band;                /* 2Mbs data rate in use? */
uint8_t p_variant;                /* False for RF24L01 and true for RF24L01P */
uint8_t payload_size;             /**< Fixed size of payloads */
uint8_t ack_payload_available;    /**< Whether there is an ack payload waiting */
uint8_t dynamic_payloads_enabled; /**< Whether dynamic payloads are enabled. */
uint8_t ack_payload_length;       /**< Dynamic size of pending ack payload. */
uint8_t pipe0_reading_address[5] = {0,0,0,0,0}; /**< Last address set on pipe 0 for reading. */

#define CSN_PORT GPIOA
#define CE_PORT  GPIOA

#define CSN LL_GPIO_PIN_6
#define CE  LL_GPIO_PIN_5

SPI_HandleTypeDef spi1_handle;
HAL_StatusTypeDef status;
sTriggerPacket send_packet;

void APP_SPIConfig(void)
{
	__HAL_RCC_GPIOA_CLK_ENABLE();
    // Configure CSN pin
    LL_GPIO_SetPinMode(CSN_PORT, CSN, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinSpeed(CSN_PORT, CSN, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinOutputType(CSN_PORT, CSN, LL_GPIO_OUTPUT_PUSHPULL);
	LL_GPIO_SetPinPull(CSN_PORT, CSN, LL_GPIO_PULL_NO);

    // Configure CE pin
    LL_GPIO_SetPinMode(CE_PORT, CE, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinSpeed(CE_PORT, CE, LL_GPIO_SPEED_FREQ_HIGH);
	LL_GPIO_SetPinOutputType(CE_PORT, CE, LL_GPIO_OUTPUT_PUSHPULL);
	LL_GPIO_SetPinPull(CE_PORT, CE, LL_GPIO_PULL_NO);

    // Configure SPI1 SCK (PA1), MISO (PA0), and MOSI (PA7) pins
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_1, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_0_7(GPIOA, LL_GPIO_PIN_1, LL_GPIO_AF_0);
    LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_1, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_1, LL_GPIO_PULL_NO);

    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_0, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_0_7(GPIOA, LL_GPIO_PIN_0, LL_GPIO_AF_10);
    LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_0, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_0, LL_GPIO_PULL_NO);

    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_7, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_0_7(GPIOA, LL_GPIO_PIN_7, LL_GPIO_AF_0);
    LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_7, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_7, LL_GPIO_PULL_NO);
    
	__HAL_RCC_SPI1_CLK_ENABLE();
    // Configure SPI1
	HAL_SPI_DeInit(&spi1_handle);
    spi1_handle.Instance = SPI1;
    spi1_handle.Init.Mode = SPI_MODE_MASTER;
    spi1_handle.Init.Direction = SPI_DIRECTION_2LINES;
    spi1_handle.Init.DataSize = SPI_DATASIZE_8BIT;
    spi1_handle.Init.CLKPolarity = SPI_POLARITY_LOW;
    spi1_handle.Init.CLKPhase = SPI_PHASE_1EDGE;
    spi1_handle.Init.NSS = SPI_NSS_SOFT;
    spi1_handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    spi1_handle.Init.FirstBit = SPI_FIRSTBIT_MSB;

    if (HAL_SPI_Init(&spi1_handle) != HAL_OK)
    {
        // Initialization Error
        __NOP();
    }

    // Set CSN high
    LL_GPIO_SetOutputPin(CSN_PORT, CSN);
    // Clear CE low
    LL_GPIO_ResetOutputPin(CE_PORT, CE);
}

uint8_t spi_transfer(uint8_t spiData)
{
	uint8_t result;
	HAL_SPI_TransmitReceive(&spi1_handle, &spiData, &result,1 ,10);
	return result;
}

uint8_t min(uint8_t a, uint8_t b)
{
	if (a < b)
		return a;
	return b;
}
/****************************************************************************/

void csn(uint8_t mode)
{
	if (mode == HIGH)
		LL_GPIO_SetOutputPin(CSN_PORT, CSN);
	else
		LL_GPIO_ResetOutputPin(CSN_PORT, CSN);
}

/****************************************************************************/

void ce(uint8_t level)
{
	if (level == HIGH)
		LL_GPIO_SetOutputPin(CSN_PORT, CE);
	else
		LL_GPIO_ResetOutputPin(CSN_PORT, CE);
}

/****************************************************************************/

uint8_t read_registers(uint8_t reg, uint8_t *buf, uint8_t len)
{
	uint8_t status;

	csn(LOW);
	status = spi_transfer(R_REGISTER | (REGISTER_MASK & reg));
	while (len--)
		*buf++ = spi_transfer(0xff);

	csn(HIGH);

	return status;
}

/****************************************************************************/

uint8_t read_register(uint8_t reg)
{
	uint8_t result;
	csn(LOW);
	spi_transfer(R_REGISTER | (REGISTER_MASK & reg));
	result = spi_transfer(0xff);

	csn(HIGH);
	return result;
}

/****************************************************************************/

uint8_t write_registers(uint8_t reg, const uint8_t *buf, uint8_t len)
{
	uint8_t status;

	csn(LOW);
	status = spi_transfer(W_REGISTER | (REGISTER_MASK & reg));
	while (len--)
		spi_transfer(*buf++);

	csn(HIGH);

	return status;
}

/****************************************************************************/

uint8_t write_register(uint8_t reg, uint8_t value)
{
	uint8_t status;
	csn(LOW);
	status = spi_transfer(W_REGISTER | (REGISTER_MASK & reg));
	spi_transfer(value);
	csn(HIGH);

	return status;
}

/****************************************************************************/

uint8_t write_payload(const void *buf, uint8_t len)
{
	uint8_t status;

	const uint8_t *current = buf;

	uint8_t data_len = min(len, payload_size);
	uint8_t blank_len = dynamic_payloads_enabled ? 0 : payload_size - data_len;

	csn(LOW);
	status = spi_transfer(W_TX_PAYLOAD);
	while (data_len--)
		spi_transfer(*current++);
	while (blank_len--)
		spi_transfer(0);
	csn(HIGH);

	return status;
}

/****************************************************************************/

uint8_t read_payload(void *buf, uint8_t len)
{
	uint8_t status;
	uint8_t *current = buf;

	uint8_t data_len = min(len, payload_size);
	uint8_t blank_len = dynamic_payloads_enabled ? 0 : payload_size - data_len;

	csn(LOW);
	status = spi_transfer(R_RX_PAYLOAD);
	while (data_len--)
		*current++ = spi_transfer(0xff);
	while (blank_len--)
		spi_transfer(0xff);
	csn(HIGH);

	return status;
}

/****************************************************************************/

uint8_t flush_rx(void)
{
	uint8_t status;

	csn(LOW);
	status = spi_transfer(FLUSH_RX);
	csn(HIGH);

	return status;
}

/****************************************************************************/

uint8_t flush_tx(void)
{
	uint8_t status;

	csn(LOW);
	status = spi_transfer(FLUSH_TX);
	csn(HIGH);

	return status;
}

/****************************************************************************/

uint8_t get_status(void)
{
	uint8_t status;

	csn(LOW);
	status = spi_transfer(NOP);
	csn(HIGH);

	return status;
}

/**************************************************************************

void RF24Init( void )
{
 	wide_band = TRUE;
	p_variant = FALSE; 
	payload_size = 32;
	ack_payload_available = FALSE;
	dynamic_payloads_enabled = FALSE;
	//pipe0_reading_address ={0}

}
**/
/****************************************************************************/

void setChannel(uint8_t channel)
{
	// TODO: This method could take advantage of the 'wide_band' calculation
	// done in setChannel() to require certain channel spacing.
	//uint8_t data;
	const uint8_t max_channel = 127;
	write_register(RF_CH, min(channel, max_channel));
	//data = read_register(RF_CH);
}

/****************************************************************************/

void setPayloadSize(uint8_t size)
{
	const uint8_t max_payload_size = 32;
	payload_size = min(size, max_payload_size);
}

/****************************************************************************/

uint8_t getPayloadSize(void)
{
	return payload_size;
}

bool id_chip_connected( void )
{
    volatile uint8_t setup = read_register(SETUP_AW);
    if (setup >= 1 && setup <= 3) {
        return true;
    }

    return false;
}
/****************************************************************************/

void begin(void)
{
	ce(LOW);
	csn(HIGH);

	// Must allow the radio time to settle else configuration bits will not necessarily stick.
	// This is actually only required following power up but some settling time also appears to
	// be required after resets too. For full coverage, we'll always assume the worst.
	// Enabling 16b CRC is by far the most obvious case if the wrong timing is used - or skipped.
	// Technically we require 4.5ms + 14us as a worst case. We'll just call it 5ms for good measure.
	// WARNING: Delay is based on P-variant whereby non-P *may* require different timing.
	//Timer3_Delay10us(500);
	LL_mDelay(5);

	// Set 1500uS (minimum for 32B payload in ESB@250KBPS) timeouts, to make testing a little easier
	// WARNING: If this is ever lowered, either 250KBS mode with AA is broken or maximum packet
	// sizes must never be used. See documentation for a more complete explanation.
	write_register(SETUP_RETR, (0x04 << ARD) | (0x0f << ARC));

	// Restore our default PA level
	setPALevel(RF24_PA_HIGH);

	// Determine if this is a p or non-p RF24 module and then
	// reset our data rate back to default value. This works
	// because a non-P variant won't allow the data rate to
	// be set to 250Kbps.
	if (setDataRate(RF24_250KBPS))
	{
		p_variant = TRUE;
	}

	// Then set the data rate to the slowest (and most reliable) speed supported by all
	// hardware.
	setDataRate(RF24_250KBPS);

	// Initialize CRC and request 2-byte (16bit) CRC
	setCRCLength(RF24_CRC_16);

	// Disable dynamic payloads, to match dynamic_payloads_enabled setting
	write_register(DYNPD, 0);

	// Reset current status
	// Notice reset and flush is the last thing we do
	write_register(STATUS, _BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT));

	// Set up default configuration.  Callers can always change it later.
	// This channel should be universally safe and not bleed over into adjacent
	// spectrum.
	setChannel(60);

	// Flush buffers
	flush_rx();
	flush_tx();

	wide_band = TRUE;
	p_variant = FALSE;
	payload_size = 32;
	ack_payload_available = FALSE;
	dynamic_payloads_enabled = FALSE;
	//pipe0_reading_address (0);
        /*
        write_register(CONFIG, (_BV(EN_CRC) | _BV(CRCO)) );    
        uint8_t config_reg = read_register(CONFIG);
        powerUp();
        // if config is not set correctly then there was a bad response from module
        config_reg = (_BV(EN_CRC) | _BV(CRCO) | _BV(PWR_UP)) ? 1 : 0;    
        if (config_reg == 0)
        {
            asm("NOP");
        }*/
        id_chip_connected();
}

/****************************************************************************/

void startListening(void)
{
	write_register(CONFIG, read_register(CONFIG) | _BV(PWR_UP) | _BV(PRIM_RX));
	write_register(STATUS, _BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT));
	write_registers(RX_ADDR_P0, pipe0_reading_address, 5);

	// Flush buffers
	flush_rx();
	flush_tx();

	// Go!
	ce(HIGH);

	// wait for the radio to come up (130us actually only needed)
	//Timer3_Delay10us(130);
	LL_mDelay(1);
}

/****************************************************************************/

void stopListening(void)
{
	ce(LOW);
	flush_tx();
	flush_rx();
}

/****************************************************************************/

void powerDown(void)
{
	LL_mDelay(10);
	write_register(CONFIG, read_register(CONFIG) & ~_BV(PWR_UP));
	LL_mDelay(10);
}

/****************************************************************************/

void powerUp(void)
{
	LL_mDelay(10);
	write_register(CONFIG, read_register(CONFIG) | _BV(PWR_UP));
	//Timer3_Delay10us(100);

	LL_mDelay(10);
}

/******************************************************************/

uint8_t write(const void *buf, uint8_t len)
{
	uint8_t result = FALSE;
	uint8_t observe_tx;
	uint8_t status;
	uint8_t tx_ok;
	uint8_t tx_fail;
	uint8_t time_out_count = 150; 
	// Begin the write
	startWrite(buf, len);

	// ------------
	// At this point we could return from a non-blocking write, and then call
	// the rest after an interrupt

	// Instead, we are going to block here until we get TX_DS (transmission completed and ack'd)
	// or MAX_RT (maximum retries, transmission failed).  Also, we'll timeout in case the radio
	// is flaky and we get neither.

	// IN the end, the send should be blocking.  It comes back in 60ms worst case, or much faster
	// if I tighted up the retry logic.  (Default settings will be 1500us.
	// Monitor the send

	// Restore the pipe0 adddress, if exists
	// if (pipe0_reading_address)
	do
	{
		status = read_register(OBSERVE_TX);
		//Timer3_Delay10us(1);
		LL_mDelay(1);
		time_out_count--;
	} while ( !(status & (_BV(TX_DS) | _BV(MAX_RT))) && time_out_count   );

	// The part above is what you could recreate with your own interrupt handler,
	// and then call this when you got an interrupt
	// ------------

	// Call this when you get an interrupt
	// The status tells us three things
	// * The send was successful (TX_DS)
	// * The send failed, too many retries (MAX_RT)
	// * There is an ack packet waiting (RX_DR)

	whatHappened(&tx_ok, &tx_fail, &ack_payload_available);

	result = tx_ok;

	// Handle the ack packet
	if (ack_payload_available)
	{
		ack_payload_length = getDynamicPayloadSize();
	}

	// Yay, we are done.

	// Power down
	powerDown();

	// Flush buffers (Is this a relic of past experimentation, and not needed anymore??)
	flush_tx();

	return result;
}
/****************************************************************************/

void startWrite(const void *buf, uint8_t len)
{
	// Transmitter power-up
	write_register(CONFIG, (read_register(CONFIG) | _BV(PWR_UP)) & ~_BV(PRIM_RX));
	//Timer3_Delay10us(15);
	LL_mDelay(1);

	// Send the payload
	write_payload(buf, len);

	// Allons!
	ce(HIGH);
	//Timer3_Delay10us(2);
	LL_mDelay(1);
	ce(LOW);
}

/****************************************************************************/

uint8_t getDynamicPayloadSize(void)
{
	uint8_t result = 0;

	csn(LOW);
	spi_transfer(R_RX_PL_WID);
	result = spi_transfer(0xff);
	csn(HIGH);

	return result;
}

/****************************************************************************/

uint8_t available(void)
{
	return availablePipe(NULL);
}

/****************************************************************************/

uint8_t availablePipe(uint8_t *pipe_num)
{
      uint8_t status = get_status();

      // Too noisy, enable if you really want lots o data!!
      //IF_SERIAL_DEBUG(print_status(status));
//      hex(status);
//      Send_Data_To_UART0('\n');
//      Send_Data_To_UART0('\r');
      uint8_t result = (status & _BV(RX_DR));




      if (result)
      {
            // If the caller wants the pipe number, include that
            if (pipe_num)
                    *pipe_num = (status >> RX_P_NO) & 0x07;

            // Clear the status bit

            // ??? Should this REALLY be cleared now?  Or wait until we
            // actually READ the payload?

            write_register(STATUS, _BV(RX_DR));

            // Handle ack payload receipt
            if (status & _BV(TX_DS))
            {
                    write_register(STATUS, _BV(TX_DS));
            }
      }

      return result;
}

/****************************************************************************/

uint8_t read(void *buf, uint8_t len)
{
	// Fetch the payload
	read_payload(buf, len);

	// was this the last of the data available?
	return read_register(FIFO_STATUS) & _BV(RX_EMPTY);
}

/****************************************************************************/

void whatHappened(uint8_t *tx_ok, uint8_t *tx_fail, uint8_t *rx_ready)
{
	// Read the status & reset the status in one easy call
	// Or is that such a good idea?
	uint8_t status = write_register(STATUS, _BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT));

	// Report to the user what happened
	*tx_ok = status & _BV(TX_DS);
	*tx_fail = status & _BV(MAX_RT);
	*rx_ready = status & _BV(RX_DR);
}

/****************************************************************************/

void openWritingPipe(uint8_t *value)
{
	// Note that AVR 8-bit uC's store this LSB first, and the NRF24L01(+)
	// expects it LSB first too, so we're good.
	const uint8_t max_payload_size = 32;
	write_registers(RX_ADDR_P0, value, 5);
	write_registers(TX_ADDR, value, 5);

	write_register(RX_PW_P0, min(payload_size, max_payload_size));
}

/****************************************************************************/

static const uint8_t child_pipe[] =
		{
				RX_ADDR_P0, RX_ADDR_P1, RX_ADDR_P2, RX_ADDR_P3, RX_ADDR_P4, RX_ADDR_P5};
static const uint8_t child_payload_size[] =
		{
				RX_PW_P0, RX_PW_P1, RX_PW_P2, RX_PW_P3, RX_PW_P4, RX_PW_P5};
static const uint8_t child_pipe_enable[] =
		{
				ERX_P0, ERX_P1, ERX_P2, ERX_P3, ERX_P4, ERX_P5};

void openReadingPipe(uint8_t child, uint8_t *address)
{
	// If this is pipe 0, cache the address.  This is needed because
	// openWritingPipe() will overwrite the pipe 0 address, so
	// startListening() will have to restore it.
	if (child == 0)
		memcpy(pipe0_reading_address, address, 5);

	if (child <= 6)
	{

		// For pipes 2-5, only write the LSB
		if (child < 2)
			write_registers(child_pipe[child], address, 5);
		else
			write_registers(child_pipe[child], address, 1);

		write_register(child_payload_size[child], payload_size);

		// Note it would be more efficient to set all of the bits for all open
		// pipes at once.  However, I thought it would make the calling code
		// more simple to do it this way.
		write_register(EN_RXADDR, read_register(EN_RXADDR) | _BV(child_pipe_enable[child]));
	}
}

/****************************************************************************/

void toggle_features(void)
{
	csn(LOW);
	spi_transfer(ACTIVATE);
	spi_transfer(0x73);
	csn(HIGH);
}

/****************************************************************************/

void enableDynamicPayloads(void)
{
	// Enable dynamic payload throughout the system
	write_register(FEATURE, read_register(FEATURE) | _BV(EN_DPL));

	// If it didn't work, the features are not enabled
	if (!read_register(FEATURE))
	{
		// So enable them and try again
		toggle_features();
		write_register(FEATURE, read_register(FEATURE) | _BV(EN_DPL));
	}

	// Enable dynamic payload on all pipes
	//
	// Not sure the use case of only having dynamic payload on certain
	// pipes, so the library does not support it.
	write_register(DYNPD, read_register(DYNPD) | _BV(DPL_P5) | _BV(DPL_P4) | _BV(DPL_P3) | _BV(DPL_P2) | _BV(DPL_P1) | _BV(DPL_P0));

	dynamic_payloads_enabled = TRUE;
}

/****************************************************************************/

void enableAckPayload(void)
{
	//
	// enable ack payload and dynamic payload features
	//

	write_register(FEATURE, read_register(FEATURE) | _BV(EN_ACK_PAY) | _BV(EN_DPL));

	// If it didn't work, the features are not enabled
	if (!read_register(FEATURE))
	{
		// So enable them and try again
		toggle_features();
		write_register(FEATURE, read_register(FEATURE) | _BV(EN_ACK_PAY) | _BV(EN_DPL));
	}

	//
	// Enable dynamic payload on pipes 0 & 1
	//

	write_register(DYNPD, read_register(DYNPD) | _BV(DPL_P1) | _BV(DPL_P0));
}

/****************************************************************************/

void writeAckPayload(uint8_t pipe, const void *buf, uint8_t len)
{
	const uint8_t *current = (uint8_t *)buf;
	const uint8_t max_payload_size = 32;
	uint8_t data_len;
	csn(LOW);
	spi_transfer(W_ACK_PAYLOAD | (pipe & 0x07));

	data_len = min(len, max_payload_size);
	while (data_len--)
		spi_transfer(*current++);

	csn(HIGH);
}

/****************************************************************************/

uint8_t isAckPayloadAvailable(void)
{
	uint8_t result = ack_payload_available;
	ack_payload_available = FALSE;
	return result;
}

/****************************************************************************/

uint8_t isPVariant(void)
{
	return p_variant;
}

/****************************************************************************/

void setAutoAck(uint8_t enable)
{
	if (enable)
		write_register(EN_AA, 0x3F);
	else
		write_register(EN_AA, 0);
}

/****************************************************************************/

void setAutoAckPipe(uint8_t pipe, uint8_t enable)
{
	if (pipe <= 6)
	{
		uint8_t en_aa = read_register(EN_AA);
		if (enable)
		{
			en_aa |= _BV(pipe);
		}
		else
		{
			en_aa &= ~_BV(pipe);
		}
		write_register(EN_AA, en_aa);
	}
}

/****************************************************************************/

uint8_t testCarrier(void)
{
	return (read_register(CD) & 1);
}

/****************************************************************************/

uint8_t testRPD(void)
{
	return (read_register(RPD) & 1);
}

/****************************************************************************/

void setPALevel(rf24_pa_dbm_e level)
{
	uint8_t setup = read_register(RF_SETUP);
	setup &= ~(_BV(RF_PWR_LOW) | _BV(RF_PWR_HIGH));

	// switch uses RAM (evil!)
	if (level == RF24_PA_MAX)
	{
		setup |= (_BV(RF_PWR_LOW) | _BV(RF_PWR_HIGH));
	}
	else if (level == RF24_PA_HIGH)
	{
		setup |= _BV(RF_PWR_HIGH);
	}
	else if (level == RF24_PA_LOW)
	{
		setup |= _BV(RF_PWR_LOW);
	}
	else if (level == RF24_PA_MIN)
	{
		// nothing
	}
	else if (level == RF24_PA_ERROR)
	{
		// On error, go to maximum PA
		setup |= (_BV(RF_PWR_LOW) | _BV(RF_PWR_HIGH));
	}

	write_register(RF_SETUP, setup);
}

/****************************************************************************/

rf24_pa_dbm_e getPALevel(void)
{
	rf24_pa_dbm_e result = RF24_PA_ERROR;
	uint8_t power = read_register(RF_SETUP) & (_BV(RF_PWR_LOW) | _BV(RF_PWR_HIGH));

	// switch uses RAM (evil!)
	if (power == (_BV(RF_PWR_LOW) | _BV(RF_PWR_HIGH)))
	{
		result = RF24_PA_MAX;
	}
	else if (power == _BV(RF_PWR_HIGH))
	{
		result = RF24_PA_HIGH;
	}
	else if (power == _BV(RF_PWR_LOW))
	{
		result = RF24_PA_LOW;
	}
	else
	{
		result = RF24_PA_MIN;
	}

	return result;
}

/****************************************************************************/

uint8_t setDataRate(rf24_datarate_e speed)
{
	uint8_t result = FALSE;
	uint8_t setup = read_register(RF_SETUP);

	// HIGH and LOW '00' is 1Mbs - our default
	wide_band = FALSE;
	setup &= ~(_BV(RF_DR_LOW) | _BV(RF_DR_HIGH));
	if (speed == RF24_250KBPS)
	{
		// Must set the RF_DR_LOW to 1; RF_DR_HIGH (used to be RF_DR) is already 0
		// Making it '10'.
		wide_band = FALSE;
		setup |= _BV(RF_DR_LOW);
	}
	else
	{
		// Set 2Mbs, RF_DR (RF_DR_HIGH) is set 1
		// Making it '01'
		if (speed == RF24_2MBPS)
		{
			wide_band = TRUE;
			setup |= _BV(RF_DR_HIGH);
		}
		else
		{
			// 1Mbs
			wide_band = FALSE;
		}
	}
	write_register(RF_SETUP, setup);

	// Verify our result
	if (read_register(RF_SETUP) == setup)
	{
		result = TRUE;
	}
	else
	{
		wide_band = FALSE;
	}

	return result;
}

/****************************************************************************/

rf24_datarate_e getDataRate(void)
{
	rf24_datarate_e result;
	uint8_t dr = read_register(RF_SETUP) & (_BV(RF_DR_LOW) | _BV(RF_DR_HIGH));

	// switch uses RAM (evil!)
	// Order matters in our case below
	if (dr == _BV(RF_DR_LOW))
	{
		// '10' = 250KBPS
		result = RF24_250KBPS;
	}
	else if (dr == _BV(RF_DR_HIGH))
	{
		// '01' = 2MBPS
		result = RF24_2MBPS;
	}
	else
	{
		// '00' = 1MBPS
		result = RF24_1MBPS;
	}
	return result;
}

/****************************************************************************/

void setCRCLength(rf24_crclength_e length)
{
	uint8_t config = read_register(CONFIG) & ~(_BV(CRCO) | _BV(EN_CRC));

	// switch uses RAM (evil!)
	if (length == RF24_CRC_DISABLED)
	{
		// Do nothing, we turned it off above.
	}
	else if (length == RF24_CRC_8)
	{
		config |= _BV(EN_CRC);
	}
	else
	{
		config |= _BV(EN_CRC);
		config |= _BV(CRCO);
	}
	write_register(CONFIG, config);
}

/****************************************************************************/

rf24_crclength_e getCRCLength(void)
{
	rf24_crclength_e result = RF24_CRC_DISABLED;
	uint8_t config = read_register(CONFIG) & (_BV(CRCO) | _BV(EN_CRC));

	if (config & _BV(EN_CRC))
	{
		if (config & _BV(CRCO))
			result = RF24_CRC_16;
		else
			result = RF24_CRC_8;
	}

	return result;
}

/****************************************************************************/

void disableCRC(void)
{
	uint8_t disable = read_register(CONFIG) & ~_BV(EN_CRC);
	write_register(CONFIG, disable);
}

/****************************************************************************/
void setRetries(uint8_t delay, uint8_t count)
{
	write_register(SETUP_RETR, (delay & 0xf) << ARD | (count & 0xf) << ARC);
}

// Module data pipes
uint8_t pipeTx[5] = {0xc7, 0xc7, 0xc7, 0xc7, 0xc7};
uint8_t pipeRx[5] = {0xe7, 0xe7, 0xe7, 0xe7, 0xe7};

uint8_t rfbuff[28];

void nrf_init(void)
{
	begin();
    setPayloadSize(24);
    setChannel(60);
    setDataRate(RF24_250KBPS);
    enableDynamicPayloads();
    openWritingPipe(pipeTx);
    openReadingPipe(1, pipeRx);
    setAutoAck(1);
    //powerUp();
    startListening();
	send_packet.mHeader.mCounter = 0;	
}

uint8_t makePacketChecksum ( uint8_t * packet, uint8_t length )
{
	uint8_t chksum = 0;
	uint8_t *ptr = (uint8_t*)packet;
	uint8_t count;
	
	for ( count =0; count < length-1; count ++ )
	{
		chksum += *ptr++;
	}	
	return ~chksum;
}
uint32_t count = 0;
void nrf_send(uint8_t buttons)
{
	powerUp();
		/* Reset the counter to 0 */
	send_packet.mHeader.mCounter = count++;
	/* Re-read in case changed in registration */
	send_packet.mHeader.mNodeId  = 0x1234;	
	send_packet.mHeader.mVersion = 4;
	send_packet.mHeader.mStart = PACKET_START;
	send_packet.mHeader.mPacketType = ePacketTypeTrigger;
	send_packet.mHeader.mBatteryVolts = 0x21;
	send_packet.mTriggerData.triggerstate = buttons;
	send_packet.mChecksum = 0x00;
	send_packet.mChecksum = makePacketChecksum((uint8_t*)&send_packet, sizeof(send_packet));

	write((uint8_t*)&send_packet,sizeof(send_packet));

//	powerDown();	
}