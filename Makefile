# single_chan_pkt_fwd
# Single Channel LoRaWAN Gateway

CC       = gcc
LDFLAGS  = -ljansson
CPPFLAGS = -std=gnu11
CFLAGS   = -Wall -Wextra -Wfatal-errors -g
TARGET   = single_chan_pkt_fwd

#CXX      = g++
#CXXFLAGS = -std=c++11

SRCFILES = base64.c main.c spi.c gpio.c time_util.c net.c
OBJECTS  = $(patsubst %.c, %.o, $(SRCFILES))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	    $(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o $(TARGET)
