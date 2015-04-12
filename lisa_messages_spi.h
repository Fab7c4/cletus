#ifndef LISA_MESSAGES_SPI_H
#define LISA_MESSAGES_SPI_H


/// Definition of the message format received from LISA/M
/// via SPI communication

#define PACKED __attribute__((__packed__))
#define NUMBER_OF_IMU_DATA_PACKETS 20


typedef struct PACKED{
    uint32_t sequence_number;         // 1
    uint32_t ticks;         // 1
    int32_t acc_x;      // 5
    int32_t acc_y;
    int32_t acc_z;
    int32_t gyro_p;     // 2
    int32_t gyro_q;
    int32_t gyro_r;
    int32_t mag_x;      // 8
    int32_t mag_y;
    int32_t mag_z;
}sensor_data_imu_t ;


typedef struct PACKED{
    uint32_t sequence_number;         // 1
    uint32_t ticks;         // 1
    uint16_t raw;
    uint16_t offset;
    float scaled;        // 11
}sensor_data_airspeed_t ;

typedef struct PACKED{
    uint32_t sequence_number;         // 1
    uint32_t ticks;         // 1
    sensor_data_imu_t imu[NUMBER_OF_IMU_DATA_PACKETS];
    sensor_data_airspeed_t airspeed;
    uint8_t checksum1;     // 14
    uint8_t checksum2;     // 15
}sensor_data_t ;






#endif // LISA_MESSAGES_SPI
