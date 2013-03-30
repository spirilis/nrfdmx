/* DMX512 protocol and buffer management
 *
 */

#include <msp430.h>
#include <stdint.h>
#include "dmx512.h"
#include "ws2811_hs_one.h"


/* Configuration define's for this specific implementation */
#define DMX512_CHANNEL_START 1
#define DMX512_CHANNEL_COUNT 3
#define DMX512_STARTCODE 0x00

uint8_t dmx512_buffer[DMX512_CHANNEL_COUNT];

void dmx512_init()
{
	uint16_t i;

	for (i=0; i < DMX512_CHANNEL_COUNT; i++) {
		dmx512_buffer[i] = 0x00;
	}
}

/* Update internal buffers with DMX512 data received exogenously */
void dmx512_update_channels(uint8_t startcode, uint16_t startchan, uint8_t *inbuf, uint16_t size)
{
	uint16_t i, j, k;

	if (startcode != DMX512_STARTCODE || !startchan)
		return;

	k = DMX512_CHANNEL_START + DMX512_CHANNEL_COUNT;
	for (i=0; i < size; i++) {
		j = startchan+i;
		if ( j >= DMX512_CHANNEL_START && j <= k ) {
			dmx512_buffer[j-DMX512_CHANNEL_START] = inbuf[i];
		}
	}
}

void dmx512_output_channels(uint8_t startcode, uint16_t startchan, uint8_t *outbuf, uint16_t size)
{
	return;  // Not implemented here
}

/* Some kind of magic should happen here.  Process the contents of the buffers! */
void dmx512_update_commit(uint8_t startcode)
{
	uint16_t i;
	uint8_t grb[3];

	if (startcode != DMX512_STARTCODE)
		return;

	/* Do something with the buffer data */
	_DINT();  // Interrupts must be disabled since ASM code uses cycle-counting for timing
	for (i=0; i < WS2811_LED_STRING_LENGTH; i++) {
		grb[0] = dmx512_buffer[1];
		grb[1] = dmx512_buffer[0];
		grb[2] = dmx512_buffer[2];
		write_ws2811_hs_one(grb, 3, WS2811_PORT_BIT);
	}
	_EINT();
	__delay_cycles(800);  // 50uS delay
	
	return;
}
