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
#include "./arduino.h"

//#include "./arduino_messages_telemetry.h"
#include "./print_output.h"

#include "./protos_c/messages.pb-c.h"
#include "./betcomm/c/betcall.pb-c.h"




//#define ALL
//#define DEBUG
#ifdef ALL
#define IMU
//#define RC
#define AHRS
//#define AIRSPEED
#define GPS
#define LINEANGLE
#endif


#define MESSAGES_TO_READ_PER_CYLE 2

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
static void fill_betcall_lineangle(BetCALL__LineAngle* betcall, sensor_data_t* sensor_data);
static void fill_betcall_imu(BetCALL__IMU* betcall, sensor_data_t* sensor_data);


BetCALL__Sensors sensors;

/* Error tracking. */
int txfails = 0, rxfails = 0;

static void __attribute__((noreturn)) die(int code) {
    zdestroy(zsock_log, NULL);
    zdestroy(zsock_sensors, NULL);
    zdestroy(zsock_print, NULL);
    zdestroy(zsock_groundstation, NULL);
    //Close connections and port opened by LISA
    arduino_close();

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
    zsock_groundstation = setup_zmq_sender(GROUNDSTATION_SEND_CHAN, &zctx, ZMQ_PUB, 10, 512);
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
    BetCALL__Ticks sensors_ticks = BET_CALL__TICKS__INIT;
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
    sensors.imu = imu_messages;
    //Initialize Protobuf for Gyro
    BetCALL__IMU** imu_messages_arm;
    imu_messages_arm = malloc (sizeof (BetCALL__IMU*) * NUMBER_OF_IMU_DATA_PACKETS);
    for (size_t i = 0; i < NUMBER_OF_IMU_DATA_PACKETS; i++)
    {
        imu_messages_arm[i] = malloc (sizeof (BetCALL__IMU));
        bet_call__imu__init (imu_messages_arm[i]);
        BetCALL__Ticks* imu_ticks = malloc (sizeof(BetCALL__Ticks));
        bet_call__ticks__init(imu_ticks);
        imu_messages_arm[i]->ticks = imu_ticks;
        //Initialize Protobuf for Gyroscope
        BetCALL__XYZI* gyro_data = malloc (sizeof(BetCALL__XYZI));
        bet_call__xyz_i__init(gyro_data);
        imu_messages_arm[i]->gyro = gyro_data;
        //Initialize Protobuf for Accelerometer
        BetCALL__XYZI* accel_data = malloc (sizeof(BetCALL__XYZI));
        bet_call__xyz_i__init(accel_data);
        imu_messages_arm[i]->accel = accel_data;
        //Initialize Protobuf for Magnetometer
        BetCALL__XYZI* mag_data = malloc (sizeof(BetCALL__XYZI));
        bet_call__xyz_i__init(mag_data);
        imu_messages_arm[i]->mag = mag_data;
    }
    sensors.imu_arm = imu_messages_arm;
#endif
#ifdef AIRSPEED
    //Initialize Protobuf for Airspeed
    BetCALL__Airspeed airspeed = BET_CALL__AIRSPEED__INIT;
    BetCALL__Ticks airspeed_ticks = BET_CALL__TICKS__INIT;
    airspeed.ticks = &airspeed_ticks;
    airspeed.has_offset = 1;
    airspeed.has_raw = 1;
#endif
#ifdef LINEANGLE
    //Initialize Protobuf for Airspeed
    BetCALL__LineAngle lineangle = BET_CALL__LINE_ANGLE__INIT;
    BetCALL__Ticks lineangle_ticks = BET_CALL__TICKS__INIT;
    lineangle.ticks = &lineangle_ticks;
#endif



    uint8_t* zmq_buffer = calloc(sizeof(uint8_t),BET_CALL__MESSAGE__CONSTANTS__MAX_MESSAGE_SIZE);
    unsigned int packed_length;



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

    //Init buffers and arduino
    uint8_t buffer[1024];
    arduino_init(buffer);
    //starting timing for periodic loop
    clock_gettime(CLOCK_MONOTONIC ,&t);
    /* start after one second */
    t.tv_sec++;
    int64_t t_old = t.tv_sec;
    uint32_t messages_sent_per_sec =0;


    //When sensor data is in circular buffer
    while (1)
    {
        if (bail) die(bail);
        /* wait until next shot */
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL);
        if (bail) die(bail);
        uint16_t messages_read =0;
        while (messages_read < MESSAGES_TO_READ_PER_CYLE)
        {
            int retval = arduino_read_message();
            if (retval > 0)
            {
                sensor_data_t* sensor_data =(sensor_data_t*) buffer;
#ifdef DEBUG
                printf("Received message of type %d\n",sensor_data->type);
#endif
#ifdef IMU
                switch (sensor_data->type)
                {
                case 0:
                    fill_betcall_imu(imu_messages[sensors.n_imu], sensor_data);
                    sensors.n_imu++;
                    sensors.imu = imu_messages;
                    break;
                case 1:
                    fill_betcall_imu(imu_messages_arm[sensors.n_imu_arm], sensor_data);
                    sensors.n_imu_arm++;
                    sensors.imu_arm = imu_messages_arm;
                    break;
                default:
                    printf("Unknown type %d \n",sensor_data->type );
                }
#endif               

#ifdef LINEANGLE
#ifdef DEBUG
                printf("LINEANGLE package with seqNo %i and %i ticks\n",sensor_data->lineangle.header.sequence_number, sensor_data->lineangle.header.ticks);
#endif
                fill_betcall_lineangle(&lineangle, sensor_data);
                sensors.line_angle = &lineangle;
#endif
                messages_read++;
            }
            else if (retval == ERROR_CHECKSUM)
            {
                send_warning(zsock_print,TAG,"ERROR Checksum test failed for id %i\n");
                continue;
            }
            else if (retval == ERROR_COMMUNICATION)
            {

                send_warning(zsock_print,TAG,"ERROR wrong receiving data\n");
                die(-1);
                continue;
            }
            else
            {
#ifdef DEBUG
                printf("...\n");
#endif
                usleep(100);
            }
        }
        get_betcall_timestamp(&sensors_timestamp);
        sensors.timestamp = &sensors_timestamp;
        sensors.ticks = &sensors_ticks;
        //get size of packed data
        packed_length = bet_call__sensors__get_packed_size(&sensors);
        //pack data to buffer
        bet_call__sensors__pack(&sensors,zmq_buffer);
        messages_sent_per_sec++;
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
        calc_next_shot(&t,rt_interval);
        if (t.tv_sec != t_old)
        {
            printf("Sending messages with a frequency of %u Hz\n", messages_sent_per_sec);
            messages_sent_per_sec = 0;
            t_old = t.tv_sec;
        }
    }

    /* Shouldn't get here. */
    return 0;
}

static void fill_betcall_imu(BetCALL__IMU* betcall, sensor_data_t* sensor_data)
{
#ifdef DEBUG
    printf("Accel \tX=%i\tY=%i\tZ=%i\n",sensor_data->imu.accel.x, sensor_data->imu.accel.y, sensor_data->imu.accel.z);
    printf("Gyro \tp=%i\tq=%i\tr=%i\n",sensor_data->imu.gyro.p, sensor_data->imu.gyro.q,sensor_data->imu.gyro.r );
    printf("Mag \tX=%i\tY=%i\tZ=%i\n",sensor_data->imu.mag.x, sensor_data->imu.mag.y, sensor_data->imu.mag.z);
#endif
    //setting data in protobufs
    betcall->accel->x = sensor_data->imu.accel.x;
    betcall->accel->y = sensor_data->imu.accel.y;
    betcall->accel->z = sensor_data->imu.accel.z;
    betcall->gyro->z = sensor_data->imu.gyro.p;
    betcall->gyro->y = sensor_data->imu.gyro.q;
    betcall->gyro->x = sensor_data->imu.gyro.r;
    betcall->mag->y = sensor_data->imu.mag.x;
    betcall->mag->x = sensor_data->imu.mag.y;
    betcall->mag->z = sensor_data->imu.mag.z;
    betcall->ticks->ticks = sensor_data->imu.header.ticks;
    betcall->ticks->incremented = sensor_data->imu.header.seconds;
    betcall->sequencenumber = sensor_data->imu.header.sequence_number;
}

static void fill_betcall_lineangle(BetCALL__LineAngle* betcall, sensor_data_t* sensor_data)
{
#ifdef DEBUG
    printf("Lineangle%i=%i \n",sensor_data->type,sensor_data->lineangle.raw_angle);
#endif

    betcall->ticks->ticks = sensor_data->lineangle.header.ticks;
    betcall->ticks->incremented = sensor_data->lineangle.header.seconds;
    switch (sensor_data->type)
    {
    case 0:
        betcall->elevation = sensor_data->lineangle.raw_angle;
        break;
    case 1:
        betcall->azimuth =  sensor_data->lineangle.raw_angle;
        break;
    default:
        break;
    }
}


//#ifdef AIRSPEED
//                printf("AIRSPEED package with seqNo %i and %i ticks\n",sensor_data->airspeed.header.sequence_number, sensor_data->airspeed.header.ticks);
//                printf("Airspeed \tscaled=%i\traw=%i\toffset=%i\n",sensor_data->airspeed.scaled, sensor_data->airspeed.raw, sensor_data->airspeed.offset);

//                airspeed.ticks->ticks = sensor_data->airspeed.header.ticks;
//                airspeed.ticks->incremented = sensor_data->airspeed.header.seconds;
//                airspeed.scaled = sensor_data->airspeed.scaled;
//                airspeed.raw = sensor_data->airspeed.raw;
//                airspeed.offset = sensor_data->airspeed.offset;
//                sensors.airspeed = &airspeed;
//#endif
