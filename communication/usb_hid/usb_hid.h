#ifndef _USB_HID_COMMUNICATION_H
#define _USB_HID_COMMUNICATION_H


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <inttypes.h>

#define DEFAULT_ARDUINO_VID 0x16C0
#define DEFAULT_ARDUINO_PID 0x0486
#define DEFAULT_PC_VID 0x16C0
#define DEFAULT_PC_PID 0x0480



#include "hid.h"

typedef struct {
    int device;
    uint8_t* buffer;
} usb_hid_device_t;

usb_hid_device_t* usb_hid_init(const int vid, const int pid);
void usb_hid_close(usb_hid_device_t* device);

int usb_hid_receive_packet(usb_hid_device_t* device, uint8_t* buffer, uint16_t length, uint16_t timeout_ms);
int usb_hid_send_packet(usb_hid_device_t* device, uint8_t* buffer, uint16_t length, uint16_t timeout_ms);




#endif //_USB_HID_COMMUNICATION_H


