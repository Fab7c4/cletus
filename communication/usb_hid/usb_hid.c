#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

//#if defined(OS_LINUX) || defined(OS_MACOSX)
//#include <sys/ioctl.h>
//#include <termios.h>
//#elif defined(OS_WINDOWS)
//#include <conio.h>
//#endif
#include <sys/ioctl.h>
#include <termios.h>
#include <errno.h>
#include <assert.h>
#include "usb_hid.h"


//HID specific
#define MAX_PACKET_LENGTH 64
#define DEFAULT_USAGE_PAGE 0xFFAB
#define DEFAULT_USAGE 0x0200

static void pabort(const char *s)
{
    perror(s);
    abort();
}

usb_hid_device_t* usb_hid_init(const int vid, const int pid){
    usb_hid_device_t* device = malloc(sizeof(usb_hid_device_t));
    if (device == NULL)
        pabort("Error allocating memory for usb_hid device.");
    device->buffer = malloc(MAX_PACKET_LENGTH);
    if (device->buffer == NULL)
        pabort("Error allocating memory for usb_hid buffer.");
    int ret = rawhid_open(1,vid,pid,DEFAULT_USAGE_PAGE,DEFAULT_USAGE);
    if (ret <= 0)
        pabort("Error opening hid device with VID and PID.");
    else
        device->device = ret;
    return device;
}


void usb_hid_close(usb_hid_device_t* device)
{
    rawhid_close(device->device);
    free(device->buffer);
    free(device);
}

int usb_hid_receive_packet(usb_hid_device_t* device, uint8_t* buffer, uint16_t length, uint16_t timeout_ms)
{
    assert(length <= MAX_PACKET_LENGTH);
    int num = rawhid_recv(device->device, buffer, length, timeout_ms);
    if (num < 0) {
        printf("\nerror reading, device went offline\n");
        usb_hid_close(device);
        return -1;
    }
    return num;
}

int usb_hid_send_packet(usb_hid_device_t* device, uint8_t* buffer, uint16_t length, uint16_t timeout_ms)
{
    assert(length <= MAX_PACKET_LENGTH);
    int num = rawhid_recv(device->device, buffer, length, timeout_ms);
    if (num < 0) {
        printf("\nerror sending, device went offline\n");
        usb_hid_close(device);
        return -1;
    }
    return num;
}





