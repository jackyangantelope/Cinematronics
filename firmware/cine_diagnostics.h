// SPDX-License-Identifier: BSD-3-Clause
#pragma once
#include <stdint.h>
#include <stddef.h>
struct CineStackWatermark {
    static constexpr uint32_t pattern=0xc17e5a93;
    volatile uint32_t *base=nullptr;
    size_t words=0;
    uint32_t minimum=0;
    void paint(volatile uint32_t *p,size_t n) {
        base=p; words=n; minimum=static_cast<uint32_t>(n*4);
        for(size_t i=0;i<n;++i) p[i]=pattern;
    }
    void sample() {
        size_t n=0; while(n<words && base[n]==pattern) ++n;
        const auto bytes=static_cast<uint32_t>(n*4);
        if(bytes<minimum) minimum=bytes;
    }
};
struct CineMemoryStats {
    uint32_t heap_tail,heap_tail_min,heap_peak,alloc_fail,stack0_min,stack1_min,guard_bad;
};
void cine_diagnostics_init();
CineMemoryStats cine_diagnostics_sample();
