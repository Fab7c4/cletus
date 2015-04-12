#include "lisa.h"
#include "lisa_messages_spi.h"
#include "./communication/spi/spi_comm.h"
#include "./communication/gpio/gpio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>


#define DEFAULT_LISA_GPIO_PIN 14


static int lisa_spi_init(void);
static int lisa_gpio_init(void);
static int lisa_check_message_checksum(uint8_t* buffer, uint16_t length);



typedef struct {
    int n_read;
    int n_failed;
} lisa_state_t;

typedef struct {
    int fd;
    epoll_event_t* event_ptr;
} gpio_t;


typedef struct {
    lisa_state_t* state;
    spi_device_t* spi;
    gpio_t* gpio;
    unsigned char* buffer;
} lisa_t;


lisa_t lisa;

int lisa_init(unsigned char* buffer)
{
    int retval;
    lisa.state = malloc(sizeof(lisa_state_t));

    retval = lisa_spi_init();
    retval = lisa_gpio_init();

    lisa.buffer = buffer;
    if (lisa.buffer == NULL)
    {
        printf("Error setting buffer for lisa!\n");
        return -1;
    }
    return retval;

}

static int lisa_spi_init(void)
{
    lisa.spi = spi_comm_init(SPI_COMM_DEVICE_SPI1, 1);
    if (lisa.spi == NULL)
    {
        printf("Error initializing SPI port for lisa!\n");
        return -1;
    }
    return 1;
}

static int lisa_gpio_init(void)
{
    int retval;
    lisa.gpio = malloc(sizeof(gpio_t));
    retval = gpio_export(DEFAULT_LISA_GPIO_PIN);
    if (retval < 0 )
    {
        printf("Error exprting GPIO pin %i for lisa!\n", DEFAULT_LISA_GPIO_PIN);
        return -1;
    }
    retval = gpio_set_dir(DEFAULT_LISA_GPIO_PIN, INPUT_PIN);
    if (retval < 0 )
    {
        printf("Error setting direction for GPIO pin %i for lisa!\n", DEFAULT_LISA_GPIO_PIN);
        return -1;
    }
    retval = gpio_set_edge(DEFAULT_LISA_GPIO_PIN, "rising");
    if (retval < 0 )
    {
        printf("Error setting edge for GPIO pin %i for lisa!\n", DEFAULT_LISA_GPIO_PIN);
        return -1;
    }
    lisa.gpio->fd = gpio_fd_open(DEFAULT_LISA_GPIO_PIN);
    if (retval < 0 )
    {
        printf("Error opening file descriptor for GPIO pin %i for lisa!\n", DEFAULT_LISA_GPIO_PIN);
        return -1;
    }
    return retval;
}

int lisa_get_gpio_fd(void)
{
    return lisa.gpio->fd;
}

epoll_event_t* lisa_get_epoll_event(void)
{
    epoll_event_t* event = malloc(sizeof(epoll_event_t));
    if (event  == NULL )
    {
        printf("Error creating epoll event for GPIO pin %i for lisa!\n", DEFAULT_LISA_GPIO_PIN);
        return NULL;
    }
    event->events = EPOLLIN | EPOLLET;
    event->data.fd = lisa.gpio->fd;
    lisa.gpio->event_ptr = event;
    return lisa.gpio->event_ptr;
}

void lisa_close(void)
{
    spi_comm_close(lisa.spi);
    gpio_fd_close(lisa.gpio->fd);
    gpio_unexport(DEFAULT_LISA_GPIO_PIN);
    free(lisa.gpio);
    free(lisa.state);

}


int lisa_read_message(void)
{
    int ret;
    ret = spi_comm_receive(lisa.spi, lisa.buffer, sizeof(sensors_spi_t));
    if (ret < 0)
    {
        lisa.state->n_failed++;
        printf("Error receiving data from SPI port!\n");
	return ERROR_COMMUNICATION;
    }
    ret = lisa_check_message_checksum(lisa.buffer,sizeof(sensors_spi_t));
    if (ret < 0)
    {
        lisa.state->n_failed++;
        for (unsigned int i = 0; i < sizeof(sensors_spi_t); i++)
	{
		printf(" %i ", lisa.buffer[i]);
	}
	printf("\n");

        printf("Error prooving checksum for message!\n");

        return ERROR_CHECKSUM;
    }
	printf("Checksum OK \n");
    return COMPLETE;
}

sensors_spi_t* lisa_get_message_data(void)
{
    return (sensors_spi_t*) lisa.buffer;
}


static int lisa_check_message_checksum(uint8_t* buffer, uint16_t length)
{
    uint8_t c1 =0;
    uint8_t c2 =0;

    for (int i = 0 ; i < length-2 ; i++)
    {
        c1 += buffer[i];
        c2 += c1;
    }
    sensors_spi_t* data = (sensors_spi_t*) buffer;
    if (data->checksums.checksum1 == c1 && data->checksums.checksum2 == c2)
        return 1;
    else
        return ERROR_CHECKSUM;
}
