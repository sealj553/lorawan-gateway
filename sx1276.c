#include "sx1276.h"
#include "registers.h"
#include "spi.h"
#include "stdio.h"

void set_frequency(uint32_t frequency){
    uint64_t frf = ((uint64_t)frequency << 19) / 32000000;
    spi_write_reg(REG_FRF_MSB, frf >> 16);
    spi_write_reg(REG_FRF_MID, frf >> 8);
    spi_write_reg(REG_FRF_LSB, frf >> 0);
}

void sleep_mode(void){
    spi_write_reg(REG_OPMODE, MODE_SLEEP);
}

uint8_t get_version(void){
    return spi_read_reg(REG_VERSION);
}

bool read_data(uint8_t *payload, uint8_t *p_length){
    //clear rxDone
    spi_write_reg(REG_IRQ_FLAGS, PAYLOAD_LENGTH);

    int irqflags = spi_read_reg(REG_IRQ_FLAGS);

    if((irqflags & PAYLOAD_CRC) == PAYLOAD_CRC) {
        puts("CRC error");
        spi_write_reg(REG_IRQ_FLAGS, PAYLOAD_CRC);
        return false;
    }

    uint8_t receivedCount = spi_read_reg(REG_RX_NB_BYTES);
    *p_length = receivedCount;

    spi_write_reg(REG_FIFO_ADDR_PTR, spi_read_reg(REG_FIFO_RX_CURRENT_ADDR));

    for(int i = 0; i < receivedCount; ++i){
        payload[i] = spi_read_reg(REG_FIFO);
    }

    return true;
}

size_t write_data(const char *buffer, int size){
    //idle/standby mode
    spi_write_reg(REG_OPMODE, MODE_LONG_RANGE_MODE | MODE_STDBY);

    // reset FIFO address and paload length
    spi_write_reg(REG_FIFO_ADDR_PTR, 0);
    spi_write_reg(REG_PAYLOAD_LENGTH, 0);

    int currentLength = spi_read_reg(REG_PAYLOAD_LENGTH);

    //check size
    if((currentLength + size) > MAX_PKT_LENGTH){
        size = MAX_PKT_LENGTH - currentLength;
    }

    //write data
    for(int i = 0; i < size; ++i){
        spi_write_reg(REG_FIFO, buffer[i]);
    }

    //update length
    spi_write_reg(REG_PAYLOAD_LENGTH, currentLength + size);

    //put in TX mode
    spi_write_reg(REG_OPMODE, MODE_LONG_RANGE_MODE | MODE_TX);

    //wait for TX done
    while((spi_read_reg(REG_IRQ_FLAGS) & IRQ_TX_DONE_MASK) == 0);

    // clear IRQ's
    spi_write_reg(REG_IRQ_FLAGS, IRQ_TX_DONE_MASK);

    return size;
}

void set_sync_word(uint8_t sw){
    spi_write_reg(REG_SYNC_WORD, sw);
}

void idle_mode(void){
    spi_write_reg(REG_OPMODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
}
