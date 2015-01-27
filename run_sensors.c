/* Copyright 2014 Matt Peddie <peddie@alum.mit.edu>
 *
 * This file is hereby placed in the public domain, or, if your legal
 * system does not recognize such a concept, you may consider it
 * licensed under BSD 3.0.  Use it for good.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <zmq.h>

#include "./zmq.h"
#include "./comms.h"
#include "./structures.h"
#include "./log.h"
#include "./sensors.h"
#include "./misc.h"
#include "./lisa.h"

#include "./lisa_messages.h"
#include "./print_output.h"

#include "./protos_c/messages.pb-c.h"



//#define ALL
//#define DEBUG
#ifdef ALL
#define IMU
//#define RC
#define AHRS
#define AIRSPEED
#define GPS
#endif


const double gyro_scale_unit_coef = 0.0139882;
const double acc_scale_unit_coef = 0.0009766;
const double mag_scale_unit_coef = 0.0004883;
const double ahrs_unit_coef = 0.0000305;

char* TAG = "RUN_SENSORS";

/* ZMQ resources */
static void *zctx = NULL;
static void *zsock_sensors = NULL;
static void *zsock_log = NULL;

void *zsock_print = NULL;


Protobetty__Sensors sensors;
Protobetty__LogMessage log_data;


/* Error tracking. */
int txfails = 0, rxfails = 0;

static void __attribute__((noreturn)) die(int code) {
    zdestroy(zsock_log, NULL);
    zdestroy(zsock_sensors, NULL);
    zdestroy(zsock_print, NULL);
    
lisa_close();

    printf("%d TX fails; %d RX fails.\n", txfails, rxfails);
    printf("Moriturus te saluto!\n");
    exit(code);
}

/* Our signal to GTFO */
static int bail = 0;

static void sigdie(int signum) {
    bail = signum;
}

int main(int argc __attribute__((unused)),
         char **argv __attribute__((unused))) {
    
    struct timespec t;
    struct sched_param param;
    int rt_interval= 0;
    //get arguments for frequency and
    if (argc == 3)
    {
        char* arg_ptr;
        long priority = strtol(argv[1], &arg_ptr,10);
        if (*arg_ptr != '\0' || priority > INT_MAX) {
            printf("Failed to read passed priority. Using DEFAULT value instead.\n");
            priority = DEFAULT_RT_PRIORITY;
            
        }
        printf("Setting priority to %li\n", priority);
        set_priority(&param, priority);
        
        long frequency = strtol(argv[2], &arg_ptr,10);
        if (*arg_ptr != '\0' || frequency > INT_MAX) {
            printf("Failed to read passed frequency. Using DEFAULT value instead.\n");
            frequency = DEFAULT_RT_FREQUENCY;
        }
        printf("Setting frequency to %li Hz. RT-interval: %i\n", frequency, rt_interval);
        rt_interval = (NSEC_PER_SEC/frequency);
    }
    else
    {
        printf("No paarameters passed. Using DEFAULT values: \nPRIORITY=%i and FREQUENCY=%i\n",
               DEFAULT_RT_PRIORITY, DEFAULT_RT_FREQUENCY);
        set_priority(&param, DEFAULT_RT_PRIORITY);
        rt_interval = (NSEC_PER_SEC/DEFAULT_RT_FREQUENCY);
    }
    stack_prefault();
    
    
    
    
    
    /* Confignals. */
    if (signal(SIGINT, &sigdie) == SIG_IGN)
        signal(SIGINT, SIG_IGN);
    if (signal(SIGTERM, &sigdie) == SIG_IGN)
        signal(SIGTERM, SIG_IGN);
    if (signal(SIGHUP, &sigdie) == SIG_IGN)
        signal(SIGHUP, SIG_IGN);
    if (signal(SIGABRT, &sigdie) == SIG_IGN)
        signal(SIGABRT, SIG_IGN);
    
    
    
    /* ZMQ setup first. */
    
    /* Set a low high-water mark (a short queue length, measured in
   * messages) so that a sending PUSH will block if the receiving PULL
   * isn't reading.  In the case of PUB/SUB, we still want a short
   * queue; it will prevent outdated messages from building up.  We
   * also set a small-ish buffer size so that the PUSH/PULL socket
   * pair will block or a PUB/SUB socket pair won't accumulate too
   * many outdated messages. */
#ifdef PUSHPULL
    zsock_sensors = setup_zmq_sender(SENSORS_CHAN, &zctx, ZMQ_PUSH, 1, 500);
#else
    zsock_sensors = setup_zmq_sender(SENSORS_CHAN, &zctx, ZMQ_PUB, 1, 500);
#endif
    if (NULL == zsock_sensors)
        die(1);
    zsock_print = setup_zmq_sender(PRINT_CHAN, &zctx, ZMQ_PUSH, 100, 500);
    if (NULL == zsock_print)
        die(1);
    zsock_log = setup_zmq_sender(LOG_CHAN, &zctx, ZMQ_PUSH, 100, 500);
    if (NULL == zsock_log)
        die(1);
    
    
    
    
    /*******************************************
 *PROTOBUF-C INITIALIZATION
 *Memory for submessages is allocated here.
 *If submessage is conatined in finally message
 *the pointer will set to the corresponding submessages
 */
    //Initializing Protobuf messages main sensor message
    protobetty__sensors__init(&sensors);
#ifdef IMU
    //Initialize Protobuf for Gyro
    Protobetty__IMU imu = PROTOBETTY__IMU__INIT;
    Protobetty__Timestamp imu_timestamp = PROTOBETTY__TIMESTAMP__INIT;
    imu.timestamp = &imu_timestamp;
    Protobetty__Xyz gyro_data = PROTOBETTY__XYZ__INIT;
    imu.gyro = &gyro_data;
    //Initialize Protobuf for Accelerometer
    Protobetty__Xyz accel_data = PROTOBETTY__XYZ__INIT;
    imu.accel = &accel_data;
    //Initialize Protobuf for Magnetometer
    Protobetty__Xyz mag_data = PROTOBETTY__XYZ__INIT;
    imu.mag = &mag_data;
#endif
#ifdef AIRSPEED
    //Initialize Protobuf for Airspeed
    Protobetty__Airspeed airspeed = PROTOBETTY__AIRSPEED__INIT;
    Protobetty__Timestamp airspeed_timestamp = PROTOBETTY__TIMESTAMP__INIT;
    airspeed.timestamp = &airspeed_timestamp;
#endif
#ifdef RC
    //Initialize Protobuf for RC commands
    Protobetty__Rc rc = PROTOBETTY__RC__INIT;
    Protobetty__Timestamp rc_timestamp = PROTOBETTY__TIMESTAMP__INIT;
    rc.timestamp = &rc_timestamp;
#endif
    protobetty__log_message__init(&log_data);
#ifdef SERVOS
    Protobetty__Servos servos = PROTOBETTY__SERVOS__INIT;
    Protobetty__Timestamp servo_timestamp = PROTOBETTY__TIMESTAMP__INIT;
    servos.direction = PROTOBETTY__SERVOS__DIRECTION__LISA2BONE;
    servos.timestamp = &servo_timestamp;
#endif
    
    
    
    
    uint8_t* zmq_buffer = calloc(sizeof(uint8_t),PROTOBETTY__MESSAGE__CONSTANTS__MAX_MESSAGE_SIZE);
    unsigned int packed_length;
    
    
    clock_gettime(CLOCK_MONOTONIC ,&t);
    /* start after one second */
    t.tv_sec++;
    
    /* const int noutputs = npolls - ninputs; */
    
    /* Here's the main loop -- we only do stuff when input or output
   * happens.  The timeout can be put to good use, or you can also use
   * timerfd_create() to create a file descriptor with a timer under
   * the hood and dispatch on that.
   *
   * I apologize for the length of this loop.  For production code,
   * you'd want to pull most of the actual handling out into functions
   * and simply loop over your polls; I've left it all inline here
   * mostly out of laziness. */
    if (bail) die(bail);
    
    //Init LISA
    uint8_t buffer[PROTOBETTY__MESSAGE__CONSTANTS__MAX_MESSAGE_SIZE];
    
    lisa_init(buffer);

    int epoll = epoll_create(1);
    if (epoll == -1)
    {
        printf("Error in epoll_create\n");
        die(1);
    }
    epoll_ctl(epoll,EPOLL_CTL_ADD,lisa_get_gpio_fd(), lisa_get_epoll_event());
    struct epoll_event pevents[ 1 ];

    //When sensor data is in circular buffer
    while (1)
    {
        int retval = epoll_wait(epoll,pevents,1,500);
        printf("Epoll returned %i \n", retval);
	if (bail) die(bail);
        if (retval > 0)
        {
            if ( pevents[0].events & EPOLLIN )
            {
		printf("Lisa event... \n");
                retval = lisa_read_message();
                if (retval > 0)
                {
                    sensors_spi_t* sensor_data =(sensors_spi_t*) buffer;
                    get_protbetty_timestamp(imu.timestamp);
                    scaled_to_protobuf(&(sensor_data->accel), imu.accel, acc_scale_unit_coef);
                    scaled_to_protobuf(&(sensor_data->gyro), imu.gyro, gyro_scale_unit_coef);
                    scaled_to_protobuf(&(sensor_data->mag), imu.mag, mag_scale_unit_coef);
                    sensors.imu = &imu;
                    get_protbetty_timestamp(airspeed.timestamp);
                    airspeed.scaled = sensor_data->airspeed.scaled;
                    airspeed.raw = sensor_data->airspeed.adc;
                    airspeed.offset = sensor_data->airspeed.offset;
                    sensors.airspeed = &airspeed;
                }
                else if (retval == ERROR_CHECKSUM)
                {
                    send_warning(zsock_print,TAG,"ERROR Checksum test failed for id %i\n",buffer[LISA_INDEX_MSG_ID]);
                }
                else if (retval == ERROR_COMMUNICATION)
                {
                    send_warning(zsock_print,TAG,"ERROR wrong receiving data\n");
                }
            }
        }
        else
        {
            continue;
        }

        //********************************************
        // SENDING SENSOR DATA to Controller
        //********************************************
        //If we have all IMU messages we send data to controller
        if (sensors.imu  != NULL)
        {
            //set message type corresponding to the data currently available
            if ((sensors.gps_position != NULL || sensors.gps_velocity != NULL) && (sensors.airspeed != NULL))
            {
                sensors.type = PROTOBETTY__SENSORS__TYPE__IMU_GPS_AIRSPEED;
            }
            else if (sensors.gps_position != NULL || sensors.gps_velocity != NULL)
            {
                sensors.type = PROTOBETTY__SENSORS__TYPE__IMU_GPS;
            }
            else if (sensors.airspeed != NULL)
            {
                sensors.type = PROTOBETTY__SENSORS__TYPE__IMU_AIRSPEED;
            }
            else
            {
                sensors.type = PROTOBETTY__SENSORS__TYPE__IMU_ONLY;
            }
            //get size of packed data
            packed_length = protobetty__sensors__get_packed_size(&sensors);
            //pack data to buffer
            protobetty__sensors__pack(&sensors,zmq_buffer);
            //sending sensor message over zmq
            int zs = zmq_send(zsock_sensors, zmq_buffer, packed_length, 0);
            if (zs < 0) {
                txfails++;
            } else {
                send_debug(zsock_print,TAG,"Message sent to controller!, size: %u\n", packed_length);
            }
            log_data.sensors = &sensors;
            //get size of packed data
            packed_length = protobetty__log_message__get_packed_size(&log_data);
            //pack data to buffer
            protobetty__log_message__pack(&log_data,zmq_buffer);
            zs = zmq_send(zsock_log, zmq_buffer, packed_length, ZMQ_NOBLOCK);
            if (zs < 0) {
                txfails++;
            } else {
                send_debug(zsock_print,TAG,"Message sent to logger!, size: %u\n", packed_length);
            }
            protobetty__sensors__init(&sensors);
            protobetty__log_message__init(&log_data);
        }

    }

    /* Shouldn't get here. */
    return 0;
}














