# single_chan_pkt_fwd
# Single Channel LoRaWAN Gateway

CC       = gcc
CXX      = g++
LDFLAGS  = -lftdi
CPPFLAGS =
CFLAGS   = -Wall -Wextra -Wfatal-errors
CXXFLAGS = -std=c++11
TARGET   = single_chan_pkt_fwd

all: $(TARGET)

$(TARGET): base64.o main.o spi.o mpsse.o fast.o support.o
	$(CXX) main.o base64.o spi.o mpsse.o fast.o support.o -o $(TARGET) $(LDFLAGS)

mpsse.o: mpsse.c
	$(CC) $(CFLAGS) -c mpsse.c

fast.o: fast.c
	$(CC) $(CFLAGS) -c fast.c

support.o: support.c
	$(CC) $(CFLAGS) -c support.c

main.o: main.cpp
	$(CXX) $(CFLAGS) -c main.cpp

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
