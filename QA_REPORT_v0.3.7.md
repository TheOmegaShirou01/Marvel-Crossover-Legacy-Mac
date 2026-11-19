# QA v0.3.7

- Portable GNU C++17 build flags: `-Wall -Wextra -Werror`
- PASS: `core_test` (8 titles, 108 achievements, persistence and state slots)
- PASS: `achievement_test` (event rules and persistence)
- PASS: `gallery_test` (first-clear unlock)
- PASS: `cheat_ex_test` (CHEAT EX code)
- PASS: `cheat_ex_settings_test` (per-game saved preferences, EX3 excluded)
- PASS: `external_roms_test` (relative path, absent/invalid ZIP, both EX sets, EX3 rejected)
- PASS: source build-script check: no ROM files are copied into bin and ROMs folder excluded
- NOT TESTED: MSVC/Win32 compilation, DirectShow runtime, actual arcade emulator boot, applying cheats or verifying contents of ROM sets
