# single_chan_pkt_fwd
# Single Channel LoRaWAN Gateway

CC       = gcc
#CXX      = g++
LDFLAGS  = -ljansson
CPPFLAGS = -std=gnu11
CFLAGS   = -Wall -Wextra -Wfatal-errors -g
#CXXFLAGS = -std=c++11
TARGET   = single_chan_pkt_fwd

all: $(TARGET)

$(TARGET): base64.o main.o spi.o gpio.o time_util.o
	$(CC) main.o base64.o spi.o gpio.o time_util.o -o $(TARGET) $(LDFLAGS)

main.o: main.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c main.c

base64.o: base64.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c base64.c

spi.o: spi.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c spi.c

gpio.o: gpio.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c gpio.c

time_util.o: time_util.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c time_util.c

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
