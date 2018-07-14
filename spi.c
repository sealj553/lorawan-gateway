#include "spi.h"

#include <stdio.h>
#include <stdlib.h>

static char dataRead[2];
static char dataWrite[2];

int spi_init(){
    return 0;
}

void spi_close(){
}

uint8_t ReadRegister(uint8_t reg){
    dataWrite[0] = reg & 0x7F;
    dataWrite[1] = 0x00;

    //FastTransfer(dev, dataWrite, dataRead, REG_SZ);

    return dataRead[1];
}

void WriteRegister(uint8_t reg, uint8_t value){
    dataWrite[0] = reg | 0x80;
    dataWrite[1] = value;

    //FastWrite(dev, dataWrite, REG_SZ);
}
