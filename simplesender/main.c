/* WS2811 LED strip - Controlled via DMX512 over Nordic nRF24L01+
 */

#include <msp430.h>
#include <stdint.h>
#include <sys/cdefs.h>
#include <string.h>
#include "msprf24.h"
#include "clockinit.h"
#include "dmx512.h"
#include "packet_processor.h"
#include "misc.h"


const uint8_t rgb_off[] = { 0x00, 0x00, 0x00 };
const uint8_t rgb_white[] = { 0xFF, 0xFF, 0xFF };
const uint8_t rgb_red[] = { 0xFF, 0x00, 0x00 };
const uint8_t rgb_cust[] = { 0x00, 0xFF, 0x00 };


int main()
{
	uint8_t rfbuf[32], i, do_lpm, pktlen, pipeid;

	WDTCTL = WDTPW | WDTHOLD;


	// I/O ports used for status LED
	//P3DIR |= BIT6;
	P1DIR |= BIT0;
	//P1OUT &= ~BIT7;
	P1OUT &= ~BIT0;
	//P3OUT |= BIT6;  // Blue LED signifies clock startup

	/* MSP430 F5172 */
	//ucs_clockinit(16000000, 0, 0);
	/* MSP430 G-series */
	DCOCTL = CALDCO_16MHZ;
	BCSCTL1 = CALBC1_16MHZ;
	BCSCTL2 = DIVS_1;

	//P3OUT &= ~BIT6;



	// Configure Nordic nRF24L01+
	rf_crc = RF24_EN_CRC | RF24_CRCO; // CRC enabled, 16-bit
	rf_addr_width      = 5;
	rf_speed_power     = RF_SPEED | RF24_POWER_MAX;  // RF_SPEED from tcsender.h
	rf_channel         = RF_CHANNEL;  // This #define is located in tcsender.h
	msprf24_init();
	msprf24_open_pipe(0, 0);
	msprf24_set_pipe_packetsize(0, 0);

	packet_init_tasklist();
	dmx512_init();

	// Submit one color change.
	memcpy(dmx512_buffer, rgb_cust, 3);
	dmx512_output_channels(0x00, 1, dmx512_buffer, 3);

	while(1) {
		do_lpm = 1;

		// Handle RF acknowledgements
		if (rf_irq & RF24_IRQ_FLAGGED) {
			msprf24_get_irq_reason();

			// Handle TX acknowledgements
			if (rf_irq & RF24_IRQ_TX) {
				// Acknowledge
				msprf24_irq_clear(RF24_IRQ_TX);
				P1OUT &= ~BIT0;
			}

			if (rf_irq & RF24_IRQ_TXFAILED) {
				msprf24_irq_clear(RF24_IRQ_TXFAILED);
				flush_tx();
			}

			if (rf_irq & RF24_IRQ_RX) {
				pktlen = r_rx_peek_payload_size();
				if (pktlen > 0 && pktlen <= 32) {
					pipeid = r_rx_payload(pktlen, (char*)rfbuf);
					if (pipeid == 1) {
						for (i=0; i < pktlen; i++) {
							if (rfbuf[i]) {
								/* Process this packet if it's valid (and payload length doesn't send us
								 *   past the end of the rfbuf buffer)
								 */
								if ( (i+rfbuf[i+1] + 1) < pktlen ) {
									packet_processor(rfbuf[i],
											 rfbuf[i+1],
											 &rfbuf[i+2]);
									i += rfbuf[i+1] + 1;
								} else {
									i = pktlen;  /* Otherwise, we're done, we assume the
										      *   the rest of the payload is corrupt.
										      */
								}
							} /* rfbuf[i] != 0x00 (i.e. valid program ID) */
						} /* for(0 .. pktlen) */
					} /* pipeid == 1 */
				} else {
					// False alarm; bad packet, nuke it.
					flush_rx();
				} /* pktlen is 1..32 */
				msprf24_irq_clear(RF24_IRQ_RX);
			} /* rf_irq & RF24_IRQ_RX */

			do_lpm = 0;
		}  /* rf_irq & RF24_IRQ_FLAGGED */

		if (packet_task_next() != NULL) {
			packet_process_txqueue();
		}

		if (do_lpm)
			LPM4;
	} /* while(1) */

	return 0;  // Should never reach here
}
