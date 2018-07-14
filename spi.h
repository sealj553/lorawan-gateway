#ifndef SPI_H
#define SPI_H

#include <fcntl.h>
#include <stdint.h>

int spi_init(const char *dev, mode_t mode);
void spi_close();
uint8_t spi_read_reg(uint8_t reg);
void spi_write_reg(uint8_t reg, uint8_t value);

#endif
