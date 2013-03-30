/* packet_processor.c
 * Handle slave node packets (TX or RX)
 */

#include <msp430.h>
#include "dmx512.h"
#include "packet_processor.h"
#include "msprf24.h"
#include <sys/cdefs.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "misc.h"

struct packet_task txtasks[PACKET_TASKLIST_DEPTH];

uint8_t packet_task_append(struct packet_task *task)
{
	int i=0;

	if (task == NULL)
		return 0;  // Invalid request
	while (txtasks[i].active && i < PACKET_TASKLIST_DEPTH)
		i++;
	if (i == PACKET_TASKLIST_DEPTH)
		return 0;  // No free slots in the tasklist queue
	memcpy(&(txtasks[i]), task, sizeof(struct packet_task));
	txtasks[i].active = 1;

	return 1;
}

struct packet_task *packet_task_next()
{
	int i;

	for (i=0; i < PACKET_TASKLIST_DEPTH; i++) {
		if (txtasks[i].active)
			return &txtasks[i];
	}
	return NULL;
}

void packet_init_tasklist()
{
	int i;

	for (i=0; i < PACKET_TASKLIST_DEPTH; i++) {
		txtasks[i].active = 0;
		txtasks[i].program = 0;
		txtasks[i].size = 0;
	}
}

/* Process RX packet payloads
 *
 * Some commands may result in TX replies, which are appended to the
 * packet task list.
 */
uint8_t packet_processor(uint8_t program, uint8_t size, uint8_t *data)
{
	P3OUT |= BIT6;
	switch (program) {
		case 0x03:  // LED test reply
			// Data: ID(1), LED_EN(1)
			if (size != 2)
				return 0;  // Packet length invalid
			if (data[0] != DEVICE_ID)
				return 0;  // Packet was sent to the wrong device
			if (data[1])
				LED_BLUE_PORTOUT |= LED_BLUE_PORT;
			else
				LED_BLUE_PORTOUT &= ~LED_BLUE_PORT;
			return 1;

		case 0x40:  // DMX512 Update COMMIT
			if (size == 1) {
				dmx512_update_commit(data[0]);
			} else {
				return 0;
			}
			break;

		default:
			if (program >= 0x20 && program <= 0x3F) {
				// DMX512 channel segment
				if (size > 1) {  // Channel spec with no data simply not allowed
					dmx512_update_channels(data[0], (program-0x20)*16+1, data+1, size-1);
				}
				return 1;
			}

			// True default action; return failure
			return 0;
	}
	return 1;
}

/* Process all outstanding TX packet payloads of a similar RF address */
uint8_t packet_process_txqueue()
{
	uint8_t packet[32], *curaddr;
	int plen=0;
	struct packet_task *ct;

	// No point in sending a packet until the TX queue is empty
	if ( msprf24_queue_state() & RF24_QUEUE_TXFULL )
		return 0;

	if ((ct = packet_task_next()) == NULL)
		return 0;
	curaddr = ct->rfaddr;
	do {
		if (ct->active && !memcmp(ct->rfaddr, curaddr, 5)) {
			if ( (ct->size + 2 + plen) < 32 ) {
				packet[plen++] = ct->program;
				packet[plen++] = ct->size;
				memcpy(&packet[plen], ct->data, ct->size);
				plen += ct->size;
				ct->active = 0;  // Delete task; it's handled
			}
			if (ct->size > PACKET_TASK_MAX_PAYLOAD)  // Cull any invalid/faulty-sized packets
				ct->active = 0;
		}
		ct = packet_task_next();
	} while (ct != NULL);
	if (!plen)
		return 0;

	w_tx_addr((char*)curaddr);
	w_rx_addr(0, (char*)curaddr);
	w_tx_payload(plen, (char*)packet);

	// Let'er rip
	msprf24_activate_tx();
	return 1;
}
