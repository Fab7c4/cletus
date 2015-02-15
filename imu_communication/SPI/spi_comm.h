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

#define MAX_BUFFER_SIZE 255


/// Available SPI devices on BBB
// default device :spidev1.0 --> SPI0 on BBB
#define DEVICE_SPI1 "/dev/spidev1.0";

typedef struct spi_ioc_transfer spi_ioc_transfer_t;


typedef struct {
    spi_ioc_transfer_t* spi_transfer;
    int fd;
}spi_device_t;


int spi_comm_receive(spi_device_t* device, uint8_t *rx, uint8_t receive_size);
void spi_comm_transfer(spi_device_t* device, uint8_t *tx, uint8_t send_size);
spi_device_t* spi_comm_init(const char *device, bool read_only);
void spi_comm_close(const spi_device_t* device);
static void pabort(const char *s);



#endif // SPI_SPIDEV_READ_H
