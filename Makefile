# single_chan_pkt_fwd
# Single Channel LoRaWAN Gateway

CC      = g++
CFLAGS  = -std=c++11 -Wall -Wextra -Wfatal-errors
LDFLAGS = -lmpsse
TARGET  = single_chan_pkt_fwd

all: $(TARGET)

$(TARGET): base64.o main.o spi.o
	$(CC) main.o base64.o spi.o -o $(TARGET) $(LDFLAGS)

main.o: main.cpp
	$(CC) $(CFLAGS) -c main.cpp

base64.o: base64.c
	$(CC) $(CFLAGS) -c base64.c

spi.o: spi.c
	$(CC) $(CFLAGS) -c spi.c

clean:
	rm -f *.o $(TARGET)

#install:
#	sudo cp -f ./single_chan_pkt_fwd.service /lib/systemd/system/
#	sudo systemctl enable single_chan_pkt_fwd.service
#	sudo systemctl daemon-reload
#	sudo systemctl start single_chan_pkt_fwd
#	sudo systemctl status single_chan_pkt_fwd -l
#
#uninstall:
#	sudo systemctl stop single_chan_pkt_fwd
#	sudo systemctl disable single_chan_pkt_fwd.service
#	sudo rm -f /lib/systemd/system/single_chan_pkt_fwd.service 
