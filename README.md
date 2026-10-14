# Cinematronics

Cinematronics vector-arcade CCPU source port for the public Adafruit Fruit Jam (RP2350, `HW_CONFIG=8`). This is a source-only release candidate. It contains no arcade ROMs, artwork, sound samples, saves, or prebuilt UF2.

## Status

The 0.3.3 baseline has 27 driver configurations: 26 MAME sets and a Warrior 4J variant. This is configuration coverage, **not** 27 hardware-verified games. The user confirmed basic physical testing of Warrior 4J and Star Castle v3: gameplay displayed, sound was normal, flicker was fixed, and short runs were near 37.98 FPS. Star Castle's 1,839-frame log includes 90 over-budget frames and a 29.336 ms maximum frame; these are not long-run stability claims. Host tests passed for 200 failed-load recoveries and 68,400 frames (30 minutes of simulated time). Other real games and extended physical stability remain unverified.

This public Fruit Jam build changes the board-level SD, USB, HDMI and input wiring. It **compiles but has not been tested on Fruit Jam hardware**. GPIO NES/SNES pad support is disabled because those pins were not mapped for this public board; USB keyboard input remains. Do not transfer the original target's measured FPS or sound result to Fruit Jam without a fresh test.

## Build and use

Use Pico SDK 2.3.x with RP2350/Fruit Jam and linker-script override support, Pico-PIO-USB, an ARM embedded toolchain, CMake and Ninja. On Windows:

```powershell
.\scripts\build-release.ps1 `
  -PicoSdkPath D:\path\to\pico-sdk `
  -PicoPioUsbPath D:\path\to\Pico-PIO-USB `
  -ToolchainPath C:\path\to\arm-toolchain `
  -NinjaPath C:\path\to\ninja.exe
```

Optional `-PioasmDir`, `-PicotoolDir`, and `-TinyUsbPath` select host tools if needed. The result is `build/fruit-jam-release/picoCinematronics.uf2`. It is linked at `0x10080000` for a compatible resident loader, **not** a standalone UF2 for direct flashing. Verify the loader layout before use. This repository does not provide a loader or an SD menu configuration.

Put your own legally obtained game set named by its ID (for example `starcas.zip`) in `/roms/Cinematronics/` on the SD card. `rom-manifest.json` contains filenames, sizes and checksums for validation, not ROM bytes. ZIP Stored/Deflate and a correctly interleaved BIN are supported for the documented configurations. Encrypted, split and ZIP64 archives are unsupported. A matching filename or file size alone does not make another game compatible. The menu lists up to 32 files; Up/Down selects, Enter or Z starts, X rescans, and Esc returns from a game. Game-specific controls are derived from MAME metadata and the board adapter; keyboard directions, Z/X/C/V and number keys cover the tested paths.

The CPU/ROM/board path covers 27 configurations, but sound is approximate rather than circuit-accurate. Speed Freak has partial effects; Demon and QB-3 have no audio implementation. Original artwork, overlays and samples are excluded. Do not infer complete playability for every configuration.

## Source and attribution

The CCPU callback core and Cinematronics board/ROM metadata are adapted from [MAME](https://github.com/mamedev/mame) revision `6ae579aed3107c0b42c1c1c5cb05c02df4456eff`. The 12 historical sound-event protocols come from the BSD-3-Clause MAME `mame0170` `src/mame/audio/cinemat.cpp` reference, not from copyrighted audio sample files. Selected reference files and their notices are retained under `deps/mame-reference/`. `scripts/generate-cinematronics-drivers.py` and `scripts/generate-cinematronics-sound.py` regenerate the metadata and protocol tables from those reference files; the current generated outputs were verified reproducible.

Port changes are mapped per file in [PORTING.md](PORTING.md). `core/ccpu_core.cpp` extracts instruction execution behind callbacks rather than importing MAME's device framework; `cinematronics_board.*` implements the board and input map; `cinematronics_rom.*` validates and loads user-supplied content; `cinematronics_sound.*` and `warrior_sound.*` provide approximate synthesis. `firmware/` adds SD menu, frame/audio scheduling and diagnostics. `platform/` adapts video, HDMI and input for the public board. The HDMI files are adapted derivatives of pico_shared, not an unmodified upstream copy.

The combined firmware is distributed under [GPLv3](LICENSE), with BSD-3-Clause and MIT components retaining their own notices. See [THIRD_PARTY.md](THIRD_PARTY.md), [THIRD_PARTY_NOTICES.txt](THIRD_PARTY_NOTICES.txt), and [LICENSES](LICENSES). Pico SDK, Pico-PIO-USB and TinyUSB are build-environment dependencies, not copied here.
