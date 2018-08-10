# Single Channel LoRaWAN Gateway

CC       = gcc
LDFLAGS  = -lprotobuf-c -lpaho-embed-mqtt3c -lpthread
CPPFLAGS = -std=gnu11
INCLUDE  = -I/usr/include/protobuf-c -I. \
		   -Igithub.com/gogo/protobuf/protobuf \
		   -Ipaho.mqtt.embedded-c/MQTTClient-C/src \
		   -Ipaho.mqtt.embedded-c/MQTTPacket/src
CFLAGS   = -Wall -Wextra -Wfatal-errors -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-parameter $(INCLUDE)
TARGET   = lorawan-gateway
SRCFILES = base64.c main.c spi.c gpio.c time_util.c connector.c sx1276.c \
		   paho.mqtt.embedded-c/MQTTClient-C/src/linux/MQTTLinux.c \
		   paho.mqtt.embedded-c/MQTTClient-C/src/MQTTClient.c \
		   github.com/gogo/protobuf/protobuf/google/protobuf/descriptor.pb-c.c \
		   github.com/gogo/protobuf/protobuf/google/protobuf/empty.pb-c.c \
		   github.com/gogo/protobuf/gogoproto/gogo.pb-c.c \
		   github.com/TheThingsNetwork/api/api.pb-c.c \
		   github.com/TheThingsNetwork/api/trace/trace.pb-c.c \
		   github.com/TheThingsNetwork/api/gateway/gateway.pb-c.c \
		   github.com/TheThingsNetwork/api/protocol/protocol.pb-c.c \
		   github.com/TheThingsNetwork/api/protocol/lorawan/lorawan.pb-c.c \
		   github.com/TheThingsNetwork/api/router/router.pb-c.c \
		   github.com/TheThingsNetwork/gateway-connector-bridge/types/types.pb-c.c
OBJECTS  = $(patsubst %.c, %.o, $(SRCFILES))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

PROTOC = protoc-c --c_out=. --proto_path=. \
		 -I github.com/TheThingsNetwork/api \
		 -I github.com/gogo/protobuf/protobuf
protoc:
	$(PROTOC) github.com/gogo/protobuf/protobuf/google/protobuf/descriptor.proto
	$(PROTOC) github.com/gogo/protobuf/protobuf/google/protobuf/empty.proto
	$(PROTOC) github.com/gogo/protobuf/gogoproto/gogo.proto
	$(PROTOC) github.com/TheThingsNetwork/api/api.proto
	$(PROTOC) github.com/TheThingsNetwork/api/trace/trace.proto
	$(PROTOC) github.com/TheThingsNetwork/api/gateway/gateway.proto
	$(PROTOC) github.com/TheThingsNetwork/api/protocol/protocol.proto
	$(PROTOC) github.com/TheThingsNetwork/api/protocol/lorawan/lorawan.proto
	$(PROTOC) github.com/TheThingsNetwork/api/router/router.proto
	$(PROTOC) github.com/TheThingsNetwork/gateway-connector-bridge/types/types.proto

clean:
	rm -f *.o $(TARGET)
