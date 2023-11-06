#ifndef _RADIO_H
#define _RADIO_H

#include <stdbool.h>
#include <stdint.h>
#include <termios.h>

/** Represents the possible choices for modulation. */
typedef enum {
    /** Lora modulation. */
    LORA,
    /** FSK modulation. */
    FSK
} Modulation;

/** Represents the possible choices for coding rate. */
typedef enum {
    /** 4/5 */
    CR_4_5,
    /** 4/6 */
    CR_4_6,
    /** 4/7 */
    CR_4_7,
    /** 4/8 */
    CR_4_8,
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

void radio_setup_tty(struct termios *tty);
void radio_set_timeout(uint64_t timeout);
bool radio_set_params(int radio_fd, const struct lora_params_t *params);
bool wait_for_ok(int radio_fd);

#endif // _RADIO_H
