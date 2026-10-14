// SPDX-License-Identifier: BSD-3-Clause
#pragma once
#include "cinematronics_profile.h"
#include "miniz.h"
namespace jack::cinematronics {
struct RomSource {
    void *context=nullptr;
    size_t (*read)(void *,uint32_t,void *,size_t)=nullptr;
    uint32_t size=0;
};

class CineRomLoader {
public:
    char error[96]{};
    bool load(const GameProfile &game,const RomSource &source,bool zip,uint8_t *out,
              const RomSource *parent=nullptr);
private:
    uint8_t packed[16384]{},chip[8192]{};
    tinfl_decompressor inflator{};
    bool fail(const char *message);
    bool extract(const RomSource &source,const RomChip &expected);
};
}
