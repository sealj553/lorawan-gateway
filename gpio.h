#ifndef GPIO_H
#define GPIO_H

#include <fcntl.h>

int gpioInit(const char *dev, mode_t mode);
void gpioClose(int fd);
int gpioRead(int fd);
int gpioWrite(int fd, int value);

#endif
