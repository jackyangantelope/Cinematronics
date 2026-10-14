// SPDX-License-Identifier: BSD-3-Clause
// Lightweight Warrior analog-board HLE. No external recordings are required.
#pragma once
#include <stdint.h>

namespace jack::cinematronics {
class WarriorSound {
public:
    static constexpr uint32_t sample_rate = 44100;
    void reset();
    void write(uint8_t port, uint8_t value);
    int16_t sample();
    uint8_t latch() const { return latch_; }
    uint32_t transitions() const { return transitions_; }
private:
    uint8_t latch_ = 0x1f;
    uint32_t transitions_ = 0;
    uint32_t noise_ = 0x6d2b79f5u, noise_clock_ = 0;
    uint32_t hum_phase_ = 0, beat_phase_ = 0, fall_phase_ = 0, mod_phase_ = 0;
    int32_t noise_value_ = 0, noise_low_ = 0, noise_high_ = 0;
    int32_t hum_level_ = 0, hiss_level_ = 0, explosion_level_ = 0;
    uint32_t explosion_hold_ = 0, fall_age_ = sample_rate;
    int32_t output_low_ = 0, output_dc_ = 0;
};
}
