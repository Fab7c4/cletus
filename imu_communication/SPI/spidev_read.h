// Copyright 2015, University of Freiburg
// Systems Theory Lab
// Author: Elias Rosch <eliasrosch@googlemail.com>

#ifndef SPI_SPIDEV_READ_H
#define SPI_SPIDEV_READ_H

#include <inttypes.h>
#include<stdint.h>
#include<stdio.h>




#define MAX_BUFFER_SIZE 255

void transfer(uint8_t *tx, uint8_t send_size);
int receive(uint8_t *rx, uint8_t receive_size);
void init_spi(void);
void close_spi(void);
void pabort(const char *s);



#endif // SPI_SPIDEV_READ_H
