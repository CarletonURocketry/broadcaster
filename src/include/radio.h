/**
 * @file radio.h
 * @brief This file contains the types necessary for the LoRa radio parameters, as well as function prototypes for all
 * functions found in radio.c.
 *
 * Parameters that have limited string options are encoded as enums. The LoRa radio parameters are represented as a
 * struct to have one collection of all the required parameters.
 */
#ifndef _RADIO_H_
#define _RADIO_H_

#include <stdbool.h>
#include <stdint.h>
#include <termios.h>

/** Represents the possible choices for modulation. */
typedef enum {
    LORA, /**< Lora modulation. */
    FSK,  /**< FSK modulation. */
} Modulation;

/** Represents the possible choices for coding rate. */
typedef enum {
    CR_4_5, /**< 4/5 */
    CR_4_6, /**< 4/6 */
    CR_4_7, /**< 4/7 */
    CR_4_8, /**< 4/8 */
} CodingRate;

/** Contains the parameters for the RN2483 LoRa radio module. */
struct lora_params_t {
    /** Can either be lora or fsk. */
    Modulation modulation;
    /** Frequency in Hz. From 433,000,000-434,800,000 or 863,000,000-870,000,000 */
    uint32_t frequency;
    /** Transceiver output power, from -3 to 15. */
    int8_t power;
    /** Spreading factor from 7-12. */
    uint8_t spread_factor;
    /** Coding rate can be one of: 4/5, 4/6, 4/7, 4/8. */
    CodingRate coding_rate;
    /** Radio bandwidth in kHz, which can be one of: 125, 250, 500. */
    uint16_t bandwidth;
    /** The preamble length from 0-65535. */
    uint16_t preamble_len;
    /** Whether or not to add a cyclic redundancy header. */
    bool cyclic_redundancy;
    /** State of the invert IQ. */
    bool iqi;
    /** The sync word for communication. Lora modulation uses one byte, FSK uses up to eight. */
    uint64_t sync_word;
};

/* PARAMETER VALIDATION. */
bool radio_validate_mod(const char *mod, struct lora_params_t *params);
bool radio_validate_freq(const char *freq, struct lora_params_t *params);
bool radio_validate_pwr(const char *power, struct lora_params_t *params);
bool radio_validate_sf(const char *power, struct lora_params_t *params);
bool radio_validate_cr(const char *coding_rate, struct lora_params_t *params);
bool radio_validate_prlen(const char *prlen, struct lora_params_t *params);
bool radio_validate_bw(const char *bandwidth, struct lora_params_t *params);
bool radio_validate_sync(const char *sync, struct lora_params_t *params);

/* RADIO SETUP. */
void radio_setup_tty(struct termios *tty);
bool radio_set_params(int radio_fd, const struct lora_params_t *params);

/* RADIO COMMUNICATION */
bool wait_for_ok(int radio_fd);
bool radio_tx(int radio_fd, const char *data);
bool radio_tx_bytes(int radio_fd, const uint8_t *data, size_t nbytes);

#endif // _RADIO_H_
