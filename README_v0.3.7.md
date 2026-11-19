# Marvel: Crossover Legacy — source checkpoint v0.3.7

## Completed
- External ROM locator reads a path from `rom_directory.txt` beside the built Windows executable. It accepts absolute paths or paths relative to that configuration file.
- Read-only readiness checker for `sfexp.zip` + `coh1000c.zip` and `sfex2p.zip` + `coh3002c.zip`. Checks file presence and ZIP signature. **It does not inspect all ROM entries or prove successful boot.**
- CHEAT EX screen offers `CHECK EX ROMS` for the selected EX game; results appear in a diagnostic dialog.
- Visual Studio build script no longer requires, packages or copies any ROM/BIOS ZIP. It copies only gallery/media assets.
- CHEAT EX preference toggles remain configuration only; no actual cheats have been applied.
- EX3 remains music-library only; no executable game support.

## Build instructions
Unzip **on top of your v0.3.6 asset/project tree** (or manually place the unchanged `earth_background.mp4`, `Previews/`, `Gallery/` and `Logos/Cards/` from your previous project directory). This small source checkpoint does not duplicate those large video assets.
Run `Build_VS2022.bat` with Visual Studio 2022 C++ build tools. Windows compilation has NOT been tested in this environment.
After build, create `bin/rom_directory.txt` with exactly one path per file, e.g. `D:\\ArcadeROMs` on its first line. Keep the arcade ZIP archives and BIOS there, OUTSIDE your build directory. Never put ROMs into the distributed source checkpoint.

## Known limitations
- GUI and embedded DirectShow preview Windows runtime are unverified.
- Presence/ZIP-header validation does not check contents, clone parent relationships, decryption keys or BIOS compatibility.
- Emulator core, actual game launching, operator settings and memory cheats are not implemented.
- Current CHEAT EX unlock accepts temporary hardcoded keyboard controls in the feature window, not yet the agreed fully remappable title-screen code.

## QA
See `QA_REPORT_v0.3.7.md`. No ROMs, BIOS, or EX3 ISO files are included.
