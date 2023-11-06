#include "radio.h"
#include <hw/spi-master.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

#define BUFFER_SIZE 250
static const char serial_port[] = "/dev/ser3";
static const struct lora_params_t radio_parameters = {.modulation = LORA,
                                                      .frequency = 433050000,
                                                      .power = 15,
                                                      .spread_factor = 9,
                                                      .coding_rate = CR_4_7,
                                                      .bandwidth = 500,
                                                      .preamble_len = 6,
                                                      .cyclic_redundancy = true,
                                                      .iqi = false,
                                                      .sync_word = 0x43};

int main(int argc, char **argv) {

    int radio = open(serial_port, O_RDWR | O_NDELAY | O_NOCTTY, O_SYNC);

    if (radio == -1 || !isatty(radio)) {
        fprintf(stderr, "Could not open tty.");
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
