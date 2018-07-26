# Single Channel LoRaWAN Gateway

This is a single channel LoRaWAN gateway for use with Semtech SX1276 (HopeRF RFM95W) transcievers connected over USB via FTDI 2232H or WCH CH341A. This is written in C and is for use on Linux (embedded or desktop) as a low-cost LoRaWAN gateway.

It outputs its data in The Things Network's Gateway Connector protocol (protocol buffer encoded over MQTT). (soon to be over SSL)

It was originally forked from [hallard/single_chan_pkt_fwd](https://github.com/hallard/single_chan_pkt_fwd) forked from [jlesech/single_chan_pkt_fwd](https://github.com/jlesech/single_chan_pkt_fwd) forked from [tftelkamp/single_chan_pkt_fwd](https://github.com/tftelkamp/single_chan_pkt_fwd).

## Features
* Configurable frequency and spreading factor (SF7-SF12)
* Can forward data to any number of servers (DNS resolved)
* Edit `config.h` to change various options

## Dependencies
[protobuf-c](https://github.com/protobuf-c/protobuf-c) and [protobuf-c-rpc](https://github.com/protobuf-c/protobuf-c-rpc) for packing and sending data

You also need gschorcht's [spi-ch341-usb](https://github.com/gschorcht/spi-ch341-usb) kernel module installed.

## Building
```shell
git clone https://github.com/sealj553/single_chan_pkt_fwd.git
cd single_chan_pkt_fwd
make protoc
make
sudo ./single_chan_pkt_fwd
```

## Liscense
base64.c and base64.h are licensed under the Revised BSD License

All other files licensed under the Eclipse Public License v1.0
