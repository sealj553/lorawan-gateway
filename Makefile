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


PROTOC = protoc-c --c_out=proto --proto_path=. \
		 -Igithub.com/TheThingsNetwork/api \
		 -Igithub.com/gogo/protobuf/protobuf \
		 github.com/
protoc:
	mkdir -p proto
	$(PROTOC)gogo/protobuf/protobuf/google/protobuf/descriptor.proto
	$(PROTOC)gogo/protobuf/protobuf/google/protobuf/empty.proto
	$(PROTOC)gogo/protobuf/gogoproto/gogo.proto
	$(PROTOC)TheThingsNetwork/api/api.proto
	$(PROTOC)TheThingsNetwork/api/trace/trace.proto
	$(PROTOC)TheThingsNetwork/api/gateway/gateway.proto
	$(PROTOC)TheThingsNetwork/api/protocol/protocol.proto
	$(PROTOC)TheThingsNetwork/api/protocol/lorawan/lorawan.proto
	$(PROTOC)TheThingsNetwork/api/router/router.proto
	$(PROTOC)TheThingsNetwork/gateway-connector-bridge/types/types.proto

clean:
	rm -f *.o $(TARGET)
	rm -rf proto
