// SPDX-License-Identifier: BSD-3-Clause
// Board behavior adapted from MAME cinemat.cpp and cinemat_v.cpp by Aaron Giles.
#include "cinematronics_board.h"
#include <string.h>
#include <algorithm>
namespace jack::cinematronics {
namespace {
constexpr uint16_t A=1,B=2,X=4,Y=8,START=32,UP=64,DOWN=128,LEFT=256,RIGHT=512;
uint16_t rgb(unsigned r,unsigned g,unsigned b) { return static_cast<uint16_t>(((r>>3)<<10)|((g>>3)<<5)|(b>>3)); }
}
void CineBoard::reset(const GameProfile &p) {
    profile=&p;
    for(unsigned page=0;page<32;++page)
        program_pages[page]=static_cast<uint16_t>(program_offset(p,static_cast<uint16_t>(page*1024u)));
    memset(ram,0,sizeof(ram));
    inputs=p.input_default; switches=p.switches; color=0x7fff;
    latch=ram_bank=0; coin=false; analog_x=analog_y=0x800;
    dial=gear=0; saved_x=saved_y=0; wheel=0;
    sundance_pads[0]=sundance_pads[1]=0;
}
void CineBoard::controls(const CineControls &c) {
    inputs=profile->input_default; switches=profile->switches;
    for (unsigned i=0;i<profile->binding_count;++i) {
        const auto &b=profile->bindings[i];
        bool pressed=(c.pads[b.player]&b.pad)!=0 || (b.key && c.keys[b.key]);
        if (b.pad==2048 && (c.pads[1]&START)) pressed=true;
        if (!pressed) continue;
        if (b.port) switches &= static_cast<uint8_t>(~b.mask);
        else inputs &= static_cast<uint16_t>(~b.mask);
    }
    if (profile->warrior4j) {
        inputs=0xffff;
        const uint16_t masks[]={RIGHT,LEFT,UP,DOWN,B,X,Y,A};
        for (unsigned player=0;player<2;++player)
            for (unsigned bit=0;bit<8;++bit)
                if (c.pads[player]&masks[bit]) inputs &= static_cast<uint16_t>(~(1u<<(bit+(player?0:8))));
    }

    if (profile->machine==Machine::tailg) {
        analog_x=std::clamp(analog_x+((c.pads[0]&RIGHT)?50:0)-((c.pads[0]&LEFT)?50:0)+c.mouse_dx*8,0x200,0xe00);
        analog_y=std::clamp(analog_y+((c.pads[0]&DOWN)?50:0)-((c.pads[0]&UP)?50:0)+c.mouse_dy*8,0x200,0xe00);
    }
    if (profile->machine==Machine::speedfrk) {
        wheel=std::clamp(((c.pads[0]&RIGHT)?2:0)-((c.pads[0]&LEFT)?2:0)+c.mouse_dx/8,-3,3);
        if (c.pads[0]&Y) gear=0;
        if (c.pads[0]&X) gear=1;
        if (c.pads[0]&B) gear=2;
        if (c.pads[0]&DOWN) gear=3;
    }
    if (profile->machine==Machine::boxingb)
        dial=static_cast<uint8_t>(dial+((c.pads[0]&LEFT)?5:0)-((c.pads[0]&RIGHT)?5:0)-c.mouse_dx);
    if (profile->machine==Machine::sundance) {
        sundance_pads[0]=sundance_pads[1]=0;
        const char *keys[2]={"789456123","qweasdzxc"};
        for (unsigned p=0;p<2;++p) {
            for (unsigned n=0;n<9;++n) if (c.keys[static_cast<unsigned>(keys[p][n])]) sundance_pads[p]|=1u<<n;
            const int x=(c.pads[p]&LEFT)?0:(c.pads[p]&RIGHT)?2:1;
            const int y=(c.pads[p]&UP)?0:(c.pads[p]&DOWN)?2:1;
            if (c.pads[p]&(UP|DOWN|LEFT|RIGHT|B)) sundance_pads[p]|=1u<<(y*3+x);
        }
    }
}
uint16_t CineBoard::read_data(uint16_t a) const { return ram[(a&255u)+ram_bank*256u]; }
void CineBoard::write_data(uint16_t a,uint16_t v) { ram[(a&255u)+ram_bank*256u]=v&4095u; }
uint8_t CineBoard::read_port(uint8_t p) const {
    if (p<16) {
        if (profile->machine==Machine::speedfrk) {
            static const uint8_t steer[]={0xe,6,2,0,3,7,0xf};
            if (p<4) return (steer[wheel+3]>>p)&1;
            if (p<7) return gear!=p-4;
        }
        if (profile->machine==Machine::boxingb && p>=12) return (dial>>(p-12+((latch&128)?0:4)))&1;
        if (profile->machine==Machine::qb3 && p==15) return 1;
        if (profile->machine==Machine::sundance) {
            static const uint16_t masks[]={0x155,0,0,0,0,0,0,0,0x1a1,0x1a1,0x155,0,0x093,0x093,0x048,0x048};
            const unsigned player=(p==8 || p==10 || p==13 || p==15)?1:0;
            if (masks[p]) return (sundance_pads[player]&masks[p])?0:1;
        }
        return (inputs>>p)&1;
    }
    if (p==23) return coin?0:1;
    static const uint8_t shuffle[]={2,5,4,3,0,1,6};
    return p<23 ? (switches>>shuffle[p-16])&1 : 1;
}
void CineBoard::write_port(uint8_t p,uint8_t v) {
    if (p>7) return;
    if (profile->machine==Machine::qb3 && p==0) { ram_bank=cpu->state().p&3; return; }
    const uint8_t mask=1u<<p;
    const bool changed=((latch>>p)&1)!=(v&1);
    latch=static_cast<uint8_t>((latch&~mask)|((v&1)<<p));
    if (p==5 && changed && v) coin=false;
    if (p!=6 || !changed) return;
    const auto &s=cpu->state();
    switch (profile->video) {
    case VectorMode::mono: color=v?0x4210:0x7fff; break;
    case VectorMode::gray16:
        if (v) { const unsigned i=((s.x&15)+1)*255/16; color=rgb(i,i,i); } break;
    case VectorMode::gray64:
        if (v) { const unsigned i=(((~s.x>>2)&63)+1)*255/64; color=rgb(i,i,i); } break;
    case VectorMode::rgb444:
        if (v) color=rgb((~s.x&15)*17,((~s.x>>4)&15)*17,((~s.x>>8)&15)*17);
        break;
    case VectorMode::rgb233:
        if (!v) { saved_x=s.x; saved_y=s.y; }
        else { color=rgb((~s.y&7)*255/7,((~s.y>>3)&7)*255/7,((~s.y>>6)&3)*255/3); cpu->restore_vector_position(saved_x,saved_y); } break;
    }
}
uint8_t CineBoard::external_input() const {
    if (profile->jmi) return cpu->read_jmi();
    const int x=static_cast<int16_t>(cpu->state().x<<4)/16;
    const int analog=profile->machine==Machine::tailg?((latch&128)?analog_x:analog_y):0;
    return analog-x<0x800;
}
void CineBoard::transform(int16_t &x,int16_t &y) const {
    int nx=static_cast<int32_t>(x)*1023/profile->width;
    int ny=static_cast<int32_t>(y)*1023/profile->height;
    if (profile->orientation&4) std::swap(nx,ny);
    if (profile->orientation&1) nx=1023-nx;
    if (profile->orientation&2) ny=1023-ny;
    x=static_cast<int16_t>(std::clamp(nx,-32768,32767));
    y=static_cast<int16_t>(std::clamp(ny,-32768,32767));
}
}
