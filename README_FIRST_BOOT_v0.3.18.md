# v0.3.18 — First Windows boot diagnostics

This checkpoint provides a repeatable, *manual* first-game boot test without treating process launch as proof of playable gameplay.

1. Build with `Build_VS2022.bat` on Windows, or use a known-compatible MAME build separately.
2. Create `mame_executable.txt` beside this script with one absolute path to `mame.exe`. Alternatively put it in `bin/`.
3. Create `rom_directory.txt` with one absolute path to your ROM folder, or use `Runtime/ROMs/` (or `bin/Runtime/ROMs/`). **ROMs and BIOS archives are never copied to source ZIPs.**
4. Run `First_Boot_Test.bat xmcota` from Command Prompt, then test remaining games by ID: `msh`, `xmvsf`, `mshvsf`, `mvsc`, `mvsc2`, `sfexp`, `sfex2p`.
5. Preserve the text logs in `FirstBootReports`, and report whether the game actually reached gameplay, displayed properly, played at correct speed, responded to inputs, and exited cleanly.

The script checks the game ZIP and appropriate BIOS ZIP file presence, executes MAME `-verifyroms`, then launches MAME with windowed display and verbose output. It fails before launch on a nonzero ROM audit result. An exit code 0 is **not proof of a playable game**. Runtime results require Windows/MAME user testing. If you use a MAME build that lacks a driver or cannot play the game, audit/boot may fail even with intact archives.

This checkpoint includes no ROMs, BIOS files or emulator executables. It does not yet implement in-game pause, live achievements, or cheat activation.
