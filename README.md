# Single Channel LoRaWAN Gateway

This is a single channel LoRaWAN gateway for use with Semtech SX1276 (HopeRF RFM95W) transcievers connected over USB via FTDI 2232H or WCH CH341A. This is written in C and is for use on Linux (embedded or desktop) as a low-cost LoRaWAN gateway.

It was originally forked from [hallard/single_chan_pkt_fwd](https://github.com/hallard/single_chan_pkt_fwd) forked from [jlesech/single_chan_pkt_fwd](https://github.com/jlesech/single_chan_pkt_fwd) forked from [tftelkamp/single_chan_pkt_fwd](https://github.com/tftelkamp/single_chan_pkt_fwd) (whew!).

## Features
* Configurable frequency and spreading factor (SF7-SF12)
* Can forward data to any number of servers (DNS resolved)
* Edit `config.h` to change various options

## Dependencies
[jansson](https://github.com/akheron/jansson) is used to pack json data

For a debian-based distro:
```shell
sudo apt-get install libjansson-dev
```

## Building
```shell
git clone https://github.com/sealj553/single_chan_pkt_fwd.git
cd single_chan_pkt_fwd
make
sudo ./single_chan_pkt_fwd
```

## Liscense
base64.c and base64.h are licensed under the Revised BSD License

All other files licensed under the Eclipse Public License v1.0