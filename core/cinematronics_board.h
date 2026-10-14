// SPDX-License-Identifier: BSD-3-Clause
#pragma once
#include "cinematronics_profile.h"
#include "ccpu_core.h"
namespace jack::cinematronics {
struct CineControls {
    uint16_t pads[2]{};
    bool keys[128]{};
    int16_t mouse_dx=0, mouse_dy=0;
};
class CineBoard {
public:
    static constexpr uint32_t clock_hz=4980750, cycles_per_frame=131072;
    uint8_t rom[32768]{};
    uint16_t ram[1024]{};
    const GameProfile *profile=nullptr;
    CcpuCore *cpu=nullptr;
    uint16_t inputs=0xffff, color=0x7fff;
    uint8_t switches=0xff, latch=0, ram_bank=0;
    bool coin=false;
    int analog_x=0x800, analog_y=0x800;
    uint8_t dial=0, gear=0;
    uint16_t saved_x=0,saved_y=0;
    void reset(const GameProfile &p);
    void controls(const CineControls &controls);

    uint8_t read_program(uint16_t address) const {
        return rom[program_pages[(address >> 10) & 31u] + (address & 1023u)];
    }
    uint16_t read_data(uint16_t address) const;
    void write_data(uint16_t address,uint16_t value);
    uint8_t read_port(uint8_t port) const;
    void write_port(uint8_t port,uint8_t value);
    uint8_t external_input() const;
    void transform(int16_t &x,int16_t &y) const;
private:
    uint16_t program_pages[32]{};
    uint16_t sundance_pads[2]{};
    int wheel=0;
};
}
