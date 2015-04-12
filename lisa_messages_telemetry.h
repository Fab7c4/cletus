#ifndef LISA_MESSAGES_TELEMETRY_H
#define LISA_MESSAGES_TELEMETRY_H

#include <inttypes.h>
#include "./structures.h"

/********************************
 * GLOBALS
 * ******************************/
enum lisa_msg_info{
    BYTES_CHECKSUM = 2, //Number of bytes used for the checksum
    BYTES_HEADER =  2, //Number of Bytes until message  (length, senderid)
    SENDER_ID = 166, //Senser ID of the Lisa set with paparazzi
    LISA_STARTBYTE = 0x99,
    WINDSENSOR_STARTBYTE =0x24,
    LISA_INDEX_MSG_ID = 3,
    LISA_INDEX_MSG_LENGTH = 1,
    LISA_INDEX_SENDER_ID = 2,
    LISA_MAX_MSG_LENGTH = 64
};


//message ids
enum Message_id{
  SYSMON = 33,
  UART_ERRORS = 208,
  ACTUATORS = 105,
  SVINFO=25,
  AIRSPEED_ETS = 57,
  GPS_INT=155,
  IMU_GYRO_SCALED=131,
  IMU_ACCEL_SCALED=132,
  IMU_MAG_SCALED=133,
  IMU_ALL_SCALED=138,
  BARO_RAW = 221,
  IMU_GYRO_RAW = 203,
  IMU_ACCEL_RAW = 204,
  IMU_MAG_RAW = 205,
  IMU_GYRO = 200,
  IMU_ACCEL = 202,
  IMU_MAG = 201,
  SERVO_COMMANDS = 72,
  AHRS_QUAT_INT = 157,
  ROTORCRAFT_RADIO_CONTROL = 160
};


typedef struct __attribute__((packed)){ // id = 57
  uint16_t adc;
  uint16_t offset;
  float scaled;
} MSG_Airspeed_ets;



typedef struct __attribute__((packed)){ // id = 133
  int32_t mx;
  int32_t my;
  int32_t mz;
} MSG_Mag_scaled;


typedef struct __attribute__((packed)){ // id = 131
  int32_t gx;
  int32_t gy;
  int32_t gz;
} MSG_Gyro_scaled;

typedef struct __attribute__((packed)){ // id = 133
  int32_t ax;
  int32_t ay;
  int32_t az;
} MSG_Accel_scaled;




#endif // LISA_MESSAGES_TELEMETRY_H
