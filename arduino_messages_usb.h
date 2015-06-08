#ifndef ARDUINO_MESSAGES_SPI_H
#define ARDUINO_MESSAGES_SPI_H


/// Definition of the message format received from Arduino
/// via SPI communication

#define PACKED __attribute__((__packed__))
#define NUMBER_OF_IMU_DATA_PACKETS 2

///********************************************************************
/// GENERAL SUBMESSAGE DEFINITIONS
///********************************************************************

/// Footer definition
typedef struct PACKED {
  uint32_t crc32;                  // CRC32
}
sensor_data_footer_t;

/// Header definition
typedef struct PACKED{
    uint32_t sequence_number;  // This number is incremented with each new header
    uint32_t seconds; // Seconds elapsed since start of device
    uint32_t ticks; // Ticks elapsed since last second
} sensor_data_header_t;

/// Datatype for VECTOR
typedef struct PACKED {
  int32_t x;
  int32_t y;
  int32_t z;
}
sensor_data_xyz_t;

/// Datatype for RATES
typedef struct PACKED {
  int32_t p;
  int32_t q;
  int32_t r;
}
sensor_data_pqr_t;


///********************************************************************
/// SENSOR MESSAGE DEFINITIONS
///********************************************************************

typedef struct PACKED{
  sensor_data_header_t header;
  sensor_data_xyz_t accel;
  sensor_data_pqr_t gyro;
  sensor_data_xyz_t mag;
}
sensor_data_imu_t ;


typedef struct PACKED {
  sensor_data_header_t header;
  uint16_t raw;
  uint16_t offset;
  uint32_t scaled;
}
sensor_data_airspeed_t ;

typedef struct PACKED {
  sensor_data_header_t header;
  uint16_t raw_angle;
}
sensor_data_lineangle_t ;


///********************************************************************
/// COMPLETE MESSAGE DEFINITION
///********************************************************************

typedef struct PACKED {
  sensor_data_imu_t imu;
  sensor_data_lineangle_t lineangle;
}
sensor_data_t ;

///********************************************************************
/// Actuator MESSAGE DEFINITIONS
///********************************************************************
typedef struct PACKED {
    sensor_data_header_t header;
    sensor_data_header_t set_time;
    uint32_t flaps_right;
    uint32_t flaps_left;
    uint32_t ailerons_right;
    uint32_t ailerons_left;
    uint32_t rudder;
    uint32_t elevator;
} sensor_data_actuators_t ;

#endif // MESSAGEDEFINITIONS_H
