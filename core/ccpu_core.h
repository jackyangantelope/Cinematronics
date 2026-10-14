// license:BSD-3-Clause
// Based on MAME's Cinematronics CCPU core by Aaron Giles.

#ifndef CINEMATRONICS_CCPU_CORE_H
#define CINEMATRONICS_CCPU_CORE_H

#include <stdint.h>

namespace jack::cinematronics {

struct CcpuCallbacks {
    void *context;
    uint8_t (*read_program)(void *context, uint16_t address);
    uint16_t (*read_data)(void *context, uint16_t address);
    void (*write_data)(void *context, uint16_t address, uint16_t value);
    uint8_t (*read_port)(void *context, uint8_t port);
    void (*write_port)(void *context, uint8_t port, uint8_t value);
    uint8_t (*external_input)(void *context);
    void (*vector)(void *context, int16_t start_x, int16_t start_y,
                   int16_t end_x, int16_t end_y, uint8_t shift);
};

struct CcpuState {
    uint16_t pc;
    uint16_t a;
    uint16_t b;
    uint16_t i;
    uint16_t j;
    uint16_t p;
    uint16_t x;
    uint16_t y;
    uint16_t t;
    uint16_t a0_flag;
    uint16_t nc_flag;
    uint16_t compare_acc;
    uint16_t compare_value;
    uint16_t mi_flag;
    uint16_t next_mi_flag;
    uint16_t next_next_mi_flag;
    uint16_t dr_flag;
    uint8_t accumulator_is_a;
    uint8_t waiting;
    uint8_t watchdog;
    uint8_t external_input_value;
};

class CcpuCore {
public:
    explicit CcpuCore(const CcpuCallbacks &callbacks);

    void reset();
    int run(int cycle_budget);
    void resume_frame();
    void watchdog_trigger();

    const CcpuState &state() const { return state_; }
    uint8_t read_jmi() const;

    void restore_vector_position(uint16_t x, uint16_t y) { state_.x = x; state_.y = y; }

private:
    uint16_t &accumulator();
    const uint16_t &accumulator() const;
    void next_accumulator_a();
    void next_accumulator_b();
    void standard_accumulator_op(uint16_t result, uint16_t compare_value);

    uint8_t read_program(uint16_t address) const;
    uint16_t read_data(uint16_t address) const;
    void write_data(uint16_t address, uint16_t value) const;
    uint8_t read_port(uint8_t port) const;
    void write_port(uint8_t port, uint8_t value) const;
    uint8_t external_input() const;
    void emit_vector(int16_t start_x, int16_t start_y,
                     int16_t end_x, int16_t end_y, uint8_t shift) const;

    CcpuCallbacks callbacks_;
    CcpuState state_;
};

}

#endif
