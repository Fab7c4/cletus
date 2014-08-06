/* Copyright 2014 Greg Horn <gregmainland@gmail.com>
 *
 * This file is hereby placed in the public domain, or, if your legal
 * system does not recognize such a concept, you may consider it
 * licensed under BSD 3.0.  Use it for good.
 */

#ifndef __SENSORS_H__
#define __SENSORS_H__
#include "./structures.h"
#include "./protos_c/messages.pb-c.h"

#define CBSIZE 1024 * 16
#define OUTPUT_BUFFER 36
#define MAX_STREAM_SIZE 255
#define MAX_OUTPUT_STREAM_SIZE 36
#define INPUT_BUFFER_SIZE 255
#define RT_PRIORITY 48




void get_sensors(lisa_messages_t * const y);
int get_lisa_data(lisa_messages_t * const data, uint8_t input_buffer[]);
void xyz_convert_to_double(const xyz_int* const source, xyz_double * dest, double coef);
void quat_convert_to_double(const quaternion_t* const source, quaternion_double_t *dest, double coef);
void raw_to_protobuf(const xyz_int *const source, Xyz *dest);
void scaled_to_protobuf(const xyz_int *const source, Xyz *dest, double coef);
void copy_timestamp(const timestamp_t *const source, Timestamp *dest);





#endif // __SENSORS_H__
