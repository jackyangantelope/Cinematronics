// SPDX-License-Identifier: BSD-3-Clause
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace jack::cinematronics {
struct WarriorCatalog {
    static constexpr unsigned capacity = 32;
    struct Entry { char name[256]; uint32_t size; } entries[capacity]{};
    unsigned count = 0, selected = 0, skipped = 0;
    static char lower(char c) { return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c; }
    static bool is_bin(const char *name) {
        const size_t n = strlen(name);
        return n > 4 && name[0] != '.' && name[n - 4] == '.' &&
            lower(name[n - 3]) == 'b' && lower(name[n - 2]) == 'i' && lower(name[n - 1]) == 'n';
    }
    bool add(const char *name, uint32_t size) {
        if (!is_bin(name)) return false;
        if (count == capacity || strlen(name) >= sizeof(entries[0].name)) { ++skipped; return false; }
        strcpy(entries[count].name, name);
        entries[count++].size = size;
        return true;
    }
    void sort() {
        for (unsigned i = 1; i < count; ++i)
            for (unsigned j = i; j && strcmp(entries[j - 1].name, entries[j].name) > 0; --j) {
                const Entry tmp = entries[j]; entries[j] = entries[j - 1]; entries[j - 1] = tmp;
            }
    }
    void move(int direction) {
        if (count) selected = (selected + count + direction) % count;
    }
};

struct WarriorButtons {
    uint16_t previous = 0;
    bool armed = false;
    void block() { armed = false; previous = 0; }
    uint16_t update(uint16_t held) {
        if (!armed) { if (!held) armed = true; previous = held; return 0; }
        const uint16_t pressed = static_cast<uint16_t>(held & ~previous);
        previous = held;
        return pressed;
    }
};
}
