#include "spi.h"

#include "mpsse.h"
#include <stdio.h>
#include <stdlib.h>

#define SPI_DEV SPI0
#define REG_SZ 2

static struct mpsse_context *flash = NULL;
char dataRead[2];
char dataWrite[2];

int spi_init(){
    return (flash = MPSSE(SPI_DEV, TWELVE_MHZ, MSB)) != NULL && flash->open;
}

const char* spi_get_error(){
    return ErrorString(flash);
}

void spi_rw(){
    Start(flash);
    FastWrite(flash, dataWrite, REG_SZ);
    FastRead(flash, dataRead, REG_SZ);
    Stop(flash);
}

uint8_t ReadRegister(uint8_t reg){
    dataWrite[0] = reg & 0x7F;
    dataWrite[1] = 0x00;

    spi_rw();

    return dataRead[1];
}

void WriteRegister(uint8_t reg, uint8_t value){
    dataWrite[0] = reg | 0x80;
    dataWrite[1] = value;

    spi_rw();
}
