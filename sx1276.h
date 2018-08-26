#ifndef SX1276_H
#define SX1276_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

void set_frequency(uint32_t frequency);
void sleep_mode(void);
uint8_t get_version(void);
bool read_data(uint8_t *payload, uint8_t *p_length);
size_t write_data(const char *buffer, int size);
void set_sync_word(uint8_t sw);
void idle_mode(void);

#endif
