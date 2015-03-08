#ifndef LISA_H
#define LISA_H

#include <sys/epoll.h>
#include "./structures.h"

typedef struct epoll_event epoll_event_t;


enum LISA_RETURNS{
    OK = 0,
    MORE_DATA = 1,
    COMPLETE = 2,
    ERROR_COMMUNICATION = -1,
    ERROR_CHECKSUM = -2
};


int lisa_init(unsigned char* buffer);
void lisa_close(void);
epoll_event_t* lisa_get_epoll_event(void);
int lisa_get_gpio_fd(void);
int lisa_read_message(void);
sensors_spi_t* lisa_get_message_data(void);





#endif // LISA_H
