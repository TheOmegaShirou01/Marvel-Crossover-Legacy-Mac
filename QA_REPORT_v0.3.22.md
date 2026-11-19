# v0.3.22 — First real Windows results (2026-11-19)

Machine: user's Windows 11 PC, VS 2022 Build Tools (MSVC 14.44), MAME 0.227 (`mame64.exe`).

## Build
- `Build_VS2022.bat`: initially FAILED (undeclared `heading`/`font`, 3-arg `GetMessageW` in arcade_select.cpp). Fixed; build now SUCCEEDS.
- The "'vswhere.exe' is not recognized" line comes from inside vcvars64.bat and is harmless.

## Tests (compiled with MSVC, executed)
- C++: 14 / 14 PASS (`gallery_test` needs the project folder passed as its argument).
- Python `tests/test_rom_preflight.py`: 5 / 5 PASS.

## MAME 0.227 -verifyroms
- `xmcotau`: **GOOD**, using rompath `MCL_Handoff\02_ROMs_and_BIOS;Runtime\ROMs_extra`.
  - `Runtime\ROMs_extra\xmcotau\xmcotau.key` was copied from the user's `xmcotau (1).zip`.
  - `Runtime\ROMs_extra\qsound_hle\dl-1425.bin` was copied from the user's `qsound.zip`. MAME 0.227 names this device `qsound_hle`; newer MAME versions use `qsound`.
  - Originals were not modified.
- msh, xmvsf, mshvsf, mvsc: missing per-game `.key` files.
- mvsc2: missing `mie`/`jvs13551` device ROMs plus `mpr-23084.ic32s` and `25lc040.ic13s`; `epr-21576c.ic27` NEEDS REDUMP.
- sfexp/sfex2p: missing small security/program files (cp04, cp12, x2pe_04.2h).

## First boot
- `xmcotau` launched in MAME (window mode). Attract mode rendered correctly (user screenshot).
- MAME exit code 0, average speed 99.80% over 102 s, QSound (HLE) started, 2 audio buffer overflows at most.
- Player-controlled gameplay: NOT yet user-confirmed.
- The frontend's launcher has NOT launched the game yet: `rom_directory.txt` accepts a single path, but this setup needs two.
