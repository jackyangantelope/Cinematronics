# Third-party source and notices

| Component | Public source and fixed revision | License / local location | Use |
| --- | --- | --- | --- |
| MAME CCPU and Cinematronics drivers | [mamedev/mame](https://github.com/mamedev/mame) `6ae579aed3107c0b42c1c1c5cb05c02df4456eff` | Selected source headers are BSD-3-Clause; `deps/mame-reference/LICENSE` and `THIRD_PARTY_NOTICES.txt` | CCPU behavior, board details, ROM/input metadata. Only selected reference files are copied. |
| Historical MAME audio protocol | [mamedev/mame](https://github.com/mamedev/mame/tree/mame0170/src/mame/audio) tag `mame0170`, `cinemat.cpp`; local source SHA-256 `3f2544cc5276adaf5c0406a4c733997889ec69886fe684683e645f284832ab45` | BSD-3-Clause per-file header and notices | Generates event decoder only; no samples. |
| pico_shared | [PicoPlus-devel/pico_shared](https://github.com/PicoPlus-devel/pico_shared) revision `df683bf00ff95e1ceda88f132bbf72f8cc7cadb0` | GPLv3 in `deps/pico_shared/LICENSE`, separate `deps/pico_shared/drivers/pico_fatfs/LICENSE` | FatFS and HDMI support. `platform/hdmi` is an adapted derivative with double-buffer interface. |
| miniz | [fhoedemakers/libchdr](https://github.com/fhoedemakers/libchdr) revision `5693f4d4abc43cdd781d26bcbb4ba1ca6c1ef92c`, `deps/miniz-3.1.1/miniz.c/.h` | MIT notice in `deps/miniz/LICENSE` and source headers | Low-level Deflate for user-supplied ZIP ROMs. |

The original MAME repository as a whole carries other licenses. This port copies only the identified, per-file BSD-3-Clause reference files; it does not claim the entire MAME tree is BSD-licensed. ROM metadata and CRCs are not ROM or sample bytes.
