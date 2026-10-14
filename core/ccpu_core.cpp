// license:BSD-3-Clause
// copyright-holders:Aaron Giles
// Derived from MAME commit 6ae579aed3107c0b42c1c1c5cb05c02df4456eff.

#include "ccpu_core.h"

#include <string.h>

namespace jack::cinematronics {
namespace {

constexpr uint16_t kValueMask = 0x0fffu;
constexpr uint16_t kProgramMask = 0x7fffu;

int16_t sign_extend_12(uint16_t value)
{
    value &= kValueMask;
    return static_cast<int16_t>((value & 0x0800u) ? (value | 0xf000u) : value);
}

uint16_t arithmetic_shift_right_12(uint16_t value)
{
    return static_cast<uint16_t>((value >> 1) | (value & 0x0800u));
}

}

CcpuCore::CcpuCore(const CcpuCallbacks &callbacks) : callbacks_(callbacks)
{
    reset();
}

void CcpuCore::reset()
{
    memset(&state_, 0, sizeof(state_));
    state_.accumulator_is_a = 1;
    state_.compare_value = 1;
}

void CcpuCore::resume_frame()
{
    state_.waiting = 0;
}

void CcpuCore::watchdog_trigger()
{
    state_.waiting = 0;
    ++state_.watchdog;
    if (state_.watchdog >= 3)
        reset();
}

uint8_t CcpuCore::read_jmi() const
{
    return static_cast<uint8_t>((state_.mi_flag >> 11) & 1u);
}

uint16_t &CcpuCore::accumulator()
{
    return state_.accumulator_is_a ? state_.a : state_.b;
}

const uint16_t &CcpuCore::accumulator() const
{
    return state_.accumulator_is_a ? state_.a : state_.b;
}

void CcpuCore::next_accumulator_a()
{
    state_.next_next_mi_flag = accumulator();
    state_.accumulator_is_a = 1;
}

void CcpuCore::next_accumulator_b()
{
    state_.next_next_mi_flag = accumulator();
    state_.accumulator_is_a = state_.accumulator_is_a ? 0 : 1;
}

void CcpuCore::standard_accumulator_op(uint16_t result, uint16_t compare_value)
{
    state_.a0_flag = state_.a;
    state_.compare_acc = accumulator();
    state_.compare_value = compare_value & kValueMask;
    state_.nc_flag = static_cast<uint16_t>(~result);
    accumulator() = result & kValueMask;
}

uint8_t CcpuCore::read_program(uint16_t address) const
{
    return callbacks_.read_program
        ? callbacks_.read_program(callbacks_.context, address & kProgramMask)
        : 0;
}

uint16_t CcpuCore::read_data(uint16_t address) const
{
    return callbacks_.read_data
        ? callbacks_.read_data(callbacks_.context, address & kValueMask) & kValueMask
        : 0;
}

void CcpuCore::write_data(uint16_t address, uint16_t value) const
{
    if (callbacks_.write_data)
        callbacks_.write_data(callbacks_.context, address & kValueMask, value & kValueMask);
}

uint8_t CcpuCore::read_port(uint8_t port) const
{
    return callbacks_.read_port
        ? callbacks_.read_port(callbacks_.context, port & 0x1fu)
        : 0;
}

void CcpuCore::write_port(uint8_t port, uint8_t value) const
{
    if (callbacks_.write_port)
        callbacks_.write_port(callbacks_.context, port & 0x1fu, value);
}

uint8_t CcpuCore::external_input() const
{
    return callbacks_.external_input
        ? callbacks_.external_input(callbacks_.context)
        : 0;
}

void CcpuCore::emit_vector(int16_t start_x, int16_t start_y,
                           int16_t end_x, int16_t end_y, uint8_t shift) const
{
    if (callbacks_.vector)
        callbacks_.vector(callbacks_.context, start_x, start_y, end_x, end_y, shift);
}

int CcpuCore::run(int cycle_budget)
{
    if (cycle_budget <= 0 || state_.waiting)
        return 0;

    int cycles = cycle_budget;
    do {
        state_.mi_flag = state_.next_mi_flag;
        state_.next_mi_flag = state_.next_next_mi_flag;

        const uint8_t opcode = read_program(state_.pc++);
        if (opcode == 0x51 || opcode == 0x59)
            state_.external_input_value = external_input();

        uint16_t tempval = 0;
        const auto test_a0 = [this]() { return (state_.a0_flag & 1u) != 0; };
        const auto test_nc = [this]() { return ((state_.nc_flag >> 12) & 1u) != 0; };
        const auto test_lt = [this]() { return state_.compare_value < state_.compare_acc; };
        const auto test_eq = [this]() { return state_.compare_value == state_.compare_acc; };
        const auto test_dr = [this]() { return state_.dr_flag != 0; };
        const auto branch_target = [this]() {
            return static_cast<uint16_t>(((state_.pc - 1u) & 0xf000u) + state_.j);
        };

        switch (opcode) {
        case 0x00: case 0x01: case 0x02: case 0x03:
        case 0x04: case 0x05: case 0x06: case 0x07:
        case 0x08: case 0x09: case 0x0a: case 0x0b:
        case 0x0c: case 0x0d: case 0x0e: case 0x0f:
            tempval = static_cast<uint16_t>((opcode & 0x0fu) << 8);
            standard_accumulator_op(tempval, tempval);
            next_accumulator_a(); cycles -= 1;
            break;

        case 0x10: case 0x11: case 0x12: case 0x13:
        case 0x14: case 0x15: case 0x16: case 0x17:
        case 0x18: case 0x19: case 0x1a: case 0x1b:
        case 0x1c: case 0x1d: case 0x1e: case 0x1f:
            tempval = state_.accumulator_is_a
                ? read_port(opcode & 0x0fu) & 1u
                : read_port(static_cast<uint8_t>(16u + (opcode & 0x07u))) & 1u;
            standard_accumulator_op(tempval, tempval);
            next_accumulator_a(); cycles -= 1;
            break;

        case 0x20:
            tempval = read_program(state_.pc++);
            standard_accumulator_op(static_cast<uint16_t>(accumulator() + tempval), tempval);
            next_accumulator_a(); cycles -= 3;
            break;

        case 0x21: case 0x22: case 0x23: case 0x24:
        case 0x25: case 0x26: case 0x27: case 0x28:
        case 0x29: case 0x2a: case 0x2b: case 0x2c:
        case 0x2d: case 0x2e: case 0x2f:
            tempval = opcode & 0x0fu;
            standard_accumulator_op(static_cast<uint16_t>(accumulator() + tempval), tempval);
            next_accumulator_a(); cycles -= 1;
            break;

        case 0x30:
            tempval = read_program(state_.pc++);
            standard_accumulator_op(static_cast<uint16_t>(accumulator() + (tempval ^ kValueMask) + 1u), tempval);
            next_accumulator_a(); cycles -= 3;
            break;

        case 0x31: case 0x32: case 0x33: case 0x34:
        case 0x35: case 0x36: case 0x37: case 0x38:
        case 0x39: case 0x3a: case 0x3b: case 0x3c:
        case 0x3d: case 0x3e: case 0x3f:
            tempval = opcode & 0x0fu;
            standard_accumulator_op(static_cast<uint16_t>(accumulator() + (tempval ^ kValueMask) + 1u), tempval);
            next_accumulator_a(); cycles -= 1;
            break;

        case 0x40: case 0x41: case 0x42: case 0x43:
        case 0x44: case 0x45: case 0x46: case 0x47:
        case 0x48: case 0x49: case 0x4a: case 0x4b:
        case 0x4c: case 0x4d: case 0x4e: case 0x4f:
            tempval = read_program(state_.pc++);
            state_.j = static_cast<uint16_t>((opcode & 0x0fu) + (tempval & 0xf0u) + ((tempval & 0x0fu) << 8));
            next_accumulator_a(); cycles -= 3;
            break;

        case 0x50:
            state_.pc = static_cast<uint16_t>((state_.p << 12) + state_.j);
            next_accumulator_b(); cycles -= 4;
            break;
        case 0x51:
            if (state_.external_input_value) { state_.pc = branch_target(); cycles -= 2; }
            next_accumulator_b(); cycles -= 2;
            break;
        case 0x52:
            if (test_dr()) { state_.pc = branch_target(); cycles -= 2; }
            next_accumulator_b(); cycles -= 2;
            break;
        case 0x53:
            if (test_lt()) { state_.pc = branch_target(); cycles -= 2; }
            next_accumulator_b(); cycles -= 2;
            break;
        case 0x54:
            if (test_eq()) { state_.pc = branch_target(); cycles -= 2; }
            next_accumulator_b(); cycles -= 2;
            break;
        case 0x55:
            if (test_nc()) { state_.pc = branch_target(); cycles -= 2; }
            next_accumulator_b(); cycles -= 2;
            break;
        case 0x56:
            if (test_a0()) { state_.pc = branch_target(); cycles -= 2; }
            next_accumulator_b(); cycles -= 2;
            break;
        case 0x57:
            next_accumulator_b(); cycles -= 2;
            break;
        case 0x58:
            state_.pc = branch_target();
            next_accumulator_a(); cycles -= 4;
            break;
        case 0x59:
            if (state_.external_input_value) { state_.pc = branch_target(); cycles -= 2; }
            next_accumulator_a(); cycles -= 2;
            break;
        case 0x5a:
            if (test_dr()) { state_.pc = branch_target(); cycles -= 2; }
            next_accumulator_a(); cycles -= 2;
            break;
        case 0x5b:
            if (test_lt()) { state_.pc = branch_target(); cycles -= 2; }
            next_accumulator_a(); cycles -= 2;
            break;
        case 0x5c:
            if (test_eq()) { state_.pc = branch_target(); cycles -= 2; }
            next_accumulator_a(); cycles -= 2;
            break;
        case 0x5d:
            if (test_nc()) { state_.pc = branch_target(); cycles -= 2; }
            next_accumulator_a(); cycles -= 2;
            break;
        case 0x5e:
            if (test_a0()) { state_.pc = branch_target(); cycles -= 2; }
            next_accumulator_a(); cycles -= 2;
            break;
        case 0x5f:
            next_accumulator_a(); cycles -= 2;
            break;

        case 0x60: case 0x61: case 0x62: case 0x63:
        case 0x64: case 0x65: case 0x66: case 0x67:
        case 0x68: case 0x69: case 0x6a: case 0x6b:
        case 0x6c: case 0x6d: case 0x6e: case 0x6f:
            state_.i = static_cast<uint16_t>((state_.p << 4) + (opcode & 0x0fu));
            tempval = read_data(state_.i);
            standard_accumulator_op(static_cast<uint16_t>(accumulator() + tempval), tempval);
            next_accumulator_a(); cycles -= 3;
            break;

        case 0x70: case 0x71: case 0x72: case 0x73:
        case 0x74: case 0x75: case 0x76: case 0x77:
        case 0x78: case 0x79: case 0x7a: case 0x7b:
        case 0x7c: case 0x7d: case 0x7e: case 0x7f:
            state_.i = static_cast<uint16_t>((state_.p << 4) + (opcode & 0x0fu));
            tempval = read_data(state_.i);
            standard_accumulator_op(static_cast<uint16_t>(accumulator() + (tempval ^ kValueMask) + 1u), tempval);
            next_accumulator_a(); cycles -= 3;
            break;

        case 0x80: case 0x81: case 0x82: case 0x83:
        case 0x84: case 0x85: case 0x86: case 0x87:
        case 0x88: case 0x89: case 0x8a: case 0x8b:
        case 0x8c: case 0x8d: case 0x8e: case 0x8f:
            state_.p = opcode & 0x0fu;
            next_accumulator_a(); cycles -= 1;
            break;

        case 0x90: case 0x91: case 0x92: case 0x93:
        case 0x94: case 0x95: case 0x96: case 0x97:
        case 0x98: case 0x99: case 0x9a: case 0x9b:
        case 0x9c: case 0x9d: case 0x9e: case 0x9f:
            if (state_.accumulator_is_a)
                write_port(opcode & 0x07u, static_cast<uint8_t>(~accumulator() & 1u));
            next_accumulator_a(); cycles -= 1;
            break;

        case 0xa0: case 0xa1: case 0xa2: case 0xa3:
        case 0xa4: case 0xa5: case 0xa6: case 0xa7:
        case 0xa8: case 0xa9: case 0xaa: case 0xab:
        case 0xac: case 0xad: case 0xae: case 0xaf:
            state_.i = static_cast<uint16_t>((state_.p << 4) + (opcode & 0x0fu));
            tempval = read_data(state_.i);
            standard_accumulator_op(tempval, tempval);
            next_accumulator_a(); cycles -= 3;
            break;

        case 0xb0: case 0xb1: case 0xb2: case 0xb3:
        case 0xb4: case 0xb5: case 0xb6: case 0xb7:
        case 0xb8: case 0xb9: case 0xba: case 0xbb:
        case 0xbc: case 0xbd: case 0xbe: case 0xbf: {
            state_.i = static_cast<uint16_t>((state_.p << 4) + (opcode & 0x0fu));
            tempval = read_data(state_.i);
            const uint16_t result = static_cast<uint16_t>(accumulator() + (tempval ^ kValueMask) + 1u);
            state_.a0_flag = state_.a;
            state_.compare_acc = accumulator();
            state_.compare_value = tempval & kValueMask;
            state_.nc_flag = static_cast<uint16_t>(~result);
            state_.next_next_mi_flag = result;
            next_accumulator_a(); cycles -= 3;
            break;
        }

        case 0xc0: case 0xc1: case 0xc2: case 0xc3:
        case 0xc4: case 0xc5: case 0xc6: case 0xc7:
        case 0xc8: case 0xc9: case 0xca: case 0xcb:
        case 0xcc: case 0xcd: case 0xce: case 0xcf:
            state_.i = static_cast<uint16_t>((state_.p << 4) + (opcode & 0x0fu));
            state_.i = read_data(state_.i) & 0x00ffu;
            next_accumulator_a(); cycles -= 3;
            break;

        case 0xd0: case 0xd1: case 0xd2: case 0xd3:
        case 0xd4: case 0xd5: case 0xd6: case 0xd7:
        case 0xd8: case 0xd9: case 0xda: case 0xdb:
        case 0xdc: case 0xdd: case 0xde: case 0xdf:
            state_.i = static_cast<uint16_t>((state_.p << 4) + (opcode & 0x0fu));
            write_data(state_.i, accumulator());
            next_accumulator_a(); cycles -= 3;
            break;

        case 0xe0: {
            const int16_t start_x = static_cast<int16_t>(state_.x);
            const int16_t start_y = static_cast<int16_t>(state_.y);
            int16_t stop_x = sign_extend_12(state_.a);
            int16_t stop_y = sign_extend_12(state_.b);
            stop_x = static_cast<int16_t>((static_cast<int32_t>(stop_x) - start_x) >> state_.t) + start_x;
            stop_y = static_cast<int16_t>((static_cast<int32_t>(stop_y) - start_y) >> state_.t) + start_y;
            emit_vector(start_x, start_y, stop_x, stop_y, static_cast<uint8_t>(state_.t));
            state_.a = state_.x & kValueMask;
            state_.b = state_.y & kValueMask;
            next_accumulator_a(); cycles -= 1;
            break;
        }

        case 0xe1:
            state_.j = read_data(state_.i);
            next_accumulator_a(); cycles -= 3;
            break;
        case 0xf1:
            state_.i = read_data(state_.i) & 0x00ffu;
            next_accumulator_a(); cycles -= 3;
            break;

        case 0xe2: case 0xf2:
            tempval = read_program(static_cast<uint16_t>(((state_.pc - 1u) & 0xf000u) + accumulator()));
            standard_accumulator_op(tempval, tempval);
            next_accumulator_a(); cycles -= 7;
            ++state_.pc;
            break;

        case 0xe3: case 0xf3: {
            tempval = read_data(state_.i);
            state_.a0_flag = state_.a;
            state_.compare_value = tempval & kValueMask;
            if (state_.accumulator_is_a) {
                if (state_.a & 1u) {
                    state_.compare_acc = state_.b;
                    state_.a = static_cast<uint16_t>((state_.a >> 1) | ((state_.b << 11) & 0x0800u));
                    state_.b = arithmetic_shift_right_12(state_.b);
                    const uint16_t result = static_cast<uint16_t>(state_.b + tempval);
                    state_.nc_flag = static_cast<uint16_t>(~result);
                    state_.next_next_mi_flag = result;
                    state_.b = result & kValueMask;
                } else {
                    state_.compare_acc = state_.a;
                    const uint16_t result = static_cast<uint16_t>(state_.a + tempval);
                    state_.a = static_cast<uint16_t>((state_.a >> 1) | ((state_.b << 11) & 0x0800u));
                    state_.b = arithmetic_shift_right_12(state_.b);
                    state_.nc_flag = static_cast<uint16_t>(~result);
                    state_.next_next_mi_flag = result;
                }
            } else {
                state_.compare_acc = state_.b;
                state_.b = arithmetic_shift_right_12(state_.b);
                const uint16_t result = static_cast<uint16_t>(state_.b + tempval);
                state_.nc_flag = static_cast<uint16_t>(~result);
                state_.next_next_mi_flag = result;
                if (state_.a & 1u)
                    state_.b = result & kValueMask;
            }
            next_accumulator_a(); cycles -= 2;
            break;
        }

        case 0xe4: case 0xf4:
            state_.t = 0;
            while (((state_.a & 0x0a00u) == 0 || (state_.a & 0x0a00u) == 0x0a00u) &&
                   ((state_.b & 0x0a00u) == 0 || (state_.b & 0x0a00u) == 0x0a00u) &&
                   state_.t < 16) {
                state_.a = static_cast<uint16_t>((state_.a << 1) & kValueMask);
                state_.b = static_cast<uint16_t>((state_.b << 1) & kValueMask);
                ++state_.t;
                --cycles;
            }
            next_accumulator_a(); cycles -= 1;
            break;

        case 0xe5: case 0xf5:
            state_.waiting = 1;
            next_accumulator_a();
            cycles = -1;
            if (read_program(state_.pc) == opcode)
                ++state_.pc;
            break;

        case 0xe6: case 0xf6:
            write_data(state_.i, accumulator());
            next_accumulator_a(); cycles -= 2;
            break;

        case 0xf7:
            state_.watchdog = 0;
            [[fallthrough]];
        case 0xe7:
            tempval = read_data(state_.i);
            standard_accumulator_op(static_cast<uint16_t>(accumulator() + tempval), tempval);
            next_accumulator_a(); cycles -= 2;
            break;

        case 0xe8: case 0xf8:
            tempval = read_data(state_.i);
            standard_accumulator_op(static_cast<uint16_t>(accumulator() + (tempval ^ kValueMask) + 1u), tempval);
            next_accumulator_a(); cycles -= 3;
            break;
        case 0xe9: case 0xf9:
            tempval = read_data(state_.i);
            standard_accumulator_op(static_cast<uint16_t>(accumulator() & tempval), tempval);
            next_accumulator_a(); cycles -= 2;
            break;
        case 0xea: case 0xfa:
            tempval = read_data(state_.i);
            standard_accumulator_op(tempval, tempval);
            next_accumulator_a(); cycles -= 2;
            break;

        case 0xeb: case 0xfb:
            tempval = state_.accumulator_is_a ? (state_.a >> 1) : arithmetic_shift_right_12(state_.b);
            tempval |= static_cast<uint16_t>((accumulator() + (0x0b0bu | (opcode & 0xf0u))) & 0x1000u);
            standard_accumulator_op(tempval, static_cast<uint16_t>(0x0b0bu | (opcode & 0xf0u)));
            next_accumulator_a(); cycles -= 1;
            break;
        case 0xec: case 0xfc:
            tempval = static_cast<uint16_t>((accumulator() << 1) & kValueMask);
            tempval |= static_cast<uint16_t>((accumulator() + (0x0c0cu | (opcode & 0xf0u))) & 0x1000u);
            standard_accumulator_op(tempval, static_cast<uint16_t>(0x0c0cu | (opcode & 0xf0u)));
            next_accumulator_a(); cycles -= 1;
            break;
        case 0xed: case 0xfd:
            tempval = arithmetic_shift_right_12(accumulator());
            standard_accumulator_op(tempval, static_cast<uint16_t>(0x0d0du | (opcode & 0xf0u)));
            next_accumulator_a(); cycles -= 1;
            break;

        case 0xee: case 0xfe:
            if (state_.accumulator_is_a) {
                tempval = static_cast<uint16_t>((state_.a >> 1) | ((state_.b << 11) & 0x0800u));
                state_.b = arithmetic_shift_right_12(state_.b);
            } else {
                tempval = arithmetic_shift_right_12(state_.b);
            }
            tempval |= static_cast<uint16_t>((accumulator() + (0x0e0eu | (opcode & 0xf0u))) & 0x1000u);
            standard_accumulator_op(tempval, static_cast<uint16_t>(0x0e0eu | (opcode & 0xf0u)));
            next_accumulator_a(); cycles -= 1;
            break;

        case 0xef: case 0xff:
            if (state_.accumulator_is_a) {
                tempval = static_cast<uint16_t>((state_.a << 1) & kValueMask);
                state_.b = static_cast<uint16_t>((state_.b << 1) & kValueMask);
            } else {
                tempval = static_cast<uint16_t>((state_.b << 1) & kValueMask);
            }
            tempval |= static_cast<uint16_t>((accumulator() + (0x0f0fu | (opcode & 0xf0u))) & 0x1000u);
            standard_accumulator_op(tempval, static_cast<uint16_t>(0x0f0fu | (opcode & 0xf0u)));
            next_accumulator_a(); cycles -= 1;
            break;

        case 0xf0:
            state_.x = static_cast<uint16_t>(sign_extend_12(state_.a));
            state_.y = static_cast<uint16_t>(sign_extend_12(state_.b));
            next_accumulator_a(); cycles -= 1;
            break;
        }
    } while (cycles > 0);

    return cycle_budget - (cycles > 0 ? cycles : 0);
}

}
