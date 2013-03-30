/* WS2811 LED strip - Controlled via DMX512 over Nordic nRF24L01+
 */

#include <msp430.h>
#include <stdint.h>
#include <sys/cdefs.h>
#include "msprf24.h"
#include "clockinit.h"
#include "dmx512.h"
#include "packet_processor.h"
#include "misc.h"
#include "ws2811_hs_one.h"


const uint8_t grb_off[] = { 0x00, 0x00, 0x00 };
const uint8_t grb_white[] = { 0xFF, 0xFF, 0xFF };
const uint8_t grb_red[] = { 0x00, 0xFF, 0x00 };

const uint8_t my_address[] = { 0xDE, 0xAD, 0xDE, 0xED, 0x01 };

int main()
{
	uint8_t rfbuf[32], i, do_lpm, pktlen, pipeid;

	WDTCTL = WDTPW | WDTHOLD;

	// I/O ports used for status LED, WS2811 LED strip
	P3DIR |= BIT6;
	P1DIR |= BIT7;
	P1OUT &= ~BIT7;
	P3OUT |= BIT6;  // Blue LED signifies clock startup
	ucs_clockinit(16000000, 0, 0);
	UCSCTL5 = DIVS__2;
	__delay_cycles(80000);  // Strawman delay just in case DCO still isn't quite stable
	P3OUT &= ~BIT6;


	// Reset WS2811 LED strips
	for (i=0; i < WS2811_LED_STRING_LENGTH; i++) {
		write_ws2811_hs_one((uint8_t*)grb_off, 3, WS2811_PORT_BIT);
	}

	// Configure Nordic nRF24L01+
	rf_crc = RF24_EN_CRC | RF24_CRCO; // CRC enabled, 16-bit
	rf_addr_width      = 5;
	rf_speed_power     = RF_SPEED | RF24_POWER_MAX;  // RF_SPEED from tcsender.h
	rf_channel         = RF_CHANNEL;  // This #define is located in tcsender.h
	msprf24_init();
	msprf24_open_pipe(1, 0);  // Pipe#1 is the primary receiver for base->slave traffic
	msprf24_set_pipe_packetsize(1, 0);  // Dynamic packet sizes
	w_rx_addr(1, (char*)my_address);  // For receiving base->slave packets
	msprf24_activate_rx();

	packet_init_tasklist();
	dmx512_init();

	while(1) {
		do_lpm = 1;

		// Handle RF acknowledgements
		if (rf_irq & RF24_IRQ_FLAGGED) {
			msprf24_get_irq_reason();

			// Handle TX acknowledgements
			if (rf_irq & (RF24_IRQ_TX|RF24_IRQ_TXFAILED)) {
				// Acknowledge
				msprf24_irq_clear(RF24_IRQ_TX|RF24_IRQ_TXFAILED);
				flush_tx();
			} /* rf_irq & RF24_IRQ_TX|RF24_IRQ_TXFAILED */

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
		}  /* rf_irq & RF24_IRQ_FLAGGED */

		if (packet_task_next() != NULL) {
			packet_process_txqueue();
		}

		if (do_lpm)
			LPM4;
	} /* while(1) */

	return 0;  // Should never reach here
}
