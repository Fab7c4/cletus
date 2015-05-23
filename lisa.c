#include "lisa.h"
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
static uint32_t Crc32Fast(uint32_t crc, uint32_t data);




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
    ret = spi_comm_receive(lisa.spi, lisa.buffer, sizeof(sensor_data_t));
    if (ret < 0)
    {
        lisa.state->n_failed++;
        printf("Error receiving data from SPI port!\n");
        return ERROR_COMMUNICATION;
    }

    ret = lisa_check_message_checksum(lisa.buffer,sizeof(sensor_data_t)-4);
    if (ret < 0)
    {
        lisa.state->n_failed++;
        for (unsigned int i = 0; i < sizeof(sensor_data_t); i++)
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

sensor_data_t* lisa_get_message_data(void)
{
    return (sensor_data_t*) lisa.buffer;
}


static int lisa_check_message_checksum(uint8_t* buffer, uint16_t length)
{
    uint32_t currentData= 0;
    uint32_t crc32 = 0xFFFFFFFF;
    uint16_t i;
    for ( i = 0 ; i < length ; i+=4)
    {
        currentData =   (*(uint8_t *)(buffer + i)) |
                        (*(uint8_t *)(buffer + i + 1)) << 8 |
                        (*(uint8_t *)(buffer + i + 2)) << 16 |
                        (*(uint8_t *)(buffer + i + 3)) << 24;
        crc32 = Crc32Fast(crc32, currentData);
        //	printf("Current crc 0x%08x for data 0x%08x and loop idx %d \n", crc32, currentData, i);
    }
    currentData = 0;
    /* remaining bytes */
    //printf("Modulo operation result %d\n", length%4);
    switch (length % 4) {
    case 1:
        currentData = *(uint8_t *)(buffer + i);
        crc32 = Crc32Fast(crc32, currentData);
        break;
    case 2:
        currentData =   (*(uint8_t *)(buffer + i)) |
                        (*(uint8_t *)(buffer + i + 1)) << 8;
        crc32 = Crc32Fast(crc32, currentData);
        break;
    case 3:
        currentData =   (*(uint8_t *)(buffer + i)) |
                        (*(uint8_t *)(buffer + i + 1)) << 8 |
                        (*(uint8_t *)(buffer + i + 2)) << 16;
        crc32 = Crc32Fast(crc32, currentData);
        break;
    default:
        break;
    }

    sensor_data_t* data = (sensor_data_t*) buffer;
    if (data->footer.crc32 == crc32)
        return 1;
    else
    {
        printf("Crc of message is 0x%08x and calculated crc is 0x%08x\n", data->footer.crc32, crc32);
        printf("Crc test is 0x%08x for data 0x%08x", Crc32Fast(0xFFFFFFFF, 0x12345678), 0x12345678);
        return ERROR_CHECKSUM;
    }
}


uint32_t Crc32Fast(uint32_t crc, uint32_t data)
{
    static const uint32_t CrcTable[16] = { // Nibble lookup table for 0x04C11DB7 polynomial
                                           0x00000000,0x04C11DB7,0x09823B6E,0x0D4326D9,0x130476DC,0x17C56B6B,0x1A864DB2,0x1E475005,
                                           0x2608EDB8,0x22C9F00F,0x2F8AD6D6,0x2B4BCB61,0x350C9B64,0x31CD86D3,0x3C8EA00A,0x384FBDBD };

    crc = crc ^ data; // Apply all 32-bits

    // Process 32-bits, 4 at a time, or 8 rounds

    crc = (crc << 4) ^ CrcTable[crc >> 28]; // Assumes 32-bit reg, masking index to 4-bits
    crc = (crc << 4) ^ CrcTable[crc >> 28]; //  0x04C11DB7 Polynomial used in STM32
    crc = (crc << 4) ^ CrcTable[crc >> 28];
    crc = (crc << 4) ^ CrcTable[crc >> 28];
    crc = (crc << 4) ^ CrcTable[crc >> 28];
    crc = (crc << 4) ^ CrcTable[crc >> 28];
    crc = (crc << 4) ^ CrcTable[crc >> 28];
    crc = (crc << 4) ^ CrcTable[crc >> 28];

    return(crc);
}
