#ifndef ARDUINO_H
#define ARDUINO_H

#include <sys/epoll.h>
#include "./structures.h"
#include "arduino_messages_spi.h"


typedef struct epoll_event epoll_event_t;


enum LISA_RETURNS{
    OK = 0,
    MORE_DATA = 1,
    COMPLETE = 2,
    ERROR_COMMUNICATION = -1,
    ERROR_CHECKSUM = -2
};


int arduino_init(unsigned char* buffer);
void arduino_close(void);
epoll_event_t* arduino_get_epoll_event(void);
int arduino_get_gpio_fd(void);
int arduino_read_message(void);
sensor_data_t* arduino_get_message_data(void);





#endif // ARDUINO_H
