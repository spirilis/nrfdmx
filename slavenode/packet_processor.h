/* packet_processor.h
 *
 * Handle slave node packets (TX or RX)
 *
 * Headers & reference documentation
 * Struct for managing packet task list
 */

/* Command/packet reference:
 * +---------+-----------+----------------+
 * | PROGRAM | PACKETLEN | PACKETCONTENTS |
 * +---------+-----------+----------------+
 *
 * Programs:
 * 0x02 : LEDtest request (slave->base)
 * 0x03 : LEDtest reply (base->slave)
 * 0x20-0x3F : DMX512 channels 1-512 (16 channels per program ID)
 * 0x40 : DMX512 Commit
 */


#ifndef PACKET_PROCESSOR_H
#define PACKET_PROCESSOR_H


#include <stdint.h>

#define PACKET_TASKLIST_DEPTH 2
#define PACKET_TASK_MAX_PAYLOAD 18

// Packet TX task; a single-linked list
struct packet_task {
	uint8_t active;
	uint8_t program;
	uint8_t size;
	uint8_t rfaddr[5];
	uint8_t data[PACKET_TASK_MAX_PAYLOAD];
};

extern struct packet_task txtasks[PACKET_TASKLIST_DEPTH];  // Task list

void packet_init_tasklist();
uint8_t packet_task_append(struct packet_task *task);
struct packet_task *packet_task_next();

uint8_t packet_processor(uint8_t program, uint8_t size, uint8_t *data); // Processes an RX packet, one at a time, returning 1 if successful

uint8_t packet_process_txqueue();	/* Submits a TX packet stuffing as many packets with the same RF address as can fit
					 * inside a 32-byte packet; returns 1 if successful or 0 if no packets found in the task
					 * queue.
					 * This function runs msprf24_activate_tx() and the MCU should enter LPM sleep shortly after
					 * successful completion of this function, handling IRQ upon wakeup.  After waking up it's
					 * prudent to clear RX pipe#0's address (w_rx_addr(0, <some_dummy_array_of_zeroes>)) to avoid
					 * inadvertently capturing other traffic destined to that remote module if we enter RX mode later.
					 */



#endif /* PACKET_PROCESSOR_H */
