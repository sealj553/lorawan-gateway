# Single Channel LoRaWAN Gateway

* This is a work in progress -- don't expect it to work

This is a single channel LoRaWAN gateway for use with Semtech SX1276 (HopeRF RFM95W) transcievers connected over USB via WCH CH341A (and maybe FTDI 2232H). This is written in C and is for use on Linux (embedded or desktop) as a low-cost LoRaWAN gateway.

It outputs its data in The Things Network's Gateway Connector protocol (protocol buffer encoded over MQTT).

It was originally forked from [hallard/single_chan_pkt_fwd](https://github.com/hallard/single_chan_pkt_fwd) forked from [jlesech/single_chan_pkt_fwd](https://github.com/jlesech/single_chan_pkt_fwd) forked from [tftelkamp/single_chan_pkt_fwd](https://github.com/tftelkamp/single_chan_pkt_fwd).

## Features
* Configurable frequency and spreading factor (SF7-SF12)
* Can forward data to any number of servers (DNS resolved)
* Edit `config.h` to change various options

## Dependencies
[protobuf-c](https://github.com/protobuf-c/protobuf-c)

You also need gschorcht's [spi-ch341-usb](https://github.com/gschorcht/spi-ch341-usb) kernel module installed.

TTN's [forked version](https://github.com/TheThingsNetwork/paho.mqtt.embedded-c.git) of paho.mqtt.embedded-c is included as a submodule

## Building
```shell
git clone --recurse-submodules -j8 --depth 1 https://github.com/sealj553/lorawan-gateway.git
cd lorawan-gateway

#build the paho library
cd paho.mqtt.embedded-c
make
sudo make install #or directly link with .so files
cd ../../

#compile the proto files
make protoc

#finally build the project
make

#must be run as root to interface with hardware
sudo ./lorawan-gateway
```

## License
base64.c and base64.h are licensed under the Revised BSD License

connector.c/h, network.h, and session.h are licensed under the MIT License

All other files licensed under the Eclipse Public License v1.0
