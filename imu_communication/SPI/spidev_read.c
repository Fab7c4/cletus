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
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#define MAX_BUFFER_SIZE 255

static void pabort(const char *s)
{
	perror(s);
	abort();
}

// spidev1.0 --> SPI0 on BBB
static const char *device = "/dev/spidev1.0";
// mode = 3(1,1) --> Clock Idle = High, Clock Phase = Falling Edge
static uint8_t mode = 3;
// number of bits received once
static uint8_t bits = 8;
// clock speed in Hz
static uint32_t speed = 500000;
static uint16_t delay;

// Transfers data on MISO(rx) and MOSI(tx)
static void transfer(int fd, uint8_t *tx, uint8_t send_size)
{
	int ret;
	uint8_t rx[MAX_BUFFER_SIZE] = {0, };
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
		.len = send_size,
		.delay_usecs = delay,
		.speed_hz = speed,
		.bits_per_word = bits,
	};

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		pabort("can't send spi message");

	for (ret = 0; ret < send_size; ret++) {
		if (!(ret % 6))
			puts("");
		printf("%i ", rx[ret]);
	}
	puts("");
}

// Receives data on MISO(rx) without sending
static void receive(int fd, uint8_t *rx, uint8_t receive_size)
{
	int ret;
    memset(rx, 0, receive_size);
	uint8_t tx[MAX_BUFFER_SIZE] = {0, };
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
		.len = receive_size,
		.delay_usecs = delay,
		.speed_hz = speed,
		.bits_per_word = bits,
	};

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		pabort("can't send spi message");

}

int main(int argc, char *argv[])
{
	int ret = 0;
	int fd;

	//parse_opts(argc, argv);

	fd = open(device, O_RDONLY);
	if (fd < 0)
		pabort("can't open device");

    // set spi mode
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		pabort("can't set spi mode");
	
    // set number of bits per word	
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't set bits per word");
	
    // set maximum clockspeed in hz
    ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't set max speed hz");
	
    //printf("spi mode: %d\n", mode);
	//printf("bits per word: %d\n", bits);
	//printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);
    
	uint8_t rx[MAX_BUFFER_SIZE] = {0, };
	receive(fd, rx, 1);

	for (ret = 0; ret < 1; ret++) {
        printf("%i\n", rx[ret]);
	}

	close(fd);

	return ret;
}
