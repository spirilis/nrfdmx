/* Miscellaneous board-specific config -- e.g. test LEDs, DEVICE_ID for nRF24
 * OTA protocol, etc.
 */

#ifndef MISC_H
#define MISC_H


#define DEVICE_ID 0x01
#define LED_BLUE_PORTOUT P3OUT
#define LED_BLUE_PORT BIT6

#define RF_SPEED RF24_SPEED_250KBPS
#define RF_CHANNEL 10

extern const uint8_t dmxchannel_address[];

#endif /* MISC_H */
