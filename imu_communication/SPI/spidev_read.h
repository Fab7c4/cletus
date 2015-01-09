// Copyright 2015, University of Freiburg
// Systems Theory Lab
// Author: Elias Rosch <eliasrosch@googlemail.com>

#ifndef SPI_SPIDEV_READ_H
#define SPI_SPIDEV_READ_H

#include<errno.h>
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<stdint.h>
#include<getopt.h> 
#include<sys/ioctl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<linux/spi/spidev.h> 
#include<fcntl.h>

#define MAX_BUFFER_SIZE 255

void transfer(uint8_t *tx, uint8_t send_size);
int receive(uint8_t *rx, uint8_t receive_size);
void init_spi(int argc, char *argv[]);
void close_spi();
static void pabort(const char *s);



#endif // SPI_SPIDEV_READ_H
