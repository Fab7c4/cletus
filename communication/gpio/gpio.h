#ifndef GPIO_H
#define GPIO_H

/****************************************************************
 * Constants
 ****************************************************************/

#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define POLL_TIMEOUT (3 * 1000) /* 3 seconds */
#define MAX_BUF 64

typedef int GPIO_PIN_DIRECTION;
typedef int GPIO_PIN_VALUE;

enum GPIO_PIN_DIRECTION{
    INPUT_PIN=0,
    OUTPUT_PIN=1
};

enum GPIO_PIN_VALUE{
    LOW=0,
    HIGH=1
};

/****************************************************************
 * gpio_export
 ****************************************************************/
int gpio_export(unsigned int gpio);
int gpio_unexport(unsigned int gpio);
int gpio_set_dir(unsigned int gpio, GPIO_PIN_DIRECTION out_flag);
int gpio_set_value(unsigned int gpio, GPIO_PIN_VALUE value);
int gpio_get_value(unsigned int gpio, unsigned int *value);
int gpio_set_edge(unsigned int gpio, char *edge);
int gpio_fd_open(unsigned int gpio);
int gpio_fd_close(int fd);


#endif // GPIO_H
