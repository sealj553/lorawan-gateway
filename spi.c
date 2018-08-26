#include "spi.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//to change the SPI mode
//uint8_t mode = SPI_MODE_0;
//uint8_t lsb = SPI_LSB_FIRST;
//...
//ioctl(spi, SPI_IOC_WR_MODE, &mode);
//ioctl(spi, SPI_IOC_WR_LSB_FIRST, &lsb);


///#define CH341_SPI_MAX_FREQ          1e6
///max sx1276 freq = 10Mhz

int fd;

void spi_init(const char *devname, mode_t mode){
    if((fd = open(devname, mode)) == -1){
        perror("open");
        exit(1);
    }
}

void spi_close(void){
    if(close(fd) == -1){
        perror("close");
    }
}

uint8_t spi_read_reg(uint8_t reg){
    char mosi[2] = { reg & 0x7F, 0x00 };
    char miso[2] = { 0x00 };

    struct spi_ioc_transfer spi_trans;
    memset(&spi_trans, 0, sizeof(spi_trans));
    spi_trans.tx_buf = (unsigned long)&mosi;
    spi_trans.rx_buf = (unsigned long)&miso;
    spi_trans.len    = 2;

    if(ioctl(fd, SPI_IOC_MESSAGE(1), &spi_trans) == -1){
        perror("spi transfer");
        exit(1);
    }

    return miso[1];
}

int spi_write_reg(uint8_t reg, uint8_t value){
    char mosi[2] = { reg | 0x80, value };

    struct spi_ioc_transfer spi_trans;
    memset(&spi_trans, 0, sizeof(spi_trans));
    spi_trans.tx_buf = (unsigned long)&mosi;
    spi_trans.rx_buf = 0;
    spi_trans.len    = 2;

    if(ioctl(fd, SPI_IOC_MESSAGE(1), &spi_trans) == -1){
        perror("spi transfer");
        exit(1);
    }
    return 2;
}
