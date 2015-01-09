/*
 * SPI-tool for receiving data from LISA
 * Copyright (c) 2015 Elias Rosch <eliasrosch@googlemail.com>
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
#include "./spidev_read.h"

// default device :spidev1.0 --> SPI0 on BBB
static const char *_device = "/dev/spidev1.0";
// default mode = 3(1,1) --> Clock Idle = High, Clock Phase = Falling Edge
static uint8_t _mode = 3;
// default number of bits received once
static uint8_t _bits = 8;
// default clock speed in Hz
static uint32_t _speed = 500000;
static uint16_t _delay;
// spi devicefile number
static int _fd;
// error handling 
static int err;

// Transfers data on MISO(rx) and MOSI(tx)
void transfer(uint8_t *tx, uint8_t send_size)
{
	uint8_t rx[MAX_BUFFER_SIZE] = {0, };
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
		.len = send_size,
		.delay_usecs = _delay,
		.speed_hz = _speed,
		.bits_per_word = _bits,
	};

	err = ioctl(_fd, SPI_IOC_MESSAGE(1), &tr);
	if (err < 0)
		pabort("error in transmitting SPI-message.");
    if (err < 1)
        pabort("nothing received.");
    int i;
	for (i = 0; i < send_size; i++) {
		if (!(i % 6))
			puts("");
		printf("%i ", rx[i]);
	}
	puts("");
}

// Receives data on MISO(rx) without sending and returns 
int receive(uint8_t *rx, uint8_t receive_size)
{
    memset(rx, 0, receive_size);
	uint8_t tx[MAX_BUFFER_SIZE] = {0, };
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
		.len = receive_size,
		.delay_usecs = _delay,
		.speed_hz = _speed,
		.bits_per_word = _bits,
	};
	
    err = ioctl(_fd, SPI_IOC_MESSAGE(1), &tr);
	if (err < 0)
		pabort("error in transmitting SPI-message.");
    if (err < 1)
        pabort("nothing received.");

    return err;
}

void set_mode(uint8_t mode) {
    _mode = mode;
    err = ioctl(_fd, SPI_IOC_WR_MODE, &_mode);
    if (err == -1)
    pabort("can't set spi mode");
}

void set_num_bits(uint8_t bits) {
    _bits = bits;
    if (ioctl(_fd, SPI_IOC_WR_BITS_PER_WORD, &_bits) == -1)
    pabort("can't set bits per word");

}

void set_speed(uint32_t speed) {
    _speed = speed;
    if (ioctl(_fd, SPI_IOC_WR_MAX_SPEED_HZ, &_speed) == -1)
    pabort("can't set max speed hz");

}

void set_device(char *device) {
    _device = device;
    _fd=open(_device, O_RDONLY);
    if (_fd < 0)
    pabort("can't open device");

}


void init_spi(int argc, char *argv[]) {

	_fd=open(_device, O_RDONLY);
    if (_fd < 0)
		pabort("can't open device");

    // set spi mode
    if (ioctl(_fd, SPI_IOC_WR_MODE, &_mode) == -1)
		pabort("can't set spi mode");
	
    // set number of bits per word	
	if (ioctl(_fd, SPI_IOC_WR_BITS_PER_WORD, &_bits) == -1)
		pabort("can't set bits per word");
	
    // set maximum clockspeed in hz
    if (ioctl(_fd, SPI_IOC_WR_MAX_SPEED_HZ, &_speed) == -1)
		pabort("can't set max speed hz");
}

void close_spi() {
	close(_fd);
}

static void pabort(const char *s)
{
	printf("%s",s);
    printf("\n%s\n", strerror(errno));
	abort();
}
