# single_chan_pkt_fwd
# Single Channel LoRaWAN Gateway

CC       = gcc
LDFLAGS  = -ljansson
CPPFLAGS = -std=gnu11
INCLUDE  = -IMQTT-C/include
CFLAGS   = -Wall -Wextra -Wfatal-errors $(INCLUDE)
TARGET   = single_chan_pkt_fwd

#CXX      = g++
#CXXFLAGS = -std=c++11

SRCFILES = base64.c main.c spi.c gpio.c time_util.c net.c MQTT-C/src/mqtt.c MQTT-C/src/mqtt_pal.c
OBJECTS  = $(patsubst %.c, %.o, $(SRCFILES))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	    $(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o $(TARGET)
