# Port adaptation map

Base: [MAME](https://github.com/mamedev/mame) commit `6ae579aed3107c0b42c1c1c5cb05c02df4456eff`; historical audio protocol reference: MAME tag `mame0170`, `src/mame/audio/cinemat.cpp`. Those reference files are preserved under `deps/mame-reference/`. This port is a reimplementation/extraction around them, not a byte-for-byte MAME subtree.

| File or group | Role and adaptation |
| --- | --- |
| `core/ccpu_core.cpp/.h` | Extract CCPU opcode, register and vector behavior from MAME `src/devices/cpu/ccpu/ccpu.cpp/.h` into plain C++ with callback HAL. MAME registration, debugger, disassembler and save-state framework are omitted. |
| `core/cinematronics_profile.cpp/.h`, `cinematronics_profiles.inc`, `rom-manifest.json` | Driver, ROM, DIP and input metadata generated from MAME `src/mame/cinematronics/cinemat.cpp`. The script checks source SHA-256; only names, offsets and checksums are emitted. |
| `core/cinematronics_board.cpp/.h` | Board memory, analog/multiplexed inputs, color and vector handling adapted from MAME Cinematronics board/video reference. |
| `core/cinematronics_rom.cpp/.h` | Bounded SD ZIP/BIN parsing, per-chip CRC validation, parent-set fallback and low-level Deflate via miniz; no ROM bytes included. |
| `core/cinematronics_sound_events.inc`, `cinematronics_sound.cpp/.h` | BSD MAME sound-port event logic extracted by script; original samples device replaced with approximate synthesis. |
| `core/warrior_sound.cpp/.h`, `warrior_catalog.h` | Warrior-specific sound and menu/catalog integration. |
| `firmware/cinematronics.cpp`, `cine_diagnostics.cpp/.h` | SD selection, scheduler, input, dual-buffer presentation, audio and memory diagnostics. |
| `platform/video`, `platform/hdmi`, `platform/input`, `platform/text` | RGB555 vector rasterization, adapted pico_shared HDMI double buffering, Fruit Jam USB/SD wiring and UI input glue. GPIO controller adapter is a disabled stub for this public board. |
| `CMakeLists.txt`, `cmake/LoaderPartition.cmake`, `scripts/build-release.ps1` | Public Fruit Jam build and loader partition. |

Run `python scripts/generate-cinematronics-drivers.py` and `python scripts/generate-cinematronics-sound.py` from the repository root to regenerate the two `.inc` files and `rom-manifest.json`. The generator outputs were checked byte-for-byte against the 0.3.3 baseline before release. Reference files remain under their upstream notices.
