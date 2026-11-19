# Marvel: Crossover Legacy — v0.3.13 QA

## New milestone
- Added `src/pause_controls.hpp/.cpp`, a deterministic logical-input state machine and `tests/pause_controls_test.cpp`.
- START+SELECT recognizes the pause-menu chord after a 150ms debounce.
- CONTINUE resumes the menu state; EXIT opens ARE YOU SURE; YES requests game-selection exit; NO immediately returns to gameplay state.
- L1+R1+R2+SELECT+START held for three continuous seconds requests a direct return to collection title; early release cancels and does not trigger the pause shortcut.
- Host must map real keys/controllers to logical buttons. No ROMs or BIOS files included.

## Test results
- 12/12 Linux portable C++ tests compiled with `g++ -std=c++17 -O0 -Wall -Wextra -Werror` and passed: achievement, cheat_ex_settings, cheat_ex, core, emulator_bridge, ex_backend, external_roms, gallery, mame_backend, naomi_backend, rom_audit, pause_controls.
- ZIP CRC checked after packaging.

## Important limitations
- The new pause state machine is not yet connected to MAME controls or an in-game UI overlay.
- `MameBackend::pause()` still returns false; no real emulator pause is implemented.
- Pause/exit actions are requests and do not yet stop the emulator or switch screens.
- MAME's actual Windows process launch, emulator boot behavior, and Windows GUI build have not been validated in this environment.
- Exact controller mappings and runtime input capture need integration and real hardware tests.
