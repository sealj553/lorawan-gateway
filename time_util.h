#ifndef TIME_UTIL_H
#define TIME_UTIL_H

#include <stdint.h>

int seconds(void);
void delay(unsigned int ms);

uint64_t get_time_ms(void);
uint64_t get_time_us(void);
uint64_t get_time_ns(void);

void boot_time(void);
uint64_t get_boot_time_ns(void);
uint32_t uptime_us(void);

#endif
