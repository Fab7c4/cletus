#include <stdlib.h>
#include "data_decoding.h"
#include <string.h>

#ifndef DEBUG 
#define DEBUG 0
#endif

#include <stdio.h>

/********************************
 * PROTOTYPES PRIVATE
 * ******************************/
static DEC_errCode data_to_struct(unsigned char sender,unsigned char stream[], int length); 
void data_write(unsigned char stream[],void *destination, int length);

/********************************
 * GLOBALS
 * ******************************/

//data variable
static Data data;

//pointer to data
static Data* data_ptr;




/********************************
 * FUNCTIONS
 * ******************************/

void init_decoding(){
#if DEBUG > 1
  printf("Entering init_decoding\n");
#endif

  data_ptr = &data;
}



DEC_errCode data_decode(unsigned char stream[])
{
#if DEBUG  > 1
  printf("Entering data_decode\n");
#endif

  uint8_t checksum_1 = 0;
  uint8_t checksum_2 = 0;
  uint8_t length = stream[LENGTH_INDEX];
  uint8_t sender = stream[SENDER_ID_INDEX];

  //check first bit is 0x99
  if(stream[STARTBYTE_INDEX] != 0x99)
    {
      //unknown package !!!
      return DEC_ERR_START_BYTE;
    }

  calculate_checksum(stream,&checksum_1,&checksum_2);

  //checksum1 is voorlaatste byte, checksum2 is last byte
  if(checksum_1 != stream[length-2] || checksum_2 != stream[length-1])
    {
      return DEC_ERR_CHECKSUM;
    }

  return data_to_struct(sender, stream, length);
}

static DEC_errCode data_to_struct(unsigned char sender,unsigned char stream[], int length) // start = 0
{
#if DEBUG  > 1
  printf("Entering data_to_struct %i \n", length);
#endif
  if (length == 0)
    printf("Datapacket is empty \n");

  switch(sender)
    {
    case BETTY: //sender_id of lisa
    case BABY_BETTY:
#if DEBUG  > 1
      printf("Received data packet from LISA of type");
#endif
      switch(stream[MESSAGE_ID_INDEX]) // the message id of the folowing message
        {
        case SVINFO:
#if DEBUG  > 1
          printf("SVINFO\n");
#endif
          data_write(stream, (void *)&data_ptr->lisa_plane.svinfo, sizeof(Svinfo)-1);
          break;
        case SYSMON:
#if DEBUG  > 1
          printf("SYSMON\n");
#endif
          data_write(stream, (void *)&data_ptr->lisa_plane.sys_mon, sizeof(Sys_mon)-1);
          break;
        case AIRSPEED_ETS:
#if DEBUG  > 1
          printf("AIRSPEED_ETS\n");
#endif
          data_write(stream, (void *)&data_ptr->lisa_plane.airspeed_ets, sizeof(Airspeed_ets)-1);
          break;
        case ACTUATORS:
#if DEBUG  > 1
          printf("ACTUATORS\n");
#endif
          data_write(stream, (void *)&data_ptr->lisa_plane.actuators, sizeof(Actuators)-1);
          break;
        case GPS_INT:
#if DEBUG  > 1
          printf("GPS_INT\n");
#endif
          data_write(stream, (void *)&data_ptr->lisa_plane.gps_int, sizeof(Gps_int)-1);
          data_write(stream, (void *)&data_ptr->zmq_sensors.gps.pos_data, sizeof(xyz_int));
          memcpy(&(data_ptr->zmq_sensors.gps.vel_data),&stream[7*sizeof(int32_t)],sizeof(xyz_int));
          memcpy(&(data_ptr->zmq_sensors.gps.timestamp),&stream[16*sizeof(int32_t)],sizeof(timestamp_t)) ;
          break;
        case IMU_GYRO_RAW:
#if DEBUG  > 1
          printf("IMU_GYRO_RAW\n");
#endif
          data_write(stream, (void *)&data_ptr->lisa_plane.imu_gyro_raw, sizeof(Imu_gyro_raw)-1);
          //          data_ptr->zmq_sensors.gyro.data.x = data_ptr->lisa_plane.imu_gyro_raw.gp;
          //          data_ptr->zmq_sensors.gyro.data.y = data_ptr->lisa_plane.imu_gyro_raw.gq;
          //          data_ptr->zmq_sensors.gyro.data.z = data_ptr->lisa_plane.imu_gyro_raw.gr;
          //          data_ptr->zmq_sensors.gyro.updated = 1;
          //          memcpy(&(data_ptr->zmq_sensors.timestamp),&(data_ptr->lisa_plane.imu_gyro_raw.tv),sizeof(timeval)) ;
          break;

        case IMU_ACCEL_RAW:
#if DEBUG  > 1
          printf("IMU_ACCEL_RAW\n");
#endif
          data_write(stream, (void *)&data_ptr->lisa_plane.imu_accel_raw, sizeof(Imu_accel_raw)-1);
          //          data_ptr->zmq_sensors.accel.data.x = data_ptr->lisa_plane.imu_accel_raw.ax;
          //          data_ptr->zmq_sensors.accel.data.y = data_ptr->lisa_plane.imu_accel_raw.ay;
          //          data_ptr->zmq_sensors.accel.data.z = data_ptr->lisa_plane.imu_accel_raw.az;
          //          data_ptr->zmq_sensors.accel.updated = 1;
          //          memcpy(&(data_ptr->zmq_sensors.timestamp),&(data_ptr->lisa_plane.imu_accel_raw.tv),sizeof(timeval)) ;
          break;

        case IMU_MAG_RAW:
#if DEBUG  > 1
          printf("IMU_MAG_RAW\n");
#endif
          data_write(stream, (void *)&data_ptr->lisa_plane.imu_mag_raw, sizeof(Imu_mag_raw)-1);
          break;

        case IMU_GYRO:
#if DEBUG  > 1
          printf("IMU_GYRO\n");
#endif
          data_write(stream, (void *)&data_ptr->lisa_plane.imu_gyro, sizeof(Imu_gyro)-1);
          //          data_ptr->zmq_sensors.gyro.data.x = data_ptr->lisa_plane.imu_gyro.gp;
          //          data_ptr->zmq_sensors.gyro.data.y = data_ptr->lisa_plane.imu_gyro.gq;
          //          data_ptr->zmq_sensors.gyro.data.z = data_ptr->lisa_plane.imu_gyro.gr;
          //          data_ptr->zmq_sensors.gyro.updated = 1;
          //          memcpy(&(data_ptr->zmq_sensors.timestamp),&(data_ptr->lisa_plane.imu_gyro_raw.tv),sizeof(timeval)) ;
          break;

        case IMU_ACCEL:
#if DEBUG  > 1
          printf("IMU_ACCELn");
#endif
          data_write(stream, (void *)&data_ptr->lisa_plane.imu_accel, sizeof(Imu_accel)-1);
          //          data_ptr->zmq_sensors.accel.data.x = data_ptr->lisa_plane.imu_accel.ax;
          //          data_ptr->zmq_sensors.accel.data.y = data_ptr->lisa_plane.imu_accel.ay;
          //          data_ptr->zmq_sensors.accel.data.z = data_ptr->lisa_plane.imu_accel.az;
          //          data_ptr->zmq_sensors.accel.updated = 1;
          //          memcpy(&(data_ptr->zmq_sensors.timestamp),&(data_ptr->lisa_plane.imu_accel_raw.tv),sizeof(timeval)) ;
          break;
        case IMU_MAG_SCALED:
#if DEBUG  > 1
          printf("IMU_MAG_RAW\n");
#endif
          data_write(stream, (void *)&data_ptr->lisa_plane.imu_mag, sizeof(Imu_mag_raw)-1);
          data_write(stream, (void *)&data_ptr->zmq_sensors.imu.imu_mag, sizeof(mag_raw_t));
          break;

        case IMU_GYRO_SCALED:
#if DEBUG  > 1
          printf("IMU_GYRO\n");
#endif
          data_write(stream, (void *)&data_ptr->lisa_plane.imu_gyro, sizeof(Imu_gyro_raw)-1);
          data_write(stream, (void *)&data_ptr->zmq_sensors.imu.imu_gyro, sizeof(gyro_raw_t));
          break;

        case IMU_ACC_SCALED:
#if DEBUG  > 1
          printf("IMU_ACCELn");
#endif
          data_write(stream, (void *)&data_ptr->lisa_plane.imu_accel, sizeof(Imu_accel_raw)-1);
          data_write(stream, (void *)&data_ptr->zmq_sensors.imu.imu_accel, sizeof(accel_raw_t));
          break;

        case AHRS_QUAT_INT:
#if DEBUG  > 1
          printf("AHRS_QUAT_INTn");
#endif
          data_write(stream, (void *)&data_ptr->zmq_sensors.ahrs, sizeof(ahrs_t));
          break;

        case ROTORCRAFT_RADIO_CONTROL:
#if DEBUG  > 1
          printf("Radio Control\n");
#endif
          data_write(stream, (void *)&data_ptr->zmq_sensors.rc, sizeof(rc_t));
          break;



        case UART_ERRORS:
#if DEBUG  > 1
          printf("UART_ERRORS\n");
#endif
          data_write(stream, (void *)&data_ptr->lisa_plane.uart_errors, sizeof(UART_errors)-1);
          break;

        case BARO_RAW:
#if DEBUG  > 1
          printf("BARO_RAW\n");
#endif
          data_write(stream, (void *)&data_ptr->lisa_plane.baro_raw, sizeof(Baro_raw)-1);
          break;
        default:
          printf("UNKNOWN ID %i\n",stream[MESSAGE_ID_INDEX] );
          return DEC_ERR_UNKNOWN_LISA_PACKAGE;break;
        }
      break;
    default:
      printf("UNKNOWN Sender %i\n",stream[MESSAGE_ID_INDEX] );
      return DEC_ERR_UNKNOWN_SENDER; break;
    }
  return DEC_ERR_NONE;
}

void data_write(unsigned char stream[],void *destination, int length)
{
#if DEBUG  > 1
  printf("Entering data_write\n");
#endif

  memcpy(destination,&(stream[MESSAGE_START_INDEX]),length);
}




DEC_errCode data_encode(unsigned char message[],long unsigned int message_length,unsigned char encoded_data[],int sender_id,int message_id)
{
#if DEBUG  > 1
  printf("Entering data_encode\n");
#endif

  uint8_t checksum_1 = 0;
  uint8_t checksum_2 = 0;
  uint8_t length = message_length+6+16; //message length + 6 info bytes + 16 timestamp bytes
  timeval timestamp;
  TimestampBeagle timestampBeagle;

  encoded_data[STARTBYTE_INDEX] = 0x99;
  encoded_data[LENGTH_INDEX] = length;
  encoded_data[SENDER_ID_INDEX] = sender_id; // sender id of server
  encoded_data[MESSAGE_ID_INDEX] = message_id; // message id

  //add message
  memcpy(&(encoded_data[MESSAGE_START_INDEX]),(void *)message,message_length);

  //get localtime
  gettimeofday(&timestamp, NULL);

  //convert beaglebone 8 byte timeval to 16 byte timeval for server, if this is server this does not change anything
  timestampBeagle.tv.tv_sec=(uint64_t)timestamp.tv_sec;
  timestampBeagle.tv.tv_usec=(uint64_t)timestamp.tv_usec;

  //add timestamp to buffer
  memcpy(&(encoded_data[message_length+4]),(void *)&timestampBeagle.tv,sizeof(timestampBeagle.tv));

  calculate_checksum(encoded_data,&checksum_1,&checksum_2);

  encoded_data[length-2] = checksum_1;
  encoded_data[length-1] = checksum_2;

  return DEC_ERR_NONE;
}



void calculate_checksum(uint8_t buffer[],uint8_t *checksum_1,uint8_t *checksum_2){
#if DEBUG  > 1
  printf("Entering calculate_checksum\n");
#endif
  int length = buffer[LENGTH_INDEX];
  *checksum_1=0;
  *checksum_2=0;

  //start byte '0x99' is not in checksum
  for (int i=1;i<length-2;i++)
    {
      *checksum_1 += buffer[i];
      *checksum_2 += *checksum_1;
    }
}

int add_timestamp(uint8_t buffer[]){
#if DEBUG  > 1
  printf("Entering add_timestamp\n");
#endif

  int length_original=buffer[1];
  uint8_t checksum_1,checksum_2;
  int new_length=length_original+16; //timeval is 16 bytes
  struct timeval tv_8;	//beaglebones timestamp is 8 byte, server timestamp is 16 byte :-(
  TimestampBeagle timestampBeagle;

  //get localtime
  gettimeofday(&tv_8, NULL);

  //convert beaglebone 8 byte timeval to 16 byte timeval for server, if this is server this does not change anything
  timestampBeagle.tv.tv_sec=(uint64_t)tv_8.tv_sec;
  timestampBeagle.tv.tv_usec=(uint64_t)tv_8.tv_usec;

  //update message length
  buffer[LENGTH_INDEX]=new_length;

  //add timestamp to buffer
  memcpy(&(buffer[length_original-2]),(void *)&timestampBeagle.tv,sizeof(timestampBeagle.tv)); //overwrite previous checksums (-2)

  //recalculate checksum
  calculate_checksum(buffer,&checksum_1,&checksum_2);
  buffer[new_length-2]=checksum_1;
  buffer[new_length-1]=checksum_2;

  return new_length;
}

int strip_timestamp(uint8_t buffer[]){
#if DEBUG  > 1
  printf("Entering strip_timestamp\n");
#endif

  int length=buffer[LENGTH_INDEX];
  uint8_t checksum_1,checksum_2;
  int new_length=length-16; //timeval is 16 bytes

  //update message length
  buffer[LENGTH_INDEX]=new_length;

  //recalculate checksum
  calculate_checksum(buffer,&checksum_1,&checksum_2);
  buffer[new_length-2]=checksum_1;
  buffer[new_length-1]=checksum_2;

  return new_length;
}

void DEC_err_handler(DEC_errCode err,void (*write_error_ptr)(char *,char *,int))  
{
#if DEBUG  > 1
  printf("Entering DEC_err_handler\n");
#endif

  static char SOURCEFILE[] = "data_decoding.c";
  //write error to local log
  switch( err ) {
    case DEC_ERR_NONE:
      break;
    case  DEC_ERR_START_BYTE:
      write_error_ptr(SOURCEFILE,"start byte is not 0x99",err);
      break;
    case DEC_ERR_CHECKSUM:
      write_error_ptr(SOURCEFILE,"wrong checksum",err);
      break;
    case DEC_ERR_UNKNOWN_BONE_PACKAGE:
      write_error_ptr(SOURCEFILE,"received unknown package from beaglebone",err);
      break;
    case DEC_ERR_UNKNOWN_LISA_PACKAGE:
      write_error_ptr(SOURCEFILE,"received unknown package from lisa",err);
      break;
    case DEC_ERR_UNKNOWN_WIND_PACKAGE:
      write_error_ptr(SOURCEFILE,"received unknown package from wind bone",err);
      break;
    case DEC_ERR_UNKNOWN_SENDER:
      write_error_ptr(SOURCEFILE,"received package from unknown sender",err);
      break;
    case DEC_ERR_LENGTH:
      write_error_ptr(SOURCEFILE,"decoded not entire package length",err);
      break;
    case DEC_ERR_UNDEFINED:
      write_error_ptr(SOURCEFILE,"undefined decoding error",err);
      break;
    default: break;
    }
}

void get_new_sensor_struct(lisa_messages_t * const data_struct)
{
  memcpy(data_struct,&(data_ptr->zmq_sensors), sizeof(lisa_messages_t));
}

uint8_t set_actuators(actuators_t* const message, unsigned char encoded_data[])
{
  uint8_t checksum_1 = 0;
  uint8_t checksum_2 = 0;
  uint8_t length = sizeof(actuators_t)+6; //message length + 6 info bytes + 16 timestamp bytes

  encoded_data[STARTBYTE_INDEX] = 0x99;
  encoded_data[LENGTH_INDEX] = length;
  encoded_data[SENDER_ID_INDEX] = BONE_PLANE; // sender id of server
  encoded_data[MESSAGE_ID_INDEX] = SERVO_COMMANDS; // message id

  //add message
  memcpy(&(encoded_data[MESSAGE_START_INDEX]),(void *)message,sizeof(actuators_t));

  calculate_checksum(encoded_data,&checksum_1,&checksum_2);

  encoded_data[length-2] = checksum_1;
  encoded_data[length-1] = checksum_2;

  return length;

}






