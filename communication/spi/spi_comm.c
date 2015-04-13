/*
 * SPI-tool for receiving data from LISA
 * Copyright (c) 2015 Elias Rosch <eliasrosch@googlemail.com>, Fabian Girrbach
 *
 */

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include "./spi_comm.h"




///DEFINING DEFAULT OPTIONS FOR SPI DEVICE

/// default mode = 3(1,1) --> Clock Idle = High, Clock Phase = Falling Edge
#define DEFAULT_MODE 3
/// default byte length length
#define DEFAULT_WORD_LENGTH 8
/// default clock speed in Hz
#define DEFAULT_CLOCK_RATE 22000000
// error handling
static int err;




// Transfers data on MISO(rx) and MOSI(tx)
void spi_comm_transfer(spi_device_t* device, uint8_t *tx, uint32_t send_size)
{
    device->spi_transfer->rx_buf = 0;
    device->spi_transfer->len = send_size;
    device->spi_transfer->tx_buf =(unsigned long) tx;
    device->spi_transfer->delay_usecs = 0;


    err = ioctl(device->fd, SPI_IOC_MESSAGE(1), device->spi_transfer);
    if (err < 0)
        pabort("error in transmitting SPI-message.");
    if (err < 1)
        pabort("nothing received.");
    for (uint32_t i = 0; i < send_size; i++) {
        if (!(i % 6))
            puts("");
        printf("%i ", tx[i]);
    }
    puts("");
}

// Receives data on MISO(rx) without sending and returns
int spi_comm_receive(spi_device_t* device, uint8_t *rx, uint32_t receive_size)
{
    memset(rx, 0, receive_size);

    device->spi_transfer->tx_buf =0;
    device->spi_transfer->len = receive_size;
    device->spi_transfer->rx_buf =(unsigned long) rx;
    device->spi_transfer->delay_usecs = 0;


    err = ioctl(device->fd, SPI_IOC_MESSAGE(1), device->spi_transfer);
    if (err < 0)
        pabort("error in transmitting SPI-message.");
    if (err < 1)
        pabort("nothing received.");

    return err;
}


spi_device_t* spi_comm_init(const char *device, bool_t read_only)
{
    spi_device_t* device_ptr = malloc(sizeof(spi_device_t));
    device_ptr->spi_transfer = malloc(sizeof(spi_ioc_transfer_t));

    if (read_only)
        device_ptr->fd=open(device, O_RDONLY);
    else
        device_ptr->fd=open(device, O_RDWR);
    if (device_ptr->fd < 0)
        pabort("can't  open SPI device");

    spi_comm_set_clock_mode(device_ptr, DEFAULT_MODE);

    spi_comm_set_word_length(device_ptr, DEFAULT_WORD_LENGTH);

    spi_comm_set_max_clock_rate(device_ptr, DEFAULT_CLOCK_RATE);

    return device_ptr;

}


void spi_comm_set_clock_mode(spi_device_t* spi, int mode)
{
    // set spi mode
    if (ioctl(spi->fd, SPI_IOC_WR_MODE, &mode) == -1)
        pabort("can't set spi mode");
}

void spi_comm_set_word_length(spi_device_t* spi, int length)
{
    // set number of bits per word
    if (ioctl(spi->fd, SPI_IOC_WR_BITS_PER_WORD, &length) == -1)
        pabort("can't set bits per word");

    spi->spi_transfer->bits_per_word = length;
}

void spi_comm_set_max_clock_rate(spi_device_t* spi, int rate)
{
    // set maximum clockspeed in hz
    if (ioctl(spi->fd, SPI_IOC_WR_MAX_SPEED_HZ, &rate) == -1)
        pabort("can't set max speed hz");

    spi->spi_transfer->speed_hz = rate;

}


void spi_comm_close(spi_device_t *device) {
    free(device->spi_transfer);
    close(device->fd);
    free(device);
}

void pabort(const char *s)
{
    printf("%s",s);
    printf("\n%s\n", strerror(errno));
    abort();
}
