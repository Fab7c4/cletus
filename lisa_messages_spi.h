#ifndef LISA_MESSAGES_SPI_H
#define LISA_MESSAGES_SPI_H


/// Definition of the message format received from LISA/M
/// via SPI communication

#define PACKED __attribute__((__packed__))
#define NUMBER_OF_IMU_DATA_PACKETS 5
#define IMU_ARRAY_SIZE 10



///********************************************************************
/// GENERAL SUBMESSAGE DEFINITIONS
///********************************************************************


/// Footer definition
typedef struct PACKED{
    uint32_t crc32;
} sensor_data_footer_t;

/// Header definition
typedef struct PACKED{
    uint32_t sequence_number;  // This number is incremented with each new header
    uint32_t seconds; // Seconds elapsed since start of device
    uint32_t ticks; // Ticks elapsed since last second
} sensor_data_header_t;

/// Datatype for VECTOR
typedef struct PACKED{
    int32_t x;      // 5
    int32_t y;
    int32_t z;
} sensor_data_xyz_t;

/// Datatype for RATES
typedef struct PACKED{
    int32_t p;
    int32_t q;
    int32_t r;
} sensor_data_pqr_t;


//********************************************************************
/// SENSOR MESSAGE DEFINITIONS
///********************************************************************


typedef struct PACKED{
    sensor_data_header_t header;
    sensor_data_xyz_t accel;
    sensor_data_pqr_t gyro;
    sensor_data_xyz_t mag;
}sensor_data_imu_t ;


typedef struct PACKED{
    sensor_data_header_t header;
    uint16_t raw;
    uint16_t offset;
    float scaled;
}sensor_data_airspeed_t ;

typedef struct PACKED {
  sensor_data_header_t header;
  uint16_t raw_angle;
}
sensor_data_lineangle_t ;


//********************************************************************
/// Actuator MESSAGE DEFINITIONS
///********************************************************************

typedef struct PACKED{
    sensor_data_header_t header;
    sensor_data_header_t set_time;
    uint32_t flaps_right;
    uint32_t flaps_left;
    uint32_t ailerons_right;
    uint32_t ailerons_left;
    uint32_t rudder;
    uint32_t elevator;
}sensor_data_actuators_t ;


//********************************************************************
/// COMPLETE MESSAGE DEFINITION
///********************************************************************

typedef struct PACKED{
    sensor_data_header_t header;
    sensor_data_imu_t imu_plane[NUMBER_OF_IMU_DATA_PACKETS];
    sensor_data_imu_t imu_arm[NUMBER_OF_IMU_DATA_PACKETS];
    sensor_data_lineangle_t lineangle1;
    sensor_data_lineangle_t lineangle2;
    sensor_data_airspeed_t airspeed;
    sensor_data_footer_t footer;
}sensor_data_t ;

typedef struct PACKED{
    uint32_t header;
    sensor_data_footer_t footer;
}test_t ;




#endif // LISA_MESSAGES_SPI
