# QA — Marvel: Crossover Legacy v0.3.12

**Scope:** MAME ROM verification preflight and selected-game Windows UI action.

All 11 portable C++ suites built without warnings (`-Wall -Wextra -Werror`) and passed:
- achievement_test
- cheat_ex_settings_test
- cheat_ex_test
- core_test
- emulator_bridge_test
- ex_backend_test
- external_roms_test
- gallery_test
- mame_backend_test
- naomi_backend_test
- rom_audit_test (new)

The new test covers all eight allowed games, correct audit arguments, missing NAOMI BIOS rejection, disallowed EX3/other names, and exit-code interpretation.

**Not tested:** Windows application compilation, actual MAME CLI invocation, GUI rendering, runtime ROM content audits, real boot success, gameplay event recording, or applying game cheats. No ROMs/BIOS/emulator binaries are included.
