// SPDX-License-Identifier: BSD-3-Clause
#include "cinematronics_rom.h"
#include <stdio.h>
#include <string.h>
#include <algorithm>
namespace jack::cinematronics {
namespace {
uint16_t u16(const uint8_t *p) { return static_cast<uint16_t>(p[0]|(p[1]<<8)); }
uint32_t u32(const uint8_t *p) { return u16(p)|(uint32_t(u16(p+2))<<16); }
bool read(const RomSource &s,uint32_t at,void *out,size_t n) {
    return s.read && at<=s.size && n<=s.size-at && s.read(s.context,at,out,n)==n;
}
}
bool CineRomLoader::fail(const char *message) { snprintf(error,sizeof(error),"%s",message); return false; }
bool CineRomLoader::extract(const RomSource &s,const RomChip &expected) {
    if(s.size<22 || s.size>4u*1024*1024) return fail("ZIP size limit (4 MiB)");

    const uint32_t tail=std::min<uint32_t>(s.size,8192u);
    if(!read(s,s.size-tail,packed,tail)) return fail("ZIP tail read failed");
    int end=-1;
    for(int p=static_cast<int>(tail)-22;p>=0;--p)
        if(u32(packed+p)==0x06054b50 && p+22u+u16(packed+p+20)==tail) { end=p; break; }
    if(end<0) return fail("ZIP directory not found");
    const uint8_t *e=packed+end;
    const unsigned count=u16(e+10);
    const uint32_t bytes=u32(e+12),start=u32(e+16),limit=s.size-tail+static_cast<uint32_t>(end);
    if(u16(e+4)||u16(e+6)||u16(e+8)!=count||count>128||bytes>65536||start>limit||bytes>limit-start)
        return fail("ZIP directory limits/format");
    uint32_t at=start;
    for(unsigned n=0;n<count;++n) {
        uint8_t h[46];
        if(at>start+bytes || sizeof(h)>start+bytes-at || !read(s,at,h,sizeof(h)) || u32(h)!=0x02014b50)
            return fail("Invalid ZIP directory entry");
        const uint32_t record=46u+u16(h+28)+u16(h+30)+u16(h+32);
        if(record>start+bytes-at) return fail("Truncated ZIP directory");
        at+=record;
        if(u32(h+16)!=expected.crc || u32(h+24)!=expected.size) continue;
        const unsigned method=u16(h+10),flags=u16(h+8);
        const uint32_t compressed=u32(h+20),local=u32(h+42);
        if((flags&1)||u16(h+34)|| (method!=0 && method!=8) || compressed>sizeof(packed))
            return fail("Unsupported ZIP compression");
        uint8_t lh[30];
        if(!read(s,local,lh,sizeof(lh)) || u32(lh)!=0x04034b50 || u16(lh+8)!=method || (u16(lh+6)&1))
            return fail("Invalid ZIP local header");
        const uint64_t data=uint64_t(local)+30u+u16(lh+26)+u16(lh+28);
        if(data>s.size || compressed>s.size-data || !read(s,static_cast<uint32_t>(data),packed,compressed))
            return fail("ZIP data read failed");
        if(method==0) {
            if(compressed!=expected.size) return fail("ZIP stored size mismatch");
            memcpy(chip,packed,compressed);
        } else {
            tinfl_init(&inflator);
            size_t in=compressed,out=expected.size;
            const auto status=tinfl_decompress(&inflator,packed,&in,chip,chip,&out,TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
            if(status!=TINFL_STATUS_DONE || in!=compressed || out!=expected.size) return fail("ZIP inflate failed");
        }
        if(cine_crc32(chip,expected.size)!=expected.crc) return fail("ROM chip CRC mismatch");
        return true;
    }
    return fail("Required ROM chip missing");
}
bool CineRomLoader::load(const GameProfile &g,const RomSource &s,bool zip,uint8_t *out,const RomSource *parent) {
    error[0]=0;
    if(g.rom_size>32768 || !g.rom_size) return fail("Invalid driver ROM size");
    memset(out,0,g.rom_size);
    if(!zip) {
        if(s.size!=g.rom_size || !read(s,0,out,g.rom_size)) return fail("BIN size/read mismatch");
        if(g.warrior4j) return cine_crc32(out,g.rom_size)==0xe60d0dbf || fail("Warrior 4J CRC mismatch");
    } else if(!g.chip_count) return fail("This driver requires packed BIN");
    for(unsigned n=0;n<g.chip_count;++n) {
        const auto &c=g.chips[n];
        if(!c.size || c.size>sizeof(chip) || c.offset+2u*(c.size-1)>=g.rom_size) return fail("Invalid driver chip layout");
        if(zip) {
            if(!extract(s,c)) {

                if(strcmp(error,"Required ROM chip missing") || !parent || !extract(*parent,c)) return false;
            }
            for(unsigned i=0;i<c.size;++i) {
                auto &dest=out[c.offset+2*i];
                if(c.nibble<0) dest=chip[i];
                else dest=static_cast<uint8_t>((dest&~(15u<<c.nibble))|((chip[i]&15u)<<c.nibble));
            }
        } else {
            if(c.nibble>=0) return fail("Nibble ROM sets require ZIP");
            for(unsigned i=0;i<c.size;++i) chip[i]=out[c.offset+2*i];
            if(cine_crc32(chip,c.size)!=c.crc) return fail("BIN chip CRC mismatch");
        }
    }
    error[0]=0;
    return true;
}
}
