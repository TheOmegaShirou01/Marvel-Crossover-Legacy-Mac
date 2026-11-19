# v0.3.10 QA report
- cheat_ex_settings_test: PASS
- core_test: PASS PASS: 8 games, 108 achievement definitions, persistence, validation, state slots, media scan

- achievement_test: PASS PASS: 108 achievements, verified events, guards, unlocks, notifications, persistence\n
- gallery_test: PASS Gallery package: 8 games, 192 character records; first clear unlocks once and persists. PASS

- cheat_ex_test: PASS PASS CHEAT EX unlock and reset


- external_roms_test: PASS PASS: external ROM configuration, ZIP header, missing files, separate EX dependencies, EX3 rejected

- emulator_bridge_test: PASS PASS: backend availability, ROM dependencies, session lifecycle, cheat/continue safeguards, EX3 rejection

- mame_backend_test: PASS PASS: MAME CPS-2 allowlist, external ROM validation, spaces in paths, invalid games, configuration

- naomi_backend_test: PASS PASS: NAOMI BIOS/ROM requirements and runtime ROM path resolution

- Windows Visual Studio GUI compilation: NOT RUN
- Actual NAOMI game boot: NOT RUN
- ROMs/BIOS deliberately excluded from source checkpoint.
