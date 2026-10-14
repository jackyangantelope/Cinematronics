// SPDX-License-Identifier: BSD-3-Clause
// Warrior sound approximates the MAME nl_warrior.cpp circuit reference.
#include "warrior_sound.h"

namespace jack::cinematronics {
namespace {
constexpr uint32_t step(uint32_t millihertz) {
    return static_cast<uint32_t>((uint64_t{1} << 32) * millihertz /
                                 (WarriorSound::sample_rate * 1000ull));
}
int32_t approach(int32_t current, int32_t target, int32_t divisor) {
    const int32_t delta = target - current;
    return current + (delta >= 0 ? (delta + divisor - 1) / divisor
                                : (delta - divisor + 1) / divisor);
}
int32_t triangle(uint32_t phase) {
    const int32_t ramp = static_cast<int32_t>(phase >> 16);
    return (ramp < 32768 ? ramp : 65535 - ramp) * 2 - 32768;
}
}

void WarriorSound::reset() { *this = WarriorSound{}; }

void WarriorSound::write(uint8_t port, uint8_t value) {
    if (port > 4) return;
    const uint8_t mask = static_cast<uint8_t>(1u << port);
    const uint8_t next = static_cast<uint8_t>((latch_ & ~mask) | ((value & 1u) << port));
    if (next == latch_) return;
    ++transitions_;
    const bool falling = (latch_ & mask) && !(next & mask);
    latch_ = next;

    if (falling && port == 2) {
        explosion_hold_ = sample_rate * 517u / 1000u;
        explosion_level_ = 32767;
    }
    if (falling && port == 3) { fall_age_ = 0; fall_phase_ = 0; }
    if (falling && port == 4) hiss_level_ = 32767;
}

int16_t WarriorSound::sample() {

    noise_clock_ += 10000;
    if (noise_clock_ >= sample_rate) {
        noise_clock_ -= sample_rate;
        noise_ ^= noise_ << 13;
        noise_ ^= noise_ >> 17;
        noise_ ^= noise_ << 5;
        noise_value_ = static_cast<int32_t>(noise_ >> 16) - 32768;
    }
    noise_low_ = approach(noise_low_, noise_value_, 44);
    noise_high_ = approach(noise_high_, noise_value_, 2);
    hum_phase_ += step(84000);
    beat_phase_ += step(1162);
    mod_phase_ += step(21140);

    const int32_t hum_target = !(latch_ & 2u) ? 7200 : !(latch_ & 1u) ? 3200 : 0;
    hum_level_ = approach(hum_level_, hum_target, 220);
    const int32_t tremolo = 24576 + triangle(beat_phase_) / 4;
    int32_t mixed = (triangle(hum_phase_) + noise_low_ / 2) * hum_level_ / 32768;
    mixed = mixed * tremolo / 32768;

    if (explosion_hold_) --explosion_hold_;
    else explosion_level_ = approach(explosion_level_, 0, 19845);
    mixed += noise_low_ * explosion_level_ / 32768;

    hiss_level_ = approach(hiss_level_, !(latch_ & 16u) ? 32767 : 0, 7422);
    mixed += (noise_value_ - noise_high_) * hiss_level_ / 65536;

    if (fall_age_ < sample_rate) {
        const uint32_t left = sample_rate - fall_age_++;
        const uint32_t frequency = 160u + 2600u * left / sample_rate;
        const int32_t modulation = triangle(mod_phase_) / 512;
        fall_phase_ += step(static_cast<uint32_t>(static_cast<int32_t>(frequency) + modulation) * 1000u);
        const int32_t level = static_cast<int32_t>(left * 8500u / sample_rate);
        mixed += triangle(fall_phase_) * level / 32768;
    }

    output_low_ = approach(output_low_, mixed, 3);
    output_dc_ = approach(output_dc_, output_low_, 2048);
    int32_t output = output_low_ - output_dc_;
    if (output > 32767) output = 32767;
    if (output < -32768) output = -32768;
    return static_cast<int16_t>(output);
}
}
