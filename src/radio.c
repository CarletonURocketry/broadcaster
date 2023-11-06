#include "radio.h"
#include <hw/spi-master.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>

/** Actual value representation of radio parameters. */
static const char *MODULATIONS[] = {[LORA] = "lora", [FSK] = "fsk"};
static const char *CODING_RATES[] = {
    [CR_4_5] = "4/5",
    [CR_4_6] = "4/6",
    [CR_4_7] = "4/7",
    [CR_4_8] = "4/8",
};
/** How many times the LoRa radio module should be polled for a response. */
static uint64_t timeout = 23;

/**
 * Sets the required parameters for UART communication to work with the LoRa module.
 * @param tty The termios tty struct containing information about the tty params.
 * */
void radio_setup_tty(struct termios *tty) {
    cfsetispeed(tty, B57600);         // Set in speed to 57600bps
    cfsetospeed(tty, B57600);         // Set in speed to 57600bps
    tty->c_cc[VMIN] = 0;              // No minimum amount of bytes to read
    tty->c_cc[VTIME] = 5;             // 0.5s before read times out
    tty->c_lflag &= ~(ECHO | ECHONL); // No echo
    tty->c_cflag &= ~PARENB;          // No parity
    tty->c_cflag &= ~CSTOPB;          // Only one stop bit
}

/**
 * Set the timeout for trying to read a response from the LoRa radio module.
 * @param t How many times the LoRa module should be polled for a response.
 * */
void radio_set_timeout(uint64_t t) { timeout = t; }

/**
 * Set parameters on the LoRa radio.
 * @param radio_fd The file descriptor to the LoRa radio device.
 * @param params A pointer to the struct of LoRa radio parameters to be set.
 * @return True if successful, false if any parameter fails to be set.
 * */
bool radio_set_params(int radio_fd, const struct lora_params_t *params) {

    dprintf(radio_fd, "radio set mod %s\n", MODULATIONS[params->modulation]);
    tcdrain(radio_fd);
    if (!wait_for_ok(radio_fd)) return false;
    dprintf(radio_fd, "radio set freq %u\n", params->frequency);
    tcdrain(radio_fd);
    if (!wait_for_ok(radio_fd)) return false;
    dprintf(radio_fd, "radio set pwr %d\n", params->power);
    tcdrain(radio_fd);
    if (!wait_for_ok(radio_fd)) return false;
    dprintf(radio_fd, "radio set sf sf%u\n", params->spread_factor);
    tcdrain(radio_fd);
    if (!wait_for_ok(radio_fd)) return false;
    dprintf(radio_fd, "radio set cr %s\n", CODING_RATES[params->coding_rate]);
    tcdrain(radio_fd);
    if (!wait_for_ok(radio_fd)) return false;
    dprintf(radio_fd, "radio set bw %u\n", params->bandwidth);
    tcdrain(radio_fd);
    if (!wait_for_ok(radio_fd)) return false;
    dprintf(radio_fd, "radio set prlen %u\n", params->preamble_len);
    tcdrain(radio_fd);
    if (!wait_for_ok(radio_fd)) return false;
    dprintf(radio_fd, "radio set crc %s\n", params->cyclic_redundancy ? "on" : "off");
    tcdrain(radio_fd);
    if (!wait_for_ok(radio_fd)) return false;
    dprintf(radio_fd, "radio set iqi %s\n", params->iqi ? "on" : "off");
    tcdrain(radio_fd);
    if (!wait_for_ok(radio_fd)) return false;
    dprintf(radio_fd, "radio set sync %lx\n", params->sync_word);
    tcdrain(radio_fd);
    if (!wait_for_ok(radio_fd)) return false;

    return true;
}

/**
 * Wait for the LoRa radio module to respond with "ok".
 * @param radio_fd The file descriptor to the LoRa radio device.
 * @return True if ok was the response, false if no response in time or different response.
 * */
bool wait_for_ok(int radio_fd) {
    char buffer[25] = {0};
    for (uint8_t i = 0; i < 3; i++) {
        read(radio_fd, buffer, sizeof(buffer));
        if (strstr(buffer, "ok") != NULL) {
            tcflush(radio_fd, TCIFLUSH);
            return true;
        }
        memset(buffer, 0, sizeof(buffer));
    }
    return false;
}
