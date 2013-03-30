/* DMX512 Protocol Actions Library
 *
 * Handle DMX512 channel update inputs and commit actions
 */

#ifndef DMX512_H
#define DMX512_H




#include <msp430.h>
#include <stdint.h>



/* Buffer */
extern uint8_t dmx512_buffer[];

/* Initialization */
void dmx512_init();

/* Input updates */
void dmx512_update_channels(uint8_t startcode, uint16_t startchan, uint8_t *inbuf, uint16_t size);

/* Commit updates */
void dmx512_update_commit(uint8_t startcode);

/* Output updates */
void dmx512_output_channels(uint8_t startcode, uint16_t startchan, uint8_t *outbuf, uint16_t size);





#endif /* DMX512_H */
