#ifndef ADS1299_H
#define ADS1299_H

#include <stdint.h>

void ads1299_init(void);
uint8_t ads1299_read_id(void);

#endif