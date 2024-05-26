/**
 * @file radio.c
 * @brief Contains functions for configuring the LoRa radio, and constants for valid radio parameters.
 *
 * This file contains implementations of functions for setting the LoRa radio parameters. It also contains functions for
 * configuring the UART connection to the LoRa radio module.
 *
 * There are several functions included for parsing command line arguments for radio parameters into their correct types
 * and validating them.
 */
#include "radio.h"
#include <devctl.h>
#include <errno.h>
#include <fcntl.h>
#include <ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

/** Macro to calculate compile time array length. */
#define array_len(a) (sizeof(a) / sizeof(a[0]))

/** Macro to early return errors. */
#define return_err(err)                                                                                                \
    if (err != EOK) return err;

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
static const uint8_t L_SF = 7;
/** Upper limit for spread factor. */
static const uint8_t H_SF = 12;

/** Valid bandwidth choices. */
static const uint16_t BANDWIDTHS[] = {125, 250, 500};

/**
 * Validates and sets a command line argument for modulation.
 * @param mod The command line argument for modulation.
 * @param params The LoRa parameters to be updated.
 * @return 0 if valid, EINVAL if invalid.
 */
int radio_validate_mod(const char *mod, struct lora_params_t *params) {
    for (uint8_t i = 0; i < array_len(MODULATIONS); i++) {
        if (!strcmp(mod, MODULATIONS[i])) {
            params->modulation = (Modulation)i;
            return 0;
        }
    }
    return EINVAL;
}

/**
 * Validates and sets a command line argument for frequency.
 * @param freq The command line argument for frequency.
 * @param params The LoRa parameters to be updated.
 * @return 0 if valid, EINVAL if invalid.
 */
int radio_validate_freq(const char *freq, struct lora_params_t *params) {
    char *end;
    uint32_t p_freq = strtoul(freq, &end, 10);
    if (errno || freq == end) return EINVAL;

    if (!(LL_FREQ <= p_freq && p_freq <= LH_FREQ) && !(HL_FREQ <= p_freq && p_freq <= HH_FREQ)) return false;

    params->frequency = p_freq;
    return 0;
}

/**
 * Validates and sets a command line argument for power.
 * @param pwr The command line argument for power.
 * @param params The LoRa parameters to be updated.
 * @return 0 if valid, EINVAL if invalid.
 */
int radio_validate_pwr(const char *power, struct lora_params_t *params) {
    int8_t power_p = atoi(power);
    if (L_PWR <= power_p && power_p <= H_PWR) {
        params->power = power_p;
        return 0;
    }
    return EINVAL;
}

/**
 * Validates and sets a command line argument for spread factor.
 * @param sf The command line argument for spread factor.
 * @param params The LoRa parameters to be updated.
 * @return 0 if valid, EINVAL if invalid.
 */
int radio_validate_sf(const char *sf, struct lora_params_t *params) {
    char *end;
    uint8_t sf_p = strtoul(sf, &end, 10);
    if (errno || sf == end) return EINVAL;

    if (L_SF <= sf_p && sf_p <= H_SF) {
        params->spread_factor = sf_p;
        return 0;
    }
    return EINVAL;
}

/**
 * Validates and sets a command line argument for coding rate.
 * @param coding_rate The command line argument for coding rate.
 * @param params The LoRa parameters to be updated.
 * @return 0 if valid, EINVAL if invalid.
 */
int radio_validate_cr(const char *coding_rate, struct lora_params_t *params) {
    for (uint8_t i = 0; i < array_len(CODING_RATES); i++) {
        if (!strcmp(coding_rate, CODING_RATES[i])) {
            params->coding_rate = (CodingRate)i;
            return 0;
        }
    }
    return EINVAL;
}

/**
 * Validates and sets a command line argument for preamble length.
 * @param prlen The command line argument for preamble length.
 * @param params The LoRa parameters to be updated.
 * @return 0 if valid, EINVAL if invalid.
 */
int radio_validate_prlen(const char *prlen, struct lora_params_t *params) {
    char *end;
    uint16_t prlen_p = strtoul(prlen, &end, 10);
    if (errno || prlen == end) return EINVAL;

    params->preamble_len = prlen_p;
    return 0;
}

/**
 * Validates and sets a command line argument for bandwidth.
 * @param bandwidth The command line argument for bandwidth.
 * @param params The LoRa parameters to be updated.
 * @return 0 if valid, EINVAL if invalid.
 */
int radio_validate_bw(const char *bandwidth, struct lora_params_t *params) {
    char *end;
    uint16_t bw_p = strtoul(bandwidth, &end, 10);
    if (errno || bandwidth == end) return false; // Call failed

    // Check if it's an option
    for (uint8_t i = 0; i < array_len(BANDWIDTHS); i++) {
        if (bw_p == BANDWIDTHS[i]) {
            params->bandwidth = bw_p;
            return EINVAL;
        }
    }
    return 0;
}

/**
 * Validates and sets a command line argument for sync word.
 * @param sync The command line argument for sync word.
 * @param params The LoRa parameters to be updated.
 * @return 0 if valid, EINVAL if invalid.
 */
int radio_validate_sync(const char *sync, struct lora_params_t *params) {
    char *end;
    uint64_t sync_p = strtoul(sync, &end, 10);
    if (errno || sync == end) return EINVAL;
    params->sync_word = sync_p;
    return 0;
}

/**
 * Set parameters on the LoRa radio.
 * @param radio_fd The file descriptor to the LoRa radio device.
 * @param params A pointer to the struct of LoRa radio parameters to be set.
 * @return 0 if successful, otherwise the type of error that occurred.
 */
int radio_set_params(int radio_fd, const struct lora_params_t *params) {

    int err;

    if (dprintf(radio_fd, "radio set mod %s\n", MODULATIONS[params->modulation]) < 0) return errno;
    err = tcdrain(radio_fd);
    return_err(err);
    err = wait_for_ok(radio_fd);
    return_err(err);

    if (dprintf(radio_fd, "radio set freq %u\n", params->frequency) < 0) return errno;
    err = tcdrain(radio_fd);
    return_err(err);
    err = wait_for_ok(radio_fd);
    return_err(err);

    if (dprintf(radio_fd, "radio set pwr %d\n", params->power) < 0) return errno;
    err = tcdrain(radio_fd);
    return_err(err);
    err = wait_for_ok(radio_fd);
    return_err(err);

    if (dprintf(radio_fd, "radio set sf sf%u\n", params->spread_factor) < 0) return errno;
    err = tcdrain(radio_fd);
    return_err(err);
    err = wait_for_ok(radio_fd);
    return_err(err);

    if (dprintf(radio_fd, "radio set cr %s\n", CODING_RATES[params->coding_rate]) < 0) return errno;
    err = tcdrain(radio_fd);
    return_err(err);
    err = wait_for_ok(radio_fd);
    return_err(err);

    if (dprintf(radio_fd, "radio set bw %u\n", params->bandwidth) < 0) return errno;
    err = tcdrain(radio_fd);
    return_err(err);
    err = wait_for_ok(radio_fd);
    return_err(err);

    if (dprintf(radio_fd, "radio set prlen %u\n", params->preamble_len) < 0) return errno;
    err = tcdrain(radio_fd);
    return_err(err);
    err = wait_for_ok(radio_fd);
    return_err(err);

    if (dprintf(radio_fd, "radio set crc %s\n", params->cyclic_redundancy ? "on" : "off") < 0) return errno;
    err = tcdrain(radio_fd);
    return_err(err);
    err = wait_for_ok(radio_fd);
    return_err(err);

    if (dprintf(radio_fd, "radio set iqi %s\n", params->iqi ? "on" : "off") < 0) return errno;
    err = tcdrain(radio_fd);
    return_err(err);
    err = wait_for_ok(radio_fd);
    return_err(err);

    if (dprintf(radio_fd, "radio set sync %lx\n", params->sync_word) < 0) return errno;
    err = tcdrain(radio_fd);
    return_err(err);
    err = wait_for_ok(radio_fd);
    return_err(err);

    // Turn off the watchdog so our params don't reset with inactivity
    if (dprintf(radio_fd, "radio set wdt 0\n") < 0) return errno;
    err = tcdrain(radio_fd);
    return_err(err);
    err = wait_for_ok(radio_fd);
    return_err(err);

    // Mac pause will pause for 4294967245ms, or about 49 days. So we'll only do this once.
    if (dprintf(radio_fd, "mac pause\n") < 0) return errno; // Mac pause to not reset parameters
    err = tcdrain(radio_fd);                                // Wait for radio to process mac pause command
    return_err(err);

    // Check that mac pause returned non-0 (success)
    char buffer[10] = {0};
    read(radio_fd, buffer, sizeof(buffer));
    if (!strcmp(buffer, "0")) return EIO;

    return 0;
}

/**
 * Wait for the LoRa radio module to respond with "ok".
 * @param radio_fd The file descriptor to the LoRa radio device.
 * @return 0 if The radio responded with an okay status, otherwise return error that occurred.
 */
int wait_for_ok(int radio_fd) {
    char buffer[25] = {0};
    for (uint8_t i = 0; i < 3; i++) {
        read(radio_fd, buffer, sizeof(buffer));
        if (strstr(buffer, "ok") != NULL) {
            return tcflush(radio_fd, TCIFLUSH);
        }
        memset(buffer, 0, sizeof(buffer));
    }
    return EIO;
}

/**
 * Transmits the passed data over LoRa radio.
 * @param radio_fd The file descriptor to the LoRa radio device.
 * @param data A pointer to the data to be sent over radio.
 * @return 0 if transmission was successful, otherwise return the error that occurred.
 */
int radio_tx(int radio_fd, const char *data) {
    int err;
    if (dprintf(radio_fd, "radio tx %s\n", data) < 0) return errno; // Transmit data
    err = tcdrain(radio_fd);                                        // Wait for radio to process transmit request
    return_err(err);
    return wait_for_ok(radio_fd); // Return result of radio response
}

/**
 * Transmits the passed binary data over LoRa radio.
 * @param radio_fd The file descriptor to the LoRa radio device.
 * @param data A pointer to the data to be sent over radio.
 * @param nbytes The number of bytes in the `data` pointer to transmit.
 * @return 0 if transmission was successful, otherwise return the error that occurred.
 */
int radio_tx_bytes(int radio_fd, const uint8_t *data, size_t nbytes) {
    int err;

    // Send all bytes as hex
    if (dprintf(radio_fd, "radio tx ") < 0) return errno;
    for (size_t i = 0; i < nbytes; i++) {
        if (dprintf(radio_fd, "%02x", data[i]) < 0) return errno;
    }
    if (dprintf(radio_fd, "\n") < 0) return errno;
    err = tcdrain(radio_fd); // Wait for radio to process transmit request
    return_err(err);
    return wait_for_ok(radio_fd); // Return result of radio response
}
