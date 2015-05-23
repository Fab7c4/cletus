// Copyright 2015, University of Freiburg
// Systems Theory Lab
// Author: Elias Rosch <eliasrosch@googlemail.com>, Fabian Girrbach

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
#include <inttypes.h>

#define MAX_BUFFER_SIZE 255
#define bool_t int

/// Available SPI devices on BBB
// default device :spidev1.0 --> SPI0 on BBB
#define SPI_COMM_DEVICE_SPI1 "/dev/spidev1.0"
#define SPI_COMM_DEVICE_SPI2 "/dev/spidev2.0"

typedef struct spi_ioc_transfer spi_ioc_transfer_t;


typedef struct {
    spi_ioc_transfer_t* spi_transfer;
    int fd;
}spi_device_t;


int spi_comm_receive(spi_device_t* device, uint8_t *rx, uint32_t receive_size);
void spi_comm_transfer(spi_device_t* device, uint8_t *tx, uint32_t send_size);
spi_device_t* spi_comm_init(const char *device, bool_t read_only);
void spi_comm_close(spi_device_t* device);


void spi_comm_set_clock_mode(spi_device_t* spi, int mode);
void spi_comm_set_word_length(spi_device_t* spi, int length);
void spi_comm_set_max_clock_rate(spi_device_t* spi, int rate);


void pabort(const char *s);



#endif // SPI_SPIDEV_READ_H
