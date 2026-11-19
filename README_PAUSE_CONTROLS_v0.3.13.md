# v0.3.13 — Pause control state machine (source-only)

Implements logical, remappable actions for START+SELECT pause and the five-button L1+R1+R2+SELECT+START three-second quick exit. Includes 150 ms pause chord deferral to prioritize five-button quick exit. An attempted quick exit cancelled before three seconds does not accidentally open pause.

Pause menu: Continue resumes; Exit Game opens ARE YOU SURE?; Yes requests return to the six-game selection; No resumes gameplay immediately, closing both menus. Quick exit requests return to the collection title screen. These are **requests**, not actual emulator stop calls.

**Not integrated with MAME keyboard/controller input, not a working overlay, and not an actual paused emulator.** MAME backend's pause() still returns false. Game-specific input remapping and safe process control remain required for real runtime operation. No ROMs/BIOS or emulator binaries are included in this checkpoint.
