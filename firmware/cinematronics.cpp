// SPDX-License-Identifier: BSD-3-Clause

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "hardware/clocks.h"
#include "hardware/uart.h"
#include <new>
#include "cine_diagnostics.h"
#include "hardware/vreg.h"
#include "pico/stdlib.h"
#include "ff.h"
#include "tf_card.h"
#include "hstx.h"
#include "font_8x8.h"
extern "C" {
#include "jack_input.h"
#include "jack_tinyusb_input.h"
#include "jack_video.h"
}
#include "jack_input_devices.h"
#include "jack_nespad_input.h"
#include "ccpu_core.h"
#include "cinematronics_sound.h"
#include "cinematronics_board.h"
#include "cinematronics_rom.h"
#include <algorithm>
#include "warrior_catalog.h"

using namespace jack::cinematronics;

namespace {
constexpr uint32_t kCcpuHz = 19923000u / 4u;
constexpr uint32_t kRefreshNumerator = kCcpuHz;
constexpr uint32_t kRefreshDenominator = 16u * 16u * 16u * 16u * 2u;
constexpr char kRomDirectory[] = "/roms/Cinematronics";

struct CineCatalog : WarriorCatalog {
    bool add(const char *name,uint32_t size) {
        const size_t n=strlen(name);
        const bool zip=n>4 && name[n-4]=='.' && lower(name[n-3])=='z' && lower(name[n-2])=='i' && lower(name[n-1])=='p';
        if(!is_bin(name) && !zip) return false;
        if(!find_game(name)) { ++skipped; return false; }
        if(count==capacity || n>=sizeof(entries[0].name)) { ++skipped; return false; }
        strcpy(entries[count].name,name); entries[count++].size=size; return true;
    }
};
struct Runtime {
    uint32_t frames=0,vectors=0,visible_vectors=0,over_budget=0,fetches=0;
    uint32_t queued=0,dropped=0,presented=0,wait_max_us=0;
    uint16_t *framebuffer=nullptr;
};
static Runtime runtime;

struct StoredVector { int16_t x0,y0,x1,y1; uint16_t color; };
static_assert(sizeof(StoredVector)==10, "Unexpected vector padding");
constexpr unsigned kVectorCapacity=4096;
struct VectorStore { StoredVector items[kVectorCapacity]; };

union WorkMemory {
    CineRomLoader loader;
    VectorStore vectors;
    WorkMemory() : vectors{} {}
    ~WorkMemory() {}
};
static WorkMemory work;
// Union members need explicit object lifetimes.
static_assert(sizeof(WorkMemory) < sizeof(CineRomLoader)+2048*sizeof(jack_video_vector),
              "Work overlay must save RAM versus 0.3.2");
struct Timing {
    uint64_t total=0;
    uint32_t maximum=0;
    void add(uint32_t us) { total+=us; maximum=std::max(maximum,us); }
};
static Timing cpu_timing,draw_timing,audio_timing,log_timing;
static uint32_t vector_peak;

static char log_buffer[1152];
static uint32_t starts,exits,load_ok,load_fail;
static CineMemoryStats memory_stats{};
static unsigned log_length,log_position;
static uint32_t log_skipped;
void pump_log() {
    if(log_position==log_length) return;
    const uint32_t start=time_us_32();
    for(unsigned n=0;n<16 && log_position<log_length && uart_is_writable(uart_default);++n)
        uart_get_hw(uart_default)->dr=static_cast<unsigned char>(log_buffer[log_position++]);
    log_timing.add(time_us_32()-start);
}
void cancel_log() {
    if(log_position!=log_length) { log_position=log_length=0; printf("\n"); }
}
static CineControls controls;
static const GameProfile *selected_game;

static FATFS filesystem;
static CineBoard board;
static uint16_t keyboard_pad;
static CinematronicsSound sound;
static CineCatalog catalog;
static bool playing;
static char menu_status[40];
static uint64_t audio_deadline;
static uint32_t audio_samples, audio_nonzero, audio_late;
static uint32_t frame_max_us;

void service_audio() {
    const uint64_t now = time_us_64() * 44100u;

    if (now > audio_deadline + 20000ull * 44100u) {
        audio_deadline = now;
        ++audio_late;
    }
    if(audio_deadline > now) return;
    const uint32_t started=time_us_32();
    const uint32_t level = hstx_di_queue_get_level();
    if (level + 1u >= HSTX_AUDIO_DI_HIGH_WATERMARK) return;
    unsigned room = (HSTX_AUDIO_DI_HIGH_WATERMARK - level - 1u) * 4u;
    if (room > 512u) room = 512u;
    while (room-- && audio_deadline <= now) {
        const int16_t value = playing ? sound.sample() : 0;
        hstx_push_audio_sample(value, value);
        ++audio_samples;
        if (value) ++audio_nonzero;
        audio_deadline += 1000000u;
    }
    audio_timing.add(time_us_32()-started);
}

void restart_audio_clock() {
    audio_deadline = time_us_64() * 44100u;

    for (unsigned i = 0; i < 384u; ++i) hstx_push_audio_sample(0, 0);
}

void acquire_framebuffer() {
    const uint32_t start=time_us_32();
    while(!hstx_framebuffer_available()) { service_audio(); tight_loop_contents(); }
    const uint32_t waited=time_us_32()-start;
    if(waited>runtime.wait_max_us) runtime.wait_max_us=waited;
}

void draw_message(const char *message, unsigned y) {
    auto *pixels = reinterpret_cast<uint16_t *>(hstx_getframebuffer());
    for (unsigned x = 16u; *message && x + 8u <= 320u; x += 8u, ++message) {
        const unsigned c = static_cast<unsigned char>(*message);
        if (c < 32u || c > 126u) continue;
        for (unsigned row = 0; row < 8u; ++row) {
            const uint8_t bits = static_cast<uint8_t>(font_8x8[row * 95u + c - 32u]);
            for (unsigned col = 0; col < 8u; ++col)
                if (bits & (1u << col)) pixels[(y + row) * 320u + x + col] = 0x7fffu;
        }
    }
}

void render_menu() {
    acquire_framebuffer();
    auto *pixels = reinterpret_cast<uint16_t *>(hstx_getframebuffer());
    memset(pixels, 0, 320u * 240u * sizeof(uint16_t));
    draw_message("CINEMATRONICS  0.3.3", 12u);
    const auto *preview=catalog.count?find_game(catalog.entries[catalog.selected].name):nullptr;
    draw_message(preview && (preview->machine==Machine::demon || preview->machine==Machine::qb3)
        ? "Experimental / sound unavailable" : "27 drivers / HLE sound / 38 FPS",28u);
    const unsigned page = catalog.selected / 8u * 8u;
    for (unsigned row = 0; row < 8u && page + row < catalog.count; ++row) {
        const unsigned index = page + row;
        char text[40];
        snprintf(text, sizeof(text), "%c %.33s", index == catalog.selected ? '>' : ' ',
                 catalog.entries[index].name);
        draw_message(text, 52u + row * 16u);
    }
    if (!catalog.count) draw_message("No known .zip/.bin games found", 68u);
    char summary[40];
    snprintf(summary, sizeof(summary), "%u/%u games  skipped:%u",
             catalog.count ? catalog.selected + 1u : 0u, catalog.count, catalog.skipped);
    draw_message(summary, 182u);
    draw_message(menu_status, 196u);
    draw_message("Arrows select  Z/Enter play", 212u);
    draw_message("X rescan  Esc returns from game", 226u);
    (void)hstx_present_frame();
    memory_stats=cine_diagnostics_sample();
    printf("CINE_MENU_MEM heap_tail=%lu heap_peak=%lu stk0_min=%lu stk1_min=%lu guard_bad=%lu starts=%lu exits=%lu load_ok=%lu load_fail=%lu\n",
        static_cast<unsigned long>(memory_stats.heap_tail),static_cast<unsigned long>(memory_stats.heap_peak),
        static_cast<unsigned long>(memory_stats.stack0_min),static_cast<unsigned long>(memory_stats.stack1_min),
        static_cast<unsigned long>(memory_stats.guard_bad),static_cast<unsigned long>(starts),static_cast<unsigned long>(exits),
        static_cast<unsigned long>(load_ok),static_cast<unsigned long>(load_fail));
}

void scan_games() {
    catalog.count = catalog.selected = catalog.skipped = 0;
    const FRESULT mounted = f_mount(&filesystem, "", 1);
    DIR directory;
    FRESULT result = mounted == FR_OK ? f_opendir(&directory, kRomDirectory) : mounted;
    if (result != FR_OK) {
        snprintf(menu_status, sizeof(menu_status), "SD/folder error %d - B to retry", result);
    } else {
        FILINFO info;
        while ((result = f_readdir(&directory, &info)) == FR_OK && info.fname[0]) {
            if (!(info.fattrib & (AM_DIR | AM_HID | AM_SYS)))
                (void)catalog.add(info.fname, static_cast<uint32_t>(info.fsize));
            service_audio();
        }
        f_closedir(&directory);
        catalog.sort();
        if (result != FR_OK) snprintf(menu_status, sizeof(menu_status), "Directory read error %d", result);
        else snprintf(menu_status, sizeof(menu_status), "%s", kRomDirectory);
    }
    printf("CINE_MENU files=%u skipped=%u sd=%d dir=%d\n",
           catalog.count, catalog.skipped, mounted, result);
    restart_audio_clock();
    render_menu();
}

uint8_t program_read(void *c,uint16_t a) {

    if((++runtime.fetches&1023u)==0) { service_audio(); pump_log(); }
    return static_cast<CineBoard *>(c)->read_program(a);
}
uint16_t data_read(void *c,uint16_t a) { return static_cast<CineBoard *>(c)->read_data(a); }
void data_write(void *c,uint16_t a,uint16_t v) { static_cast<CineBoard *>(c)->write_data(a,v); }
uint8_t port_read(void *c,uint8_t p) { return static_cast<CineBoard *>(c)->read_port(p); }
void port_write(void *c,uint8_t p,uint8_t v) {
    service_audio();
    sound.write(p,v);
    static_cast<CineBoard *>(c)->write_port(p,v);
}
uint8_t external_input(void *c) { return static_cast<CineBoard *>(c)->external_input(); }
void emit_vector(void *c,int16_t sx,int16_t sy,int16_t ex,int16_t ey,uint8_t shift) {
    auto *b=static_cast<CineBoard *>(c);
    ++runtime.vectors;
    if((runtime.vectors&15u)==0) service_audio();
    const uint8_t intensity=sx==ex && sy==ey ? static_cast<uint8_t>(std::min(255u,0x1ffu*shift/8u)) : 255u;
    if(!intensity) return;
    b->transform(sx,sy); b->transform(ex,ey);
    const unsigned color=b->color;
    const uint16_t scaled=static_cast<uint16_t>(((((color>>10)&31u)*intensity/255u)<<10) |
        ((((color>>5)&31u)*intensity/255u)<<5) | ((color&31u)*intensity/255u));
    if(runtime.queued<kVectorCapacity) work.vectors.items[runtime.queued++]={sx,sy,ex,ey,scaled};
    else ++runtime.dropped;
}
void render_game() {
    acquire_framebuffer();
    const uint32_t started=time_us_32();
    const uint64_t audio_before=audio_timing.total;
    runtime.framebuffer=reinterpret_cast<uint16_t *>(hstx_getframebuffer());
    memset(runtime.framebuffer,0,320u*240u*sizeof(uint16_t));
    for(unsigned i=0;i<runtime.queued;++i) {
        const auto &stored=work.vectors.items[i];
        const jack_video_vector v={stored.x0,stored.y0,stored.x1,stored.y1,stored.color,255};
        (void)jack_video_draw_vectors(runtime.framebuffer,320,240,320,&v,1,1023);
        if(std::max(v.x0,v.x1)>=0 && std::min(v.x0,v.x1)<=1023 &&
           std::max(v.y0,v.y1)>=0 && std::min(v.y0,v.y1)<=1023) ++runtime.visible_vectors;
        if((i&15u)==0) service_audio();
    }
    if(hstx_present_frame()) ++runtime.presented;
    runtime.framebuffer=nullptr;
    vector_peak=std::max(vector_peak,runtime.queued);
    draw_timing.add(time_us_32()-started-static_cast<uint32_t>(audio_timing.total-audio_before));
}
size_t sd_read(void *context,uint32_t at,void *out,size_t size) {
    auto *file=static_cast<FIL *>(context);
    UINT got=0;
    if(f_lseek(file,at)!=FR_OK || f_read(file,out,static_cast<UINT>(size),&got)!=FR_OK) return 0;
    return got;
}
bool load_rom_impl(const char *name) {
    selected_game=find_game(name);
    if(!selected_game) { snprintf(menu_status,sizeof(menu_status),"Unknown game set name"); return false; }
    char path[sizeof(kRomDirectory)+256u];
    snprintf(path,sizeof(path),"%s/%s",kRomDirectory,name);

    static FIL file,parent;
    FRESULT result=f_open(&file,path,FA_READ);
    if(result!=FR_OK) { snprintf(menu_status,sizeof(menu_status),"ROM open error %d",result); return false; }
    const RomSource source{&file,sd_read,static_cast<uint32_t>(f_size(&file))};
    const bool zip=!WarriorCatalog::is_bin(name);

    if(!zip && selected_game->machine==Machine::warrior && source.size==8192 &&
       sd_read(&file,0,board.rom,8192)==8192 && cine_crc32(board.rom,8192)==0xe60d0dbf)
        selected_game=find_game("warrior4j");
    bool parent_open=false;
    RomSource parent_source{};
    if(zip && selected_game->parent[0]) {
        snprintf(path,sizeof(path),"%s/%s.zip",kRomDirectory,selected_game->parent);
        parent_open=f_open(&parent,path,FA_READ)==FR_OK;
        if(parent_open) parent_source={&parent,sd_read,static_cast<uint32_t>(f_size(&parent))};
    }
    auto &rom_loader=*new (&work.loader) CineRomLoader;
    const bool ok=rom_loader.load(*selected_game,source,zip,board.rom,parent_open?&parent_source:nullptr);
    f_close(&file); if(parent_open) f_close(&parent);
    printf("CINE_ROM set=%s bytes=%u result=%s\n",selected_game->id,selected_game->rom_size,ok?"OK":rom_loader.error);
    if(!ok) snprintf(menu_status,sizeof(menu_status),"%.39s",rom_loader.error);
    rom_loader.~CineRomLoader();
    new (&work.vectors) VectorStore;
    return ok;
}

bool load_rom(const char *name) {
    const bool ok=load_rom_impl(name);
    if(ok) ++load_ok; else ++load_fail;
    memory_stats=cine_diagnostics_sample();
    return ok;
}

void route_key(const jack_input_event &event) {
    if (event.type != JACK_INPUT_KEY_DOWN && event.type != JACK_INPUT_KEY_UP) return;
    unsigned key=event.code;
    if(key>='A' && key<='Z') key+='a'-'A';
    if(key<128) controls.keys[key]=event.type==JACK_INPUT_KEY_DOWN;
    uint16_t bit = 0;
    switch (event.code) {
    case JACK_KEY_LEFT: bit = JACK_PAD_LEFT; break;
    case JACK_KEY_RIGHT: bit = JACK_PAD_RIGHT; break;
    case JACK_KEY_UP: bit = JACK_PAD_UP; break;
    case JACK_KEY_DOWN: bit = JACK_PAD_DOWN; break;
    case JACK_KEY_ENTER: bit = JACK_PAD_START; break;
    case JACK_KEY_ESCAPE: bit = JACK_PAD_SELECT; break;
    case '1': bit = JACK_PAD_L; break;
    case '2': bit = JACK_PAD_R; break;
    case 'z': case 'Z': bit = JACK_PAD_A; break;
    case 'x': case 'X': bit = JACK_PAD_B; break;
    case 'c': case 'C': bit = JACK_PAD_X; break;
    case 'v': case 'V': bit = JACK_PAD_Y; break;
    default: return;
    }
    if (event.type == JACK_INPUT_KEY_DOWN) keyboard_pad |= bit;
    else keyboard_pad &= static_cast<uint16_t>(~bit);
}

uint16_t poll_inputs(jack_input_devices &devices, jack_input_queue &queue,
                     bool usb_ok, bool nes_ok) {
    if (usb_ok) jack_tinyusb_input_poll();
    if (nes_ok) (void)jack_nespad_input_poll();
    jack_input_event event;
    while (jack_input_pop(&queue, &event)) route_key(event);
    return static_cast<uint16_t>(devices.pads[0] | keyboard_pad);
}
}

int main() {
    cine_diagnostics_init();
    stdio_init_all();
    vreg_set_voltage(VREG_VOLTAGE_1_30);
    sleep_ms(10);
    if (!set_sys_clock_khz(252000, true)) return 2;
    stdio_init_all();
    sleep_ms(200);
    printf("CINE_BOOT version=0.3.3-fruit-jam-source app=%08x\n", static_cast<unsigned>(JACK_APP_BASE));
    pico_fatfs_spi_config_t config = {SDCARD_SPI, CLK_SLOW_DEFAULT, CLK_FAST_DEFAULT_PIO,
        SDCARD_PIN_MISO, SDCARD_PIN_CS, SDCARD_PIN_SCK, SDCARD_PIN_MOSI, true};
    if (!pico_fatfs_set_config(&config)) return 3;
    hstx_init(false);
    restart_audio_clock();
    jack_input_queue queue;
    jack_input_devices devices;
    jack_input_init(&queue);
    const bool input_ok = jack_input_devices_init(&devices, &queue);
    const bool usb_ok = input_ok && jack_tinyusb_input_init(&devices);
    const bool nes_ok = input_ok && jack_nespad_input_init(&devices, 252000u);
    printf("CINE_BOOT menu=1 audio=44100-hle usb=%u nes=%u\n", usb_ok, nes_ok);
    CcpuCallbacks callbacks = {&board, program_read, data_read, data_write,
        port_read, port_write, external_input, emit_vector};
    CcpuCore cpu(callbacks);
    board.cpu = &cpu;
    scan_games();
    uint32_t last_frame = hstx_getframecounter();
    uint64_t frame_phase = 0;
    uint32_t report_frames=0;
    int32_t mouse_x=0,mouse_y=0;
    uint32_t report_time = to_ms_since_boot(get_absolute_time());
    WarriorButtons buttons;
    uint32_t menu_ready_at = to_ms_since_boot(get_absolute_time()) + 300u;
    while (true) {
        service_audio();
        pump_log();
        const uint16_t pad = poll_inputs(devices, queue, usb_ok, nes_ok);
        const uint32_t input_time = to_ms_since_boot(get_absolute_time());
        if (static_cast<int32_t>(input_time - menu_ready_at) < 0) buttons.block();
        const uint16_t pressed = buttons.update(pad);
        if (!playing) {
            bool dirty = false;
            if (pressed & JACK_PAD_UP) { catalog.move(-1); dirty = true; }
            if (pressed & JACK_PAD_DOWN) { catalog.move(1); dirty = true; }
            if (pressed & JACK_PAD_B) { scan_games(); buttons.block(); }
            else if ((pressed & (JACK_PAD_A | JACK_PAD_START)) && catalog.count) {
                if (load_rom(catalog.entries[catalog.selected].name)) {
                    board.reset(*selected_game);
                    runtime=Runtime{};
                    cpu_timing=draw_timing=audio_timing=log_timing=Timing{};
                    vector_peak=log_skipped=0;
                    log_position=log_length=0;
                    cpu.reset();
                    sound.reset(selected_game->machine);
                    controls=CineControls{};
                    mouse_x=devices.mouse_x; mouse_y=devices.mouse_y;
                    report_frames=0;
                    audio_samples = audio_nonzero = audio_late = frame_max_us = 0;
                    playing = true;
                    ++starts;
                    frame_phase = 0;
                    last_frame = hstx_getframecounter();
                    report_time = to_ms_since_boot(get_absolute_time());
                    printf("CINE_START set=%s target=38.000 audio=%s ESC=menu ENTER=coin 1/2=start\n", selected_game->id, sound.available()?"44100-HLE":"unimplemented-Z80");
                } else dirty = true;
                restart_audio_clock();
                buttons.block();
            }
            if (dirty && !playing) render_menu();
            tight_loop_contents();
            continue;
        }
        if (pressed & JACK_PAD_SELECT) {
            cancel_log();
            playing = false;
            ++exits;
            sound.reset(selected_game->machine);
            buttons.block();
            snprintf(menu_status, sizeof(menu_status), "%s", kRomDirectory);
            render_menu();
            printf("CINE_MENU returned=1\n");
            continue;
        }
        if (pressed & JACK_PAD_START) board.coin = true;
        const uint32_t frame = hstx_getframecounter();
        if (frame == last_frame) { tight_loop_contents(); continue; }
        const uint32_t elapsed = frame - last_frame;
        last_frame = frame;
        frame_phase += static_cast<uint64_t>(elapsed) * kRefreshNumerator;
        while (frame_phase >= 60u * kRefreshDenominator) {
            frame_phase -= 60u * kRefreshDenominator;
            const uint32_t frame_started = time_us_32();
            runtime.queued=0;
            runtime.visible_vectors = 0;
            controls.pads[0]=buttons.armed?pad:0;
            controls.pads[1]=buttons.armed?devices.pads[1]:0;

            controls.mouse_dx=static_cast<int16_t>(std::clamp<int64_t>(int64_t(devices.mouse_x)-mouse_x,-1024,1024));
            controls.mouse_dy=static_cast<int16_t>(std::clamp<int64_t>(int64_t(devices.mouse_y)-mouse_y,-1024,1024));
            mouse_x=devices.mouse_x; mouse_y=devices.mouse_y;
            board.controls(controls);
            sound.frame();
            cpu.watchdog_trigger();
            const uint32_t cpu_started=time_us_32();
            const uint64_t audio_before=audio_timing.total;
            (void)cpu.run(static_cast<int>(kRefreshDenominator));
            cpu_timing.add(time_us_32()-cpu_started-static_cast<uint32_t>(audio_timing.total-audio_before));
            render_game();
            ++runtime.frames;
            const uint32_t frame_us = time_us_32() - frame_started;
            if(frame_us > 26315u) ++runtime.over_budget;
            if (frame_us > frame_max_us) frame_max_us = frame_us;
            service_audio();
        }
        const uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - report_time >= 1000u) {
            memory_stats=cine_diagnostics_sample();
            const uint32_t count=runtime.frames-report_frames;
            const uint32_t fps100=count*100000u/(now-report_time);
            if(log_position==log_length) {
                const uint32_t format_started=time_us_32();
                const int length=snprintf(log_buffer,sizeof(log_buffer),
                    "CINE_ALIVE fps=%lu.%02lu frames=%lu vectors=%lu visible=%lu"
                    " pc=%04x waiting=%u audio=%lu nonzero=%lu late=%lu latch=%02x edges=%lu"
                    " max_us=%lu over=%lu present=%lu wait_us=%lu vdrop=%lu\r\n"
                    "CINE_PERF cpu_avg=%lu cpu_max=%lu draw_avg=%lu draw_max=%lu"
                    " audio_us=%lu audio_max=%lu log_us=%lu log_skip=%lu vpeak=%lu vcap=%u work=%u\r\n"
                    "CINE_MEM up_s=%lu heap_tail=%lu heap_tail_min=%lu heap_peak=%lu alloc_fail=%lu"
                    " stk0_min=%lu stk1_min=%lu guard_bad=%lu starts=%lu exits=%lu load_ok=%lu load_fail=%lu\r\n",
                    static_cast<unsigned long>(fps100/100),static_cast<unsigned long>(fps100%100),
                    static_cast<unsigned long>(runtime.frames),static_cast<unsigned long>(runtime.vectors),
                    static_cast<unsigned long>(runtime.visible_vectors),cpu.state().pc,cpu.state().waiting,
                    static_cast<unsigned long>(audio_samples),static_cast<unsigned long>(audio_nonzero),
                    static_cast<unsigned long>(audio_late),sound.latch(),static_cast<unsigned long>(sound.transitions()),
                    static_cast<unsigned long>(frame_max_us),static_cast<unsigned long>(runtime.over_budget),
                    static_cast<unsigned long>(runtime.presented),static_cast<unsigned long>(runtime.wait_max_us),
                    static_cast<unsigned long>(runtime.dropped),
                    static_cast<unsigned long>(count?cpu_timing.total/count:0),static_cast<unsigned long>(cpu_timing.maximum),
                    static_cast<unsigned long>(count?draw_timing.total/count:0),static_cast<unsigned long>(draw_timing.maximum),
                    static_cast<unsigned long>(audio_timing.total),static_cast<unsigned long>(audio_timing.maximum),
                    static_cast<unsigned long>(log_timing.total),static_cast<unsigned long>(log_skipped),
                    static_cast<unsigned long>(vector_peak),kVectorCapacity,static_cast<unsigned>(sizeof(work)),
                    static_cast<unsigned long>(time_us_64()/1000000),
                    static_cast<unsigned long>(memory_stats.heap_tail),static_cast<unsigned long>(memory_stats.heap_tail_min),
                    static_cast<unsigned long>(memory_stats.heap_peak),static_cast<unsigned long>(memory_stats.alloc_fail),
                    static_cast<unsigned long>(memory_stats.stack0_min),static_cast<unsigned long>(memory_stats.stack1_min),
                    static_cast<unsigned long>(memory_stats.guard_bad),static_cast<unsigned long>(starts),
                    static_cast<unsigned long>(exits),static_cast<unsigned long>(load_ok),static_cast<unsigned long>(load_fail));
                if(length>0 && static_cast<unsigned>(length)<sizeof(log_buffer)) {
                    log_position=0; log_length=static_cast<unsigned>(length);
                } else ++log_skipped;
                cpu_timing=draw_timing=audio_timing=log_timing=Timing{};

                log_timing.add(time_us_32()-format_started);
            } else {
                ++log_skipped;
                cpu_timing=draw_timing=audio_timing=log_timing=Timing{};
            }
            report_time = now;
            report_frames=runtime.frames;
        }
    }
}
