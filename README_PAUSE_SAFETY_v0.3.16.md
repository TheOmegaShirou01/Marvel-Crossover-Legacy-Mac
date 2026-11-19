# v0.3.16 Pause safety transaction
Adds `PauseTransaction` to prevent the custom overlay from opening merely because a pause key was injected. Explicit independently verified paused/resumed signals are required. The existing MAME integration does NOT provide those signals yet, so this is a tested safety foundation, not a working in-game overlay.
No ROMs, BIOS, or emulator binaries are included.
