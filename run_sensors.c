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

#include "./lisa_messages_telemetry.h"
#include "./print_output.h"

#include "./protos_c/messages.pb-c.h"
#include "./betcomm/c/betcall.pb-c.h"




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
static void *zsock_groundstation = NULL;


void *zsock_print = NULL;


BetCALL__Sensors sensors;

/* Error tracking. */
int txfails = 0, rxfails = 0;

static void __attribute__((noreturn)) die(int code) {
    zdestroy(zsock_log, NULL);
    zdestroy(zsock_sensors, NULL);
    zdestroy(zsock_print, NULL);
    zdestroy(zsock_groundstation, NULL);
    //Close connections and port opened by LISA
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
    zsock_groundstation = setup_zmq_sender(GROUNDSTATION_SEND_CHAN, &zctx, ZMQ_REQ, 10, 512);
    if (NULL == zsock_groundstation)
        die(1);




    /*******************************************
 *PROTOBUF-C INITIALIZATION
 *Memory for submessages is allocated here.
 *If submessage is conatined in finally message
 *the pointer will set to the corresponding submessages
 */
    //Initializing Protobuf messages main sensor message
    bet_call__sensors__init(&sensors);
    BetCALL__Timestamp sensors_timestamp = BET_CALL__TIMESTAMP__INIT;
    #ifdef IMU
    //Initialize Protobuf for Gyro
    BetCALL__IMU** imu_messages;
    imu_messages = malloc (sizeof (BetCALL__IMU*) * NUMBER_OF_IMU_DATA_PACKETS);
    for (size_t i = 0; i < NUMBER_OF_IMU_DATA_PACKETS; i++)
    {
        imu_messages[i] = malloc (sizeof (BetCALL__IMU));
        bet_call__imu__init (imu_messages[i]);
        BetCALL__Ticks* imu_ticks = malloc (sizeof(BetCALL__Ticks));
        bet_call__ticks__init(imu_ticks);
        imu_messages[i]->ticks = imu_ticks;
        //Initialize Protobuf for Gyroscope
        BetCALL__XYZI* gyro_data = malloc (sizeof(BetCALL__XYZI));
        bet_call__xyz_i__init(gyro_data);
        imu_messages[i]->gyro = gyro_data;
        //Initialize Protobuf for Accelerometer
        BetCALL__XYZI* accel_data = malloc (sizeof(BetCALL__XYZI));
        bet_call__xyz_i__init(accel_data);
        imu_messages[i]->accel = accel_data;
        //Initialize Protobuf for Magnetometer
        BetCALL__XYZI* mag_data = malloc (sizeof(BetCALL__XYZI));
        bet_call__xyz_i__init(mag_data);
        imu_messages[i]->mag = mag_data;
    }
    sensors.n_imu = NUMBER_OF_IMU_DATA_PACKETS;
    sensors.imu = imu_messages;
    #endif
    #ifdef AIRSPEED
    //Initialize Protobuf for Airspeed
    BetCALL__Airspeed airspeed = BET_CALL__AIRSPEED__INIT;
    BetCALL__Ticks airspeed_ticks = BET_CALL__TICKS__INIT;
    airspeed.ticks = &airspeed_ticks;
    airspeed.has_offset = 1;
    airspeed.has_raw = 1;
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
    uint8_t buffer[1024];

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
        printf("...\n");
        int retval = epoll_wait(epoll,pevents,1,500);
        if (bail) die(bail);
        if (retval > 0)
        {
            if ( pevents[0].events & EPOLLIN )
            {
                retval = lisa_read_message();
                if (retval > 0)
                {
                    sensor_data_t* sensor_data =(sensor_data_t*) buffer;
                    printf("\nDEBUG output of message with seqNo %i and ticks %i\n", sensor_data->header.sequence_number, sensor_data->header.ticks);
                    for (int32_t i = 0 ; i < NUMBER_OF_IMU_DATA_PACKETS; i++)
                    {
#ifdef IMU

                        //Debug output on beaglebone
                        printf("IMU package %i with seqNo %i and %i ticks\n", i, sensor_data->imu[i].header.sequence_number, sensor_data->imu[i].header.ticks);
                        printf("Accel \tX=%i\tY=%i\tZ=%i\n",sensor_data->imu[i].accel.x, sensor_data->imu[i].accel.y, sensor_data->imu[i].accel.z);
                        printf("Gyro \tp=%i\tq=%i\tr=%i\n",sensor_data->imu[i].gyro.p, sensor_data->imu[i].gyro.q,sensor_data->imu[i].gyro.r );
                        printf("Mag \tX=%i\tY=%i\tZ=%i\n",sensor_data->imu[i].mag.x, sensor_data->imu[i].mag.y, sensor_data->imu[i].mag.z);

                        //setting data in protobufs
                        imu_messages[i]->accel->x = sensor_data->imu[i].accel.x;
                        imu_messages[i]->accel->y = sensor_data->imu[i].accel.y;
                        imu_messages[i]->accel->z = sensor_data->imu[i].accel.z;
                        imu_messages[i]->gyro->z = sensor_data->imu[i].gyro.p;
                        imu_messages[i]->gyro->y = sensor_data->imu[i].gyro.q;
                        imu_messages[i]->gyro->x = sensor_data->imu[i].gyro.r;
                        imu_messages[i]->mag->y = sensor_data->imu[i].mag.x;
                        imu_messages[i]->mag->x = sensor_data->imu[i].mag.y;
                        imu_messages[i]->mag->z = sensor_data->imu[i].mag.z;
                        imu_messages[i]->ticks->ticks = sensor_data->imu[i].header.ticks;
                        imu_messages[i]->ticks->incremented = sensor_data->imu[i].header.incremented_ticks;
                        imu_messages[i]->sequencenumber = sensor_data->imu[i].header.sequence_number;
                        sensors.imu = imu_messages;
                        sensors.n_imu = NUMBER_OF_IMU_DATA_PACKETS;
#endif


                    }

#ifdef AIRSPEED
                    printf("AIRSPEED package with seqNo %i and %i ticks\n",sensor_data->airspeed.header.sequence_number, sensor_data->airspeed.header.ticks);
                    printf("Airspeed \tscaled=%f\traw=%i\toffset=%i\n",sensor_data->airspeed.scaled, sensor_data->airspeed.raw, sensor_data->airspeed.offset);

                    airspeed.ticks->ticks = sensor_data->airspeed.header.ticks;
                    airspeed.ticks->incremented = sensor_data->airspeed.header.incremented_ticks;
                    airspeed.scaled = sensor_data->airspeed.scaled;
                    airspeed.raw = sensor_data->airspeed.raw;
                    airspeed.offset = sensor_data->airspeed.offset;
                    sensors.airspeed = &airspeed;
#endif
                    get_betcall_timestamp(&sensors_timestamp);
                    sensors.timestamp = &sensors_timestamp;

                }
                else if (retval == ERROR_CHECKSUM)
                {
                    send_warning(zsock_print,TAG,"ERROR Checksum test failed for id %i\n",buffer[LISA_INDEX_MSG_ID]);
                    continue;
                }
                else if (retval == ERROR_COMMUNICATION)
                {
                    send_warning(zsock_print,TAG,"ERROR wrong receiving data\n");
                    continue;
                }
            }
        }
        else
        {
            continue;
        }

        //get size of packed data
        packed_length = bet_call__sensors__get_packed_size(&sensors);
        //pack data to buffer
        bet_call__sensors__pack(&sensors,zmq_buffer);
        //sending sensor message over zmq to groundstation
        int zs = zmq_send(zsock_groundstation, zmq_buffer, packed_length, 0);
        if (zs < 0) {
            txfails++;
        } else {
            send_debug(zsock_print,TAG,"Message sent to Groundstation!, size: %u\n", packed_length);
        }
        //sending sensor message over zmq internal to other processes
        zs = zmq_send(zsock_sensors, zmq_buffer, packed_length, 0);
        if (zs < 0) {
            txfails++;
        } else {
            send_debug(zsock_print,TAG,"Message sent to other process!, size: %u\n", packed_length);
        }
        bet_call__sensors__init(&sensors);
    }

    /* Shouldn't get here. */
    return 0;
}
