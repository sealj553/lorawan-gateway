#include "spi.h"

#include "libmpsse/mpsse.h"

#include <stdio.h>
#include <stdlib.h>

#define SPI_DEV SPI2
#define REG_SZ 2

static struct mpsse_context *dev = NULL;
char dataRead[2];
char dataWrite[2];

int spi_init(){
    return (dev = MPSSE(SPI_DEV, TWELVE_MHZ, MSB)) != NULL && dev->open;
}

void spi_close(){
    Close(dev);
}

const char* spi_get_error(){
    return ErrorString(dev);
}

/*
 * Function to perform fast transfers in MPSSE.
 * mpsse - libmpsse context pointer.
 * wdata - The data to write.
 * rdata - The destination buffer to read data into.
 * size  - The number of bytes to transfer.
 * Returns MPSSE_OK on success, MPSSE_FAIL on failure.
 */
//int FastTransfer(struct mpsse_context *mpsse, const char *wdata, char *rdata, size_t size)

uint8_t ReadRegister(uint8_t reg){
    dataWrite[0] = reg & 0x7F;
    dataWrite[1] = 0x00;

    Start(dev);
    FastTransfer(dev, dataWrite, dataRead, REG_SZ);
    Stop(dev);

    return dataRead[1];
}

/*
 * Function for performing fast writes in MPSSE.
 * mpsse - libmpsse context pointer.
 * data  - The data to write.
 * size  - The number of bytes to write.
 * Returns MPSSE_OK on success, MPSSE_FAIL on failure.
 */
//int FastWrite(struct mpsse_context *mpsse, const char *data, size_t size)
void WriteRegister(uint8_t reg, uint8_t value){
    dataWrite[0] = reg | 0x80;
    dataWrite[1] = value;

    Start(dev);
    FastWrite(dev, dataWrite, REG_SZ);
    Stop(dev);
}
