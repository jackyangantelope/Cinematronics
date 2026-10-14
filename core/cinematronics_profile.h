// SPDX-License-Identifier: BSD-3-Clause
#pragma once
#include <stddef.h>
#include <stdint.h>
namespace jack::cinematronics {
enum class Machine : uint8_t {
    spacewar, barrier, speedfrk, starhawk, sundance, tailg, warrior,
    armora, ripoff, starcas, solarq, boxingb, wotw, demon, qb3
};
enum class VectorMode : uint8_t { mono, gray16, gray64, rgb444, rgb233 };
struct RomChip { const char *name; uint32_t crc; uint16_t offset, size; int8_t nibble; };
struct InputBinding {
    uint16_t mask, pad;
    uint8_t port, player, key;
};
struct GameProfile {
    const char *id, *title, *parent;
    Machine machine;
    VectorMode video;
    uint16_t rom_size, width, height, input_default;
    uint8_t switches;
    bool jmi;
    uint8_t orientation;
    bool warrior4j;
    const RomChip *chips;
    uint8_t chip_count;
    const InputBinding *bindings;
    uint8_t binding_count;
};
extern const GameProfile game_profiles[];
extern const size_t game_profile_count;
const GameProfile *find_game(const char *filename);
uint32_t cine_crc32(const uint8_t *data, size_t size);
uint32_t program_offset(const GameProfile &profile, uint16_t address);
}
