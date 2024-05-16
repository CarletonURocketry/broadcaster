#include "radio.h"
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

/** The read buffer size for incoming data.
 * Since no radio packet can be greater than 512 bytes, and data is provided in ASCII encoding for hex symbols, one byte
 * of data will be one ASCII hex character. So a 512 character buffer is sufficient for all packet sizes. */
#define BUFFER_SIZE 512

/** How many times broadcaster will attempt to transmit a packet before giving up. */
#define RETRY_LIMIT 3

/** The name of the message queue to read input from. */
#define IN_QUEUE "packager-out"

/** The read buffer for input. */
char buffer[BUFFER_SIZE];

/** Whether to read from a message queue or from stdin. Queue by default. */
bool from_q = true;

/** Input message queue file descriptor. */
mqd_t input_q;

/**
 * A macro for exiting with a failure when validation fails.
 * @param vfunc The validation function, which should take optarg and radio_parameters as parameters.
 * @param message The message to be printed when validation fails. Should include a %s option for optarg.
 * */
#define validate_param(vfunc, message)                                                                                 \
    if (!vfunc(optarg, &radio_parameters)) {                                                                           \
        fprintf(stderr, message, optarg);                                                                              \
        exit(EXIT_FAILURE);                                                                                            \
    }

/** The device name of the serial port connected to the LoRa radio. */
static char *serial_port = NULL;

/** The default radio parameters. */
static struct lora_params_t radio_parameters = {.modulation = LORA,
                                                .frequency = 433050000,
                                                .power = 15,
                                                .spread_factor = 9,
                                                .coding_rate = CR_4_7,
                                                .bandwidth = 500,
                                                .preamble_len = 6,
                                                .cyclic_redundancy = false,
                                                .iqi = false,
                                                .sync_word = 0x43};

int main(int argc, char **argv) {

    int c;
    while ((c = getopt(argc, argv, ":m:f:p:s:r:b:l:cqy:i")) != -1) {
        switch (c) {
        case 'm':
            validate_param(radio_validate_mod, "Invalid modulation type '%s'\n");
            break;
        case 'f':
            validate_param(radio_validate_freq, "Invalid frequency value '%s'\n");
            break;
        case 'p':
            validate_param(radio_validate_pwr, "Invalid power value '%s'\n");
            break;
        case 's':
            validate_param(radio_validate_sf, "Invalid spread factor '%s'\n");
            break;
        case 'r':
            validate_param(radio_validate_cr, "Invalid coding rate '%s'\n");
            break;
        case 'b':
            validate_param(radio_validate_bw, "Invalid bandwidth value '%s'\n");
            break;
        case 'l':
            validate_param(radio_validate_prlen, "Invalid preamble length '%s'\n");
            break;
        case 'y':
            validate_param(radio_validate_sync, "Invalid sync word '%s'\n");
            break;
        case 'c':
            radio_parameters.cyclic_redundancy = true;
            break;
        case 'q':
            radio_parameters.iqi = true;
            break;
        case 'i':
            from_q = false;
            break;
        case ':':
            fprintf(stderr, "Option -%c requires an argument.", optopt);
            exit(EXIT_FAILURE);
            break;
        case '?':
            fprintf(stderr, "Unknown option -%c\n", optopt);
            exit(EXIT_FAILURE);
            break;
        }
    }

    /* Positional argument for device descriptor. */
    if (optind >= argc) {
        fprintf(stderr, "LoRa module device descriptor is required.\n");
        exit(EXIT_FAILURE);
    }
    serial_port = argv[optind];

    /* Open message queue for input if not reading from stdin. */
    if (from_q) {
        input_q = mq_open(IN_QUEUE, O_RDONLY);
        if (input_q == -1) {
            fprintf(stderr, "Could not open input message queue %s: %s\n", IN_QUEUE, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    /* Open radio for reading and writing. */
    int radio = open(serial_port, O_RDWR | O_NDELAY | O_NOCTTY);
    if (radio == -1) {
        fprintf(stderr, "Could not open tty with error %d\n.", radio);
        exit(EXIT_FAILURE);
    }

    /* Set up device using correct UART settings. */
    struct termios tty;
    if (tcgetattr(radio, &tty) != 0) {
        fprintf(stderr, "Failed to get tty attributes with error %d\n", errno);
        close(radio);
        exit(EXIT_FAILURE);
    }

    radio_setup_tty(&tty);

    if (tcsetattr(radio, TCSANOW, &tty) != 0) {
        fprintf(stderr, "Failed to set tty attrs with error %d\n", errno);
        close(radio);
        exit(EXIT_FAILURE);
    }
    tcflush(radio, TCIFLUSH); // Flush all unread messages from radio

    /* Set radio parameters */
    uint8_t count = 0;
    for (; !radio_set_params(radio, &radio_parameters) && count < RETRY_LIMIT; count++)
        ;
    if (count == RETRY_LIMIT) {
        close(radio);
        fprintf(stderr, "Failed to set radio parameters\n");
    }

    /* Read input stream data line by line. */
    while (1) {

        uint8_t transmission_tries = 0;

        if (from_q) {
            size_t nbytes;
            nbytes = mq_receive(input_q, buffer, BUFFER_SIZE, NULL);
            if (nbytes == (size_t)-1) {
                fprintf(stderr, "Failed to read from queue: %s\n", strerror(errno));
                // Don't quit, just continue
            }
            while (transmission_tries < RETRY_LIMIT && !radio_tx_bytes(radio, (uint8_t *)buffer, nbytes))
                ; // Try 3 times
        } else {
            // End of input stream triggers program exit
            if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) break;
            while (transmission_tries < RETRY_LIMIT && !radio_tx(radio, buffer))
                ; // Try 3 times
        }

        // If transmission fails just log and continue
        if (transmission_tries >= RETRY_LIMIT) fprintf(stderr, "Failed to transmit %s\n", buffer);
    }

    close(radio);
    return EXIT_SUCCESS;
}
