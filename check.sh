#!/bin/sh
#troublesshoots ch341 connection

if ! type "lsusb" > /dev/null; then
    echo "lsusb not installed, exiting"
    exit
fi

if [[ -z $(lsusb | grep "CH341") ]]; then
    echo "CH341 not plugged in"
    exit
else
    echo "CH341 plugged in"
fi

if ! type "lsmod" > /dev/null; then
    echo "lsmod not installed, exiting"
    exit
fi

if [[ -z $(lsmod | grep "spi_ch341_usb") ]]; then
    echo "spi_ch341_usb kernel module is not loaded"
    exit
else
    echo "kernel module is loaded"
fi

echo "all good"
