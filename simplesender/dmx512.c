/* DMX512 protocol and buffer management
 *
 */

#include <msp430.h>
#include <stdint.h>
#include <string.h>
#include "dmx512.h"
#include "misc.h"
#include "packet_processor.h"


/* Configuration define's for this specific implementation */
#define DMX512_CHANNEL_START 1
#define DMX512_CHANNEL_COUNT 3
#define DMX512_STARTCODE 0x00
#define DMX512_NRFCMD_CHANNEL 0x20
#define DMX512_NRFCMD_COMMIT 0x40

uint8_t dmx512_buffer[DMX512_CHANNEL_COUNT];
const uint8_t dmxchannel_address[] = { 0xDE, 0xAD, 0xDE, 0xED, 0x01 };


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

// Helper function to submit packet_task entry
void dmx512_submit_nrfpacket(uint8_t, uint8_t, const uint8_t *, const uint8_t *);

// Generate nRF24L01+ output of current buffer contents
void dmx512_output_channels(uint8_t startcode, uint16_t startchan, uint8_t *outbuf, uint16_t size)
{
	uint16_t i;
	uint8_t chanbuf[17], fsz=0, progid=0x00, progid_old=0x00, begin=1;

	if (!startchan)  // Channels start at 1
		return;

	// Nordic nRF24L01+ implementation
	chanbuf[0] = startcode;
	for (i=0; i < size; i++) {
		progid = DMX512_NRFCMD_CHANNEL + (startchan-1+i)/16;
		if (progid != progid_old) {
			if (!begin) {
				dmx512_submit_nrfpacket(progid_old, fsz, chanbuf, dmxchannel_address);
			} else {
				begin = 0;
			}
			fsz = 1;
			progid_old = progid;
		}
		chanbuf[fsz++] = outbuf[i];
	}

	if (fsz > 1) {  // Flush the remainder
		dmx512_submit_nrfpacket(progid, fsz, chanbuf, dmxchannel_address);
	}

	// Send final commit frame
	dmx512_submit_nrfpacket(DMX512_NRFCMD_COMMIT, 1, chanbuf, dmxchannel_address);
	
	return;
}

void dmx512_submit_nrfpacket(uint8_t progID, uint8_t plsize, const uint8_t *buf, const uint8_t *nrfaddr)
{
	struct packet_task t;

	t.program = progID;
	t.size = plsize;
	memcpy(t.rfaddr, nrfaddr, 5);
	memcpy(t.data, buf, plsize);

	packet_task_append(&t);
}

/* Some kind of magic should happen here.  Process the contents of the buffers! */
void dmx512_update_commit(uint8_t startcode)
{
	return;  // Not implemented
}
