// SPDX-License-Identifier: BSD-3-Clause
#include "cinematronics_sound.h"
#include <string.h>
#include <algorithm>
namespace jack::cinematronics {
void CineSynth::reset(const char *const *names) {
    for(auto &v:voices) v=Voice{};
    names_=names; events=0; random_=0x12345678;
}
void CineSynth::start(unsigned channel,unsigned effect,bool loop) {
    if(channel>=12 || !names_) return;
    unsigned count=0; while(names_[count+1]) ++count;
    if(effect>=count) return;
    const char *name=names_[effect+1];
    Voice &v=voices[channel]; v=Voice{}; v.active=true; v.loop=loop;
    v.duration=44100/3; v.target=5000;
    unsigned frequency=400+effect*71;

    if(strstr(name,"exp") || strstr(name,"die") || strstr(name,"crack") || strstr(name,"nuke")) {
        v.kind=1; v.duration=44100; v.target=12000;
    } else if(strstr(name,"thrust") || strstr(name,"eng") || strstr(name,"offroad") || strstr(name,"chopper")) {
        v.kind=2; v.target=6500; frequency=90+effect*11;
    } else if(strstr(name,"fire") || strstr(name,"laser") || strstr(name,"cannon")) {
        v.kind=3; v.duration=11025; frequency=1800+effect*93;
    } else if(strstr(name,"music") || strstr(name,"drone") || strstr(name,"bg")) {
        frequency=110+effect*23; v.target=3000;
    }
    v.step=static_cast<uint32_t>((uint64_t{1}<<32)*frequency/44100);
    ++events;
}
void CineSynth::stop(unsigned ch) { if(ch<12) { voices[ch].target=0; voices[ch].loop=false; voices[ch].age=voices[ch].duration; } }
bool CineSynth::playing(unsigned ch) const { return ch<12 && voices[ch].active; }
void CineSynth::set_frequency(unsigned ch,double frequency) {
    if(ch>=12) return;

    const double hz=std::clamp(frequency*1050.0/44100.0,20.0,12000.0);
    voices[ch].step=static_cast<uint32_t>(4294967296.0*hz/44100.0);
}
void CineSynth::set_volume(unsigned ch,double volume) { if(ch<12) voices[ch].target=static_cast<int32_t>(5000*std::clamp(volume,0.0,1.0)); }
int16_t CineSynth::sample() {
    random_^=random_<<13; random_^=random_>>17; random_^=random_<<5;
    const int32_t noise=static_cast<int32_t>(random_>>16)-32768;
    int32_t mix=0;
    for(auto &v:voices) {
        if(!v.active) continue;
        if(!v.loop && v.age>=v.duration) v.target=0;
        const int32_t delta=v.target-v.level;
        v.level+=(delta>=0?(delta+63)/64:(delta-63)/64);
        if(!v.level && !v.target) { v.active=false; continue; }
        if(v.kind==3 && v.age<v.duration) v.phase+=static_cast<uint32_t>(static_cast<uint64_t>(v.step)*(v.duration-v.age)/v.duration);
        else v.phase+=v.step;
        const int32_t ramp=static_cast<int32_t>(v.phase>>16);
        int32_t wave=(ramp<32768?ramp:65535-ramp)*2-32768;
        if(v.kind==1 || v.kind==2) { v.filter+=(noise-v.filter)/8; wave=v.kind==1?v.filter:(v.filter+wave)/2; }
        int32_t level=v.level;
        if(!v.loop && v.age<v.duration) level=static_cast<int32_t>(static_cast<int64_t>(level)*(v.duration-v.age)/v.duration);
        mix+=wave*level/32768;
        if(v.age<44100u*60u) ++v.age;
    }
    return static_cast<int16_t>(std::clamp<int32_t>(mix/2,-32768,32767));
}
#include "cinematronics_sound_events.inc"
void CinematronicsSound::reset(Machine machine) {
    machine_=machine; warrior_.reset(); synth_.reset(effect_names(machine));
    m_samples=&synth_; frame_=transitions_=0; random_=0x3456789;
    m_sound_control=0x9f; m_current_shift=m_last_shift=m_last_shift2=0xffff;
    m_current_pitch=0x10000; m_last_frame=0; m_current_volume=m_target_volume=0;
}
void CinematronicsSound::write(uint8_t port,uint8_t value) {
    if(port>7 || port==5 || port==6) return;
    const uint8_t next=static_cast<uint8_t>((m_sound_control&~(1u<<port))|((value&1u)<<port));
    const uint8_t changed=next^m_sound_control;
    if(!changed) return;
    m_sound_control=next; ++transitions_;
    switch(machine_) {
    case Machine::warrior: warrior_.write(port,value); break;
    case Machine::spacewar: spacewar_sound_w(next,changed); break;
    case Machine::barrier: barrier_sound_w(next,changed); break;
    case Machine::speedfrk: speedfrk_sound_w(next,changed); break;
    case Machine::starhawk: starhawk_sound_w(next,changed); break;
    case Machine::sundance: sundance_sound_w(next,changed); break;
    case Machine::tailg: tailg_sound_w(next,changed); break;
    case Machine::armora: armora_sound_w(next,changed); break;
    case Machine::ripoff: ripoff_sound_w(next,changed); break;
    case Machine::starcas: starcas_sound_w(next,changed); break;
    case Machine::solarq: solarq_sound_w(next,changed); break;
    case Machine::boxingb: boxingb_sound_w(next,changed); break;
    case Machine::wotw: wotw_sound_w(next,changed); break;
    default: break;
    }
}
int16_t CinematronicsSound::sample() { return machine_==Machine::warrior ? warrior_.sample() : synth_.sample(); }
}
