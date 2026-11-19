# Deep QA — v0.3.17

## Build-script validation
- Optional Earth video and previews now warn rather than block compilation.
- Required gallery and logo assets still fail the build when missing.
- MSVC/Windows GUI build not verified here.

## Portable C++ regression suite
- achievement_test: **PASS** PASS: 108 achievements, verified events, guards, unlocks, notifications, persistence\n
- cheat_ex_settings_test: **PASS** PASS: EX settings persist per game, default off, invalid data rejected, EX3 excluded
- cheat_ex_test: **PASS** PASS CHEAT EX unlock and reset
- core_test: **PASS** PASS: 8 games, 108 achievement definitions, persistence, validation, state slots, media scan
- emulator_bridge_test: **PASS** PASS: backend availability, ROM dependencies, session lifecycle, cheat/continue safeguards, EX3 rejection
- ex_backend_test: **PASS** PASS: ZN-1/ZN-2 ROM and BIOS, game allowlist, paths, failure cases
- external_roms_test: **PASS** PASS: external ROM configuration, ZIP header, missing files, separate EX dependencies, EX3 rejected
- gallery_test: **PASS** Gallery package: 8 games, 192 character records; first clear unlocks once and persists. PASS
- mame_backend_test: **PASS** PASS: MAME CPS-2 allowlist, external ROM validation, spaces in paths, invalid games, configuration
- naomi_backend_test: **PASS** PASS: NAOMI BIOS/ROM requirements and runtime ROM path resolution
- pause_controls_test: **PASS** PASS: pause, confirm-No resume, confirm-Yes exit, 3s quick-exit priority, cancellation, rearm
- pause_request_test: **PASS** PASS: MAME pause request failure guards and success semantics
- pause_transaction_test: **PASS** 
- rom_audit_test: **PASS** PASS: eight-game audit specs, BIOS gate, unsupported IDs, exit interpretation

## Package verification
- Prohibited ROM/BIOS/executable candidates: none.
- Source archive CRC checked after packing.

- Portable test total: 14; passed: 14.
