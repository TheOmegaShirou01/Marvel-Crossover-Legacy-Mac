# CPS-2 launch integration v0.3.9

The five original CPS-2 titles can now launch in an **external, separately installed MAME executable**, if configured and their ROM ZIP files are present in a separate folder. This is an initial REAL process-launch bridge, **not embedded emulation**. The NAOMI and EX games cannot yet launch.

Place these plain-text configuration files next to `MarvelCrossoverLegacy_Features.exe`:

- `mame_executable.txt`: full path to your separately installed `mame.exe`
- `rom_directory.txt`: full path to your untouched arcade ROM ZIP directory

The frontend starts MAME with a validated CPS-2 set ID and `-rompath` using Windows `CreateProcessW` (not via a shell). No ROMs, BIOS, emulator executable or game assets are packaged in this checkpoint.

MAME provides its own original machine boot, arcade controls, sound, save settings, etc. The collection's pause overlay, cheat application and live achievement detection are **not connected to MAME**. When the external process exits, the frontend returns to selection. The Exit Running Game action sends WM_CLOSE to MAME's top-level windows, and reports failure rather than forcefully terminating an unresponsive game.

**Caveats**: Actual booting depends on complete compatible ROM sets and keys. MAME must be acquired separately, and its distribution/license terms must be respected. This source is not Windows-built or hardware-playtested here.
