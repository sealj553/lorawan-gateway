#ifndef SPI_H
#define SPI_H

#include <fcntl.h>
#include <stdint.h>

void spi_init(const char *dev, mode_t mode);
void spi_close(void);
uint8_t spi_read_reg(uint8_t reg);
int spi_write_reg(uint8_t reg, uint8_t value);

#endif
