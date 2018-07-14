#ifndef SPI_H
#define SPI_H

#include <fcntl.h>
#include <stdint.h>

int spi_init(const char *dev, mode_t mode);
void spi_close(int spi);
uint8_t spi_read_reg(int fd, uint8_t reg);
int spi_write_reg(int fd, uint8_t reg, uint8_t value);

#endif
