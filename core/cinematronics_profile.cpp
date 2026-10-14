// SPDX-License-Identifier: BSD-3-Clause
#include "cinematronics_profile.h"
#include <string.h>
namespace jack::cinematronics {
#include "cinematronics_profiles.inc"
const GameProfile *find_game(const char *filename) {
    char name[32]{};
    const char *base = filename;
    for (const char *p = filename; *p; ++p) if (*p == '/' || *p == '\\') base = p + 1;
    size_t n = 0;
    while (base[n] && base[n] != '.') {
        if (n + 1 == sizeof(name)) return nullptr;
        name[n] = base[n] >= 'A' && base[n] <= 'Z' ? base[n] + ('a'-'A') : base[n]; ++n;
    }
    for (size_t i = 0; i < game_profile_count; ++i)
        if (strcmp(name, game_profiles[i].id) == 0) return &game_profiles[i];
    return nullptr;
}
uint32_t cine_crc32(const uint8_t *data, size_t size) {
    uint32_t crc = 0xffffffff;
    for (size_t i=0;i<size;++i) {
        crc ^= data[i];
        for (unsigned b=0;b<8;++b) crc=(crc>>1)^((crc&1)?0xedb88320u:0u);
    }
    return ~crc;
}
uint32_t program_offset(const GameProfile &p, uint16_t address) {
    if (p.rom_size == 8192) return (address&0xfff) | ((address&0x2000)>>1);
    return address & (p.rom_size-1u);
}
}
