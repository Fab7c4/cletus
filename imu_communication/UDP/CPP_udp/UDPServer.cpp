// Copyright 2014, University of Freiburg
// Systems Theory Lab
// Author: Elias Rosch <eliasrosch@googlemail.com>

#include <sys/time.h>
#include <iostream>
#include "./UDP.h"

// OTHER_IP should be the address of the BeagleBone Black
#define OTHER_IP "10.42.0.42"
#define OTHER_PORT 8080
#define OWN_PORT 8080

int main (int argc, char** argv) {
    // Define number of samples
    int numOfSamples = 200000;
    // Declare and initialize the Sensordevice
    UDP my_udp;
    my_udp.initUDP(OTHER_IP, OTHER_PORT, OWN_PORT);
    // Define file which is written to
    FILE * accelFile;
    accelFile = fopen("accel.txt","w+");
    FILE * gyroFile;
    gyroFile = fopen("gyro.txt","w+");

    // Variables for time measurement
    timeval startTime, endTime;
    float neededSeconds;
    // Start time measure
    gettimeofday(&startTime, 0); 
    // Declare structpointer to the received values
    SensorValues *my_values;
    for(int i = 0;i < numOfSamples ; ) {
        // Do one receive operation
        my_values = my_udp.receiveUDPstruct();
        // Get the time when value arrived
        gettimeofday(&endTime, 0);
        neededSeconds = (endTime.tv_sec - startTime.tv_sec)
                              + 0.000001 * (endTime.tv_usec - startTime.tv_usec);
        // Overwrite the value in the file
        if (my_values->type == 1) {
            rewind(accelFile);
            fprintf(accelFile,"%f %i %i %i\n", neededSeconds, my_values->compX, my_values->compY, my_values->compZ);
        } else if (my_values->type == 2) {
            rewind(gyroFile);
            fprintf(gyroFile,"%f %i %i %i\n", neededSeconds, my_values->compX, my_values->compY, my_values->compZ);
        }

    }
    return 0;
}
