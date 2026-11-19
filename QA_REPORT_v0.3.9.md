# v0.3.9 QA Report — CPS-2 MAME Launch Adapter

## Implemented
- External MAME process backend for five CPS-2 game IDs: xmcota, msh, xmvsf, mshvsf, mvsc.
- `CreateProcessW` invocation with explicitly quoted arguments, configured executable, external ROM folder and validated MAME set identifier; no shell invocation.
- Arcade selection GUI launch action and session lifecycle, MAME process exit polling, and best-effort graceful window close.
- Explicit refusal for MVC2/NAOMI, both Street Fighter EX sets, and EX3, which is music-only.
- ROM and BIOS files are **not** copied or packaged. No binary emulator or copyrighted ROM artwork generated.

## Automated tests (Linux C++17 with -Wall -Wextra -Werror)
- achievement_test: PASS
- cheat_ex_settings_test: PASS
- cheat_ex_test: PASS
- core_test: PASS
- emulator_bridge_test: PASS
- external_roms_test: PASS
- gallery_test: PASS
- mame_backend_test: PASS
- Cross-file GUI call/build references: PASS static check

## Limitations / unresolved
- **Windows C++ compilation is NOT verified:** no Visual Studio or Windows cross-compiler here.
- **No real CPS-2 boot test:** MAME is not installed in the development environment.
- Launch is via an external MAME executable, NOT an embedded emulator.
- External MAME is responsible for displaying the original BIOS/boot sequence and running games.
- No collection pause overlay, customized control mappings, arcade coin/test controls, cheats, or live achievement hooks inside the external MAME session yet.
- Emulator shutdown via WM_CLOSE can fail if MAME has no closable top-level window. The code refuses to forcibly terminate a game.
- Source-only checkpoint deliberately omits ROMs, BIOS, large gameplay previews and Earth video; install earlier approved media assets separately before using Windows build script.
- Original MAME set compatibility and CPS-2 security keys are not verified by mere ZIP presence.
