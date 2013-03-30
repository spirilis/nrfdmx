/* WS2811 ASM MSP430 driver by oPossum with modifications from Rickta59, spirilis
 */


#ifndef WS2811_HS_ONE_H
#define WS2811_HS_ONE_H



#include <msp430.h>
#include <stdint.h>


/* User configuration */
#define WS2811_LED_STRING_LENGTH 28
#define WS2811_PORT_BIT BIT7


/* ASM function prototype */
void write_ws2811_hs_one(uint8_t *data, uint16_t length, uint8_t pinmask);



#endif /* WS2811_HS_ONE_H */
