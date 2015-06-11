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
#include "./misc.h"
#include "./actuators.h"


#include "./uart.h"
#include "./arduino_messages_usb.h"
#include "./betcomm/c/betpush.pb-c.h"


#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define MIN_SERVO_VALUE -30
#define MAX_SERVO_VALUE 30
#define NEUTRAL_SERVO_VALUE 90

char* const TAG = "RUN_ACTUATORS";

/* ZMQ resources */
static void *zctx = NULL;
static void *zsock_controller = NULL;
void *zsock_print = NULL;
static void *zsock_log = NULL;
static void *zsock_groundstation = NULL;




/* Error tracking. */
int txfails = 0, rxfails = 0;

static void __attribute__((noreturn)) die(int code) {
    zdestroy(zsock_controller, NULL);
    zdestroy(zsock_print,NULL);
    zdestroy(zsock_log,NULL);
    zdestroy(zsock_groundstation,NULL);
    serial_port_close();
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
    printf("%i", argc);
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
        printf("Setting frequency to %li Hz.\n", frequency);
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


    int err = serial_port_setup();
    if (err != UART_ERR_NONE)
        printf("Error setting up UART \n");

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
    zsock_controller = setup_zmq_receiver(ACTUATORS_CHAN, &zctx, ZMQ_SUB, NULL, 1, 500);
    if (NULL == zsock_controller)
        die(1);
    zsock_print = setup_zmq_sender(PRINT_CHAN, &zctx, ZMQ_PUSH, 1000, 500);
    if (NULL == zsock_print)
        die(1);
    zsock_groundstation = setup_zmq_receiver(GROUNDSTATION_RECV_CHAN, &zctx, ZMQ_SUB, NULL, 1, 500);
    if (NULL == zsock_groundstation)
        die(1);



    /* Actuator data storage. */
    BetPUSH__Actuators* actuators;
    //create template for actuator message for lisa
    sensor_data_actuators_t output;
    //output.startbyte = 0xFE;


//    zmq_pollitem_t poll_controller = {
//        /* Inputs -- in this case, our incoming actuator commands. */
//            .socket = zsock_controller,
//            .fd = -1,
//            .events = ZMQ_POLLIN,
//            .revents = 0
//    };

    zmq_pollitem_t poll_groundstation = {
        /* Inputs -- in this case, our incoming actuator commands. */
            .socket = zsock_groundstation,
            .fd = -1,
            .events = ZMQ_POLLIN,
            .revents = 0
    };

    uint8_t zmq_buffer[BET_PUSH__MESSAGE__CONSTANTS__MAX_MESSAGE_SIZE]; // Input data container for bytes

    clock_gettime(CLOCK_MONOTONIC ,&t);
    /* start after one second */
    t.tv_sec++;

    /* Here's the main loop -- we only do stuff when input or output
   * happens.  The timeout can be put to good use, or you can also use
   * timerfd_create() to create a file descriptor with a timer under
   * the hood and dispatch on that.
   *
   * I apologize for the length of this loop.  For production code,
   * you'd want to pull most of the actual handling out into functions
   * and simply loop over your polls; I've left it all inline here
   * mostly out of laziness. */
    for (;;) {
        if (bail) die(bail);
        /* Poll for activity; time out after 10 milliseconds. */
        const int polled = zmq_poll(&poll_groundstation, 1, 0);
        if (polled < 0) {
            if (bail) die(bail);
            zerr("while polling");
            calc_next_shot(&t,rt_interval);
            continue;
        } else if (polled == 0) {
            if (bail) die(bail);
            calc_next_shot(&t,rt_interval);
            continue;
        }

        if (bail) die(bail);
        if (poll_groundstation.revents & ZMQ_POLLIN) {
            /* Read in some sensor data from this sensor. */
            const int zr = zmq_recvm(zsock_groundstation, zmq_buffer,
                                     BET_PUSH__MESSAGE__CONSTANTS__MAX_MESSAGE_SIZE);
            actuators = bet_push__actuators__unpack(NULL,zr,zmq_buffer);


            if (actuators != NULL){
                //Bound values to SERVO LIMITS
                int32_t t_flaps, t_rudder, t_elevator, t_aileron;
                t_flaps = MIN(actuators->flaps_right, MAX_SERVO_VALUE);
                t_flaps = MAX(t_flaps, MIN_SERVO_VALUE);

                t_rudder = MIN(actuators->rudder, MAX_SERVO_VALUE);
                t_rudder = MAX(t_rudder, MIN_SERVO_VALUE);

                t_elevator = MIN(actuators->elevator, MAX_SERVO_VALUE);
                t_elevator = MAX(t_elevator, MIN_SERVO_VALUE);

                t_aileron = MIN(actuators->ailerons_right, MAX_SERVO_VALUE);
                t_aileron = MAX(t_aileron, MIN_SERVO_VALUE);

                //Set values
                output.flaps =(uint8_t) NEUTRAL_SERVO_VALUE + t_flaps;
                output.rudder =(uint8_t) NEUTRAL_SERVO_VALUE + t_rudder;
                output.elevator =(uint8_t) NEUTRAL_SERVO_VALUE + t_elevator;
                output.aileron =(uint8_t) NEUTRAL_SERVO_VALUE + t_aileron;
                printf("Sending servo values:\n rudder:%d\t elevator:%d\t flap:%d\t aileron:%d\t",output.rudder,output.elevator, output.flaps, output.aileron );
                calculate_checksum((uint8_t*) &output, sizeof(output)-2, &(output.checksum1), &(output.checksum2));
                write_uart((uint8_t*)&output,sizeof(output));
            }
            /* Clear the poll state. */
            poll_groundstation.revents = 0;
            poll_groundstation.events = ZMQ_POLLIN;
        }


        if (bail) die(bail);
        calc_next_shot(&t,rt_interval);

    }

    /* Shouldn't get here. */
    return 0;
}
