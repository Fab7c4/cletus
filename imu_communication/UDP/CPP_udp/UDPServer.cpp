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
    FILE * pFile;
    pFile = fopen("values.txt","w+");

    // Variables for time measurement
    timeval startTime, endTime;
    float neededSeconds;
    // Start time measure
    gettimeofday(&startTime, 0); 
    for(int i = 0;i < numOfSamples ; i++) {
        // Do one receive operation
        SensorValues *my_values = my_udp.receiveUDPstruct();
        gettimeofday(&endTime, 0);
        neededSeconds = (endTime.tv_sec - startTime.tv_sec)
                              + 0.000001 * (endTime.tv_usec - startTime.tv_usec);
        //printf("%f, %i, %i, %i\n", neededSeconds, my_values->compX, my_values->compY, my_values->compZ);
        rewind(pFile);
        fprintf(pFile,"%f %i %i %i\n", neededSeconds, my_values->compX, my_values->compY, my_values->compZ);

        // Measure the interval between 2 samples
        //gettimeofday(&endTime, 0);
        //float neededSeconds = (endTime.tv_sec - startTime.tv_sec) + 0.000001 * (endTime.tv_usec - startTime.tv_usec);

    }
    // Stop time measurement
    gettimeofday(&endTime, 0);
    neededSeconds = (endTime.tv_sec - startTime.tv_sec)
                  + 0.000001 * (endTime.tv_usec - startTime.tv_usec);
    //std::cout << "Number of samples: " << numOfSamples << "\tTime needed: " << neededSeconds << " s" << std::endl;
    //std::cout << "Average per sample: " << 1000*neededSeconds/numOfSamples << " ms" << std::endl;
    return 0;
}
