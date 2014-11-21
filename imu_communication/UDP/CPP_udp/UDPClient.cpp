// Copyright 2014, University of Freiburg
// Systems Theory Lab
// Author: Elias Rosch <eliasrosch@googlemail.com>

#include <sys/time.h>
#include <iostream>
#include "./UDP.h"

// OTHER_IP should be the address of the Groundstation
#define OTHER_IP "10.42.0.1"
#define OTHER_PORT 8080
#define OWN_PORT 8080

int main (int argc, char** argv) {
    // Define number of samples
    int numOfSamples = 200000;
    // Declare and initialize the Sensordevice
    Sensor my_sensor;
    my_sensor.initI2C(ACCEL_GYRO_DEVICE, MAG_DEVICE);
    // Declare and initialize the UDP-Socket
    UDP my_udp;
    my_udp.initUDP(OTHER_IP, OTHER_PORT, OWN_PORT);
    // Variables for time measurement
    timeval startTime, endTime;
    // Start time measure
    gettimeofday(&startTime, 0);
    
    for(int16_t i = 0;i < numOfSamples ; i++) {
        // Do the read operation
        my_sensor.getSensorValues(ACCEL_TYPE);
        // Do one send operation
        printf("compX: %i\n", my_sensor.accel.compX);
        printf("compY: %i\n", my_sensor.accel.compY);
        printf("compZ: %i\n", my_sensor.accel.compZ);
        my_udp.sendUDPstruct(&my_sensor.accel);
        //int16_t value = my_udp.receiveUDP();
        //printf("The received value is: %i\n", value);
        // Measure the interval between 2 samples
        //gettimeofday(&endTime, 0);
        //float neededSeconds = (endTime.tv_sec - startTime.tv_sec) + 0.000001 * (endTime.tv_usec - startTime.tv_usec);

    }
    // Stop time measurement
    gettimeofday(&endTime, 0);
    float neededSeconds = (endTime.tv_sec - startTime.tv_sec)
                  + 0.000001 * (endTime.tv_usec - startTime.tv_usec);
    std::cout << "Number of samples: " << numOfSamples << "\tTime needed: " << neededSeconds << " s" << std::endl;
    std::cout << "Average per sample: " << 1000*neededSeconds/numOfSamples << " ms" << std::endl;
    return 0;
}
