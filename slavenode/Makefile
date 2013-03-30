TARGETMCU	?= msp430f5172

CROSS		:= msp430-
CC		:= $(CROSS)gcc
MSPDEBUG	:= mspdebug
CFLAGS		:= -Os -Wall -Werror -g -mmcu=$(TARGETMCU)
CFLAGS += -fdata-sections -ffunction-sections -Wl,--gc-sections

SRCS			:= main.c clockinit.c vcore.c packet_processor.c msprf24.c dmx512.c ws2811_hs_one.S
PROG			:= nrfdmx_ws2811

all:			$(PROG).elf

$(PROG).elf:	$(OBJS)
	$(CC) $(CFLAGS) -o $(PROG).elf $(SRCS)

clean:
	-rm -f $(PROG).elf

install: $(PROG).elf
	$(MSPDEBUG) -n rf2500 "prog $(PROG).elf"
