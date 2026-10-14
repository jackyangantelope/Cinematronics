// SPDX-License-Identifier: BSD-3-Clause
#include "cine_diagnostics.h"
#include "hstx.h"
#include "pico/multicore.h"
#include "hardware/sync.h"
#include <stdint.h>
extern "C" {
extern char end, __HeapLimit;
extern uint32_t __StackBottom[];
void *__real__sbrk(int increment);
}
namespace {
uint32_t heap_break,heap_peak,failures;
CineStackWatermark stack0,stack1;
uint32_t tail_min=UINT32_MAX;
}

extern "C" void *__wrap__sbrk(int increment) {
    void *p=__real__sbrk(increment);
    if(p==reinterpret_cast<void *>(-1)) {
        __atomic_fetch_add(&failures,1u,__ATOMIC_RELAXED); return p;
    }
    const auto top=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(__real__sbrk(0)));
    __atomic_store_n(&heap_break,top,__ATOMIC_RELAXED);
    uint32_t old=__atomic_load_n(&heap_peak,__ATOMIC_RELAXED);
    while(top>old && !__atomic_compare_exchange_n(&heap_peak,&old,top,false,__ATOMIC_RELAXED,__ATOMIC_RELAXED)) {}
    return p;
}
void cine_diagnostics_init() {

    const uint32_t irq=save_and_disable_interrupts();
    uintptr_t sp; __asm volatile("mrs %0, msp" : "=r"(sp));
    const auto bottom=reinterpret_cast<uintptr_t>(__StackBottom);
    if(sp>bottom+256) stack0.paint(reinterpret_cast<uint32_t *>(bottom),(sp-bottom-256)/4);
    restore_interrupts(irq);

    multicore_reset_core1();
    size_t bytes=0; auto *base=static_cast<uint32_t *>(hstx_default_core1_stack(&bytes));
    stack1.paint(base,bytes/4);
}
CineMemoryStats cine_diagnostics_sample() {
    stack0.sample(); stack1.sample();
    const auto begin=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&end));
    const auto limit=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&__HeapLimit));
    uint32_t top=__atomic_load_n(&heap_break,__ATOMIC_RELAXED);
    uint32_t peak=__atomic_load_n(&heap_peak,__ATOMIC_RELAXED);
    if(!top) top=begin;
    if(peak<begin) peak=begin;
    const uint32_t tail=top<=limit?limit-top:0;
    const uint32_t peak_tail=peak<=limit?limit-peak:0;
    if(peak_tail<tail_min) tail_min=peak_tail;
    uint32_t bad=0;
    if(!stack0.words || stack0.minimum==0) bad|=1;
    if(!stack1.words || stack1.minimum==0) bad|=2;
    return {tail,tail_min,peak-begin,__atomic_load_n(&failures,__ATOMIC_RELAXED),stack0.minimum,stack1.minimum,bad};
}
