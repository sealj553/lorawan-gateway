# single_chan_pkt_fwd
# Single Channel LoRaWAN Gateway

CC       = gcc
LDFLAGS  =
CPPFLAGS = -std=gnu11
INCLUDE  = -I/usr/include/protobuf-c-rpc
CFLAGS   = -Wall -Wextra -Wfatal-errors $(INCLUDE)
TARGET   = single_chan_pkt_fwd

#CXX      = g++
#CXXFLAGS = -std=c++11

SRCFILES = base64.c main.c spi.c gpio.c time_util.c net.c
OBJECTS  = $(patsubst %.c, %.o, $(SRCFILES))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	    $(CC) -o $@ $^ $(LDFLAGS)

protoc:
	protoc-c --c_out=@ lib/gateway-connector-bridge/types/types.proto

clean:
	rm -f *.o $(TARGET)
