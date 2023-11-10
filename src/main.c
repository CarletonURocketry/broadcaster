#include "radio.h"
#include <getopt.h>
#include <hw/spi-master.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

/** The read buffer size for incoming data. */
#define BUFFER_SIZE 250

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
static const char serial_port[] = "/dev/ser3";
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
    while ((c = getopt(argc, argv, ":m:f:p:s:r:b:l:ciy:")) != -1) {
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
        case 'i':
            radio_parameters.iqi = true;
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

    int radio = open(serial_port, O_RDWR | O_NDELAY | O_NOCTTY);

    if (radio == -1) {
        fprintf(stderr, "Could not open tty with error %d\n.", radio);
        exit(EXIT_FAILURE);
    }

    // Set up port
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

    // Set radio parameters
    bool success = radio_set_params(radio, &radio_parameters);
    if (!success) {
        close(radio);
        fprintf(stderr, "Failed to set radio parameters\n");
    }

    // Start transmitting
    dprintf(radio, "radio tx %x\n", 0x111);
    tcdrain(radio);

    close(radio);
    return EXIT_SUCCESS;
}
