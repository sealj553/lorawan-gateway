#include "spi.h"

#include <stdio.h>
#include <stdlib.h>

static char dataRead[2];
static char dataWrite[2];

//to change the SPI mode
//uint8_t mode = SPI_MODE_0;
//uint8_t lsb = SPI_LSB_FIRST;
//...
//ioctl(spi, SPI_IOC_WR_MODE, &mode);
//ioctl(spi, SPI_IOC_WR_LSB_FIRST, &lsb);

int spi_init(const char *dev, mode_t mode){
    int spi;
    if((spi = open(dev, mode)) == -1){
        perror("open");
        exit(1);
    }
    return spi;
}

void spi_close(){
}

uint8_t spi_read_reg(uint8_t reg){
    dataWrite[0] = reg & 0x7F;
    dataWrite[1] = 0x00;

    //FastTransfer(dev, dataWrite, dataRead, REG_SZ);

    return dataRead[1];
}

void spi_write_reg(uint8_t reg, uint8_t value){
    dataWrite[0] = reg | 0x80;
    dataWrite[1] = value;

    //FastWrite(dev, dataWrite, REG_SZ);
}
