// SPDX-License-Identifier: BSD-3-Clause
#pragma once
#include "cinematronics_profile.h"
#include "warrior_sound.h"
namespace jack::cinematronics {

class CineSynth {
public:
    void reset(const char *const *names);
    void start(unsigned channel,unsigned effect,bool loop=false);
    void stop(unsigned channel);
    bool playing(unsigned channel) const;
    void set_frequency(unsigned channel,double frequency);
    void set_volume(unsigned channel,double volume);
    int16_t sample();
    uint32_t events=0;
private:
    struct Voice { uint32_t phase=0,step=0,age=0,duration=0; int32_t level=0,target=0,filter=0;
        bool loop=false,active=false; uint8_t kind=0; } voices[12];
    const char *const *names_=nullptr;
    uint32_t random_=0x12345678;
};
class CinematronicsSound {
public:
    void reset(Machine machine);
    void write(uint8_t port,uint8_t value);
    void frame() { ++frame_; }
    int16_t sample();
    bool available() const { return machine_!=Machine::demon && machine_!=Machine::qb3; }
    uint32_t transitions() const { return transitions_; }
    uint32_t events() const { return machine_==Machine::warrior ? warrior_.transitions() : synth_.events; }
    uint8_t latch() const { return m_sound_control; }
private:
    using UINT8=uint8_t; using UINT16=uint16_t; using UINT32=uint32_t;
    void spacewar_sound_w(UINT8,UINT8); void barrier_sound_w(UINT8,UINT8);
    void speedfrk_sound_w(UINT8,UINT8); void starhawk_sound_w(UINT8,UINT8);
    void sundance_sound_w(UINT8,UINT8); void tailg_sound_w(UINT8,UINT8);
    void armora_sound_w(UINT8,UINT8); void ripoff_sound_w(UINT8,UINT8);
    void starcas_sound_w(UINT8,UINT8); void solarq_sound_w(UINT8,UINT8);
    void boxingb_sound_w(UINT8,UINT8); void wotw_sound_w(UINT8,UINT8);
    uint32_t random() { random_^=random_<<13; random_^=random_>>17; random_^=random_<<5; return random_; }
    Machine machine_=Machine::warrior;
    WarriorSound warrior_;
    CineSynth synth_;
    CineSynth *m_samples=&synth_;
    uint32_t frame_=0,transitions_=0,random_=0x3456789;
    UINT8 m_sound_control=0x9f;
    UINT16 m_current_shift=0xffff,m_last_shift=0xffff,m_last_shift2=0xffff;
    UINT32 m_current_pitch=0x10000,m_last_frame=0;
    double m_current_volume=0,m_target_volume=0;
};
}
