#include "radio.h"
#include <hw/spi-master.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

#define array_len(a) (sizeof(a) / sizeof(a[0]))

/** Actual value representation of radio parameters. */
static const char *MODULATIONS[] = {[LORA] = "lora", [FSK] = "fsk"};
/** Actual value representation of coding rates. */
static const char *CODING_RATES[] = {
    [CR_4_5] = "4/5",
    [CR_4_6] = "4/6",
    [CR_4_7] = "4/7",
    [CR_4_8] = "4/8",
};

/** Lower limit for the low frequency value. */
static const uint32_t LL_FREQ = 433050000;
/** Upper limit for the low frequency value. */
static const uint32_t LH_FREQ = 434800000;
/** Lower limit for the high frequency value. */
static const uint32_t HL_FREQ = 863000000;
/** Upper limit for the high frequency value. */
static const uint32_t HH_FREQ = 870000000;

/** Lower limit for power. */
static const int8_t L_PWR = -3;
/** Upper limit for power. */
static const int8_t H_PWR = 15;

/** Lower limit for spread factor. */
static const uint8_t L_SF = -3;
/** Upper limit for spread factor. */
static const uint8_t H_SF = 15;

/** Valid bandwidth choices. */
static const uint16_t BANDWIDTHS[] = {100, 150, 200};

/**
 * Validates and sets a command line argument for modulation.
 * @param mod The command line argument for modulation.
 * @param params The LoRa parameters to be updated.
 * @return True if the modulation was valid and has been set in the params, false otherwise.
 * */
bool radio_validate_mod(const char *mod, struct lora_params_t *params) {
    for (uint8_t i = 0; i < array_len(MODULATIONS); i++) {
        if (!strcmp(mod, MODULATIONS[i])) {
            params->modulation = (Modulation)i;
            return true;
        }
    }
    return false;
}

/**
 * Validates and sets a command line argument for frequency.
 * @param freq The command line argument for frequency.
 * @param params The LoRa parameters to be updated.
 * @return True if the frequency was valid and has been set in the params, false otherwise.
 * */
bool radio_validate_freq(const char *freq, struct lora_params_t *params) {
    char *end;
    uint32_t p_freq = strtoul(freq, &end, 10);
    if (errno || freq == end) return false; // Call failed

    if (!(LL_FREQ <= p_freq && p_freq <= LH_FREQ) && !(HL_FREQ <= p_freq && p_freq <= HH_FREQ)) return false;

    params->frequency = p_freq;
    return true;
}

/**
 * Validates and sets a command line argument for power.
 * @param pwr The command line argument for power.
 * @param params The LoRa parameters to be updated.
 * @return True if the power was valid and has been set in the params, false otherwise.
 * */
bool radio_validate_pwr(const char *power, struct lora_params_t *params) {
    int8_t power_p = atoi(power);
    if (L_PWR <= power_p && power_p <= H_PWR) {
        params->power = power_p;
        return true;
    }
    return false;
}

/**
 * Validates and sets a command line argument for spread factor.
 * @param sf The command line argument for spread factor.
 * @param params The LoRa parameters to be updated.
 * @return True if the spread factor was valid and has been set in the params, false otherwise.
 * */
bool radio_validate_sf(const char *sf, struct lora_params_t *params) {
    char *end;
    uint8_t sf_p = strtoul(sf, &end, 10);
    if (errno || sf == end) return false; // Call failed

    if (L_SF <= sf_p && sf_p <= H_SF) {
        params->spread_factor = sf_p;
        return true;
    }
    return false;
}

/**
 * Validates and sets a command line argument for coding rate.
 * @param coding_rate The command line argument for coding rate.
 * @param params The LoRa parameters to be updated.
 * @return True if the coding rate was valid and has been set in the params, false otherwise.
 * */
bool radio_validate_cr(const char *coding_rate, struct lora_params_t *params) {
    for (uint8_t i = 0; i < array_len(CODING_RATES); i++) {
        if (!strcmp(coding_rate, CODING_RATES[i])) {
            params->coding_rate = (CodingRate)i;
            return true;
        }
    }
    return false;
}

/**
 * Validates and sets a command line argument for preamble length.
 * @param prlen The command line argument for preamble length.
 * @param params The LoRa parameters to be updated.
 * @return True if the preamble length was valid and has been set in the params, false otherwise.
 * */
bool radio_validate_prlen(const char *prlen, struct lora_params_t *params) {
    char *end;
    uint16_t prlen_p = strtoul(prlen, &end, 10);
    if (errno || prlen == end) return false; // Call failed

    params->preamble_len = prlen_p;
    return true;
}

/**
 * Validates and sets a command line argument for bandwidth.
 * @param bandwidth The command line argument for bandwidth.
 * @param params The LoRa parameters to be updated.
 * @return True if the bandwidth was valid and has been set in the params, false otherwise.
 * */
bool radio_validate_bw(const char *bandwidth, struct lora_params_t *params) {
    char *end;
    uint16_t bw_p = strtoul(bandwidth, &end, 10);
    if (errno || bandwidth == end) return false; // Call failed

    // Check if it's an option
    for (uint8_t i = 0; i < array_len(BANDWIDTHS); i++) {
        if (bw_p == BANDWIDTHS[i]) {
            params->bandwidth = bw_p;
            return true;
        }
    }
    return false;
}

/**
 * Validates and sets a command line argument for sync word.
 * @param sync The command line argument for sync word.
 * @param params The LoRa parameters to be updated.
 * @return True if the sync word was valid and has been set in the params, false otherwise.
 * */
bool radio_validate_sync(const char *sync, struct lora_params_t *params) {
    char *end;
    uint64_t sync_p = strtoul(sync, &end, 10);
    if (errno || sync == end) return false; // Call failed
    params->sync_word = sync_p;
    return true;
}

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
    dprintf(radio_fd, "radio set wdt 0\n"); // Turn off the watchdog so our params don't reset with inactivity
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
