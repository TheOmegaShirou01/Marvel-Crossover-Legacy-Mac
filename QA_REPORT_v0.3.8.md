# v0.3.8 QA

- core_test: PASS — PASS: 8 games, 108 achievement definitions, persistence, validation, state slots, media scan
- achievement_test: PASS — PASS: 108 achievements, verified events, guards, unlocks, notifications, persistence\n
- gallery_test: PASS — Gallery package: 8 games, 192 character records; first clear unlocks once and persists. PASS
- cheat_ex_test: PASS — PASS CHEAT EX unlock and reset
- cheat_ex_settings_test: PASS — PASS: EX settings persist per game, default off, invalid data rejected, EX3 excluded
- external_roms_test: PASS — PASS: external ROM configuration, ZIP header, missing files, separate EX dependencies, EX3 rejected
- emulator_bridge_test: PASS — PASS: backend availability, ROM dependencies, session lifecycle, cheat/continue safeguards, EX3 rejection

Windows compilation and live arcade boot: NOT TESTED.
