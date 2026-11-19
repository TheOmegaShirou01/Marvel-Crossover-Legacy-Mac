# Marvel: Crossover Legacy — Seven-feature foundation v0.1

Unofficial Windows C++17 application. This is a real, minimal feature-browser prototype and a portable data back end, NOT a playable arcade frontend or emulator.

## Included
1. **Arcade Theater**: lists imported MP4/AVI/WMV clips per game, opens the selected video through Windows file association. Does not bypass the emulator or fake original attract-mode playback.
2. **Sound Test/Music Gallery**: scans imported WAV/MP3/WMA clips, opens them in the Windows-associated player. No fabricated game audio.
3. **Character Encyclopedia**: browse a starter set of factual descriptions for characters in each game; extend through verified data later.
4. **Achievement Gallery**: all 84 previously agreed base achievements (72 game-specific and 12 collection-wide); displays locked/unlocked from stored progress. Achievement-award API exists; no fake gameplay triggers.
5. **Save States**: safe binary save/load API with 10 slots per game; the frontend view is READ ONLY until an emulator supplies genuine serialized state data and restore hooks. State data never equals or replaces an arcade ROM.
6. **Player Statistics**: wins/losses/credits/launches/clears/playtime per title, persistent across restarts, updated by event API.
7. **Arcade Museum**: browse all eight titles with year, system, and brief descriptions; original screenshots and art can be imported later.

## Build (Windows, Visual Studio 2022)
Double-click `Build_VS2022.bat` from a Windows PC with VS2022 C++ build tools. Run `bin\MarvelCrossoverLegacy_Features.exe`.

## Import media
After running once, place your legally sourced/captured files into:
`bin\MCL_UserData\media\<game id>\theater\` (MP4, AVI, WMV)
`bin\MCL_UserData\media\<game id>\music\` (WAV, MP3, WMA)
`bin\MCL_UserData\media\<game id>\art\` (PNG, JPG, BMP, scanner only at this stage)

IDs: xmcota, msh, xmvsf, mshvsf, mvsc, mvsc2, sfexp, sfex2p.
Original ROMs/BIOS ZIPs are not included in this source package or modified.

## Technical blockers before full integration
- Choose/integrate licensed emulation components for CPS-2, NAOMI, ZN-1 and ZN-2; match exact ROM revisions and BIOS files.
- Use real emulator state serialization for save/load. State snapshots must be compatible with game/ROM/core version.
- Build verified memory/event watchers per game before enabling automatic unlocks and statistics.
- Capture authentic audio/video from supplied arcade ROMs through appropriate emulator tools; this prototype does not extract any.
- Expand encyclopedia with sourced and verified per-character move lists, portraits and biographies.
- Create proper themed 1990s animated UI; this first pass is deliberately a functional Win32 browser, not a polished final design.
- Both EX games need newly authored achievement definitions; the existing 84 are for the original six + the collection.

## Testing
`g++ -std=c++17 -Wall -Wextra -Werror -pedantic src/core.cpp tests/core_test.cpp -o core_test && ./core_test` (Linux/macOS) validates portable back end. Windows GUI must still be compiled on the user's VS2022 machine.


## v0.3.19 first boot QA
See `README_FIRST_BOOT_v0.3.19.md` and `QA_REPORT_v0.3.19.md`. The diagnostic records audit and exit codes plus optional manual confirmation. No game has yet been verified to boot on Windows in this environment.

## v0.3.20 Windows setup preflight
Run `Windows_Preflight.bat` to check required file names and paths before `First_Boot_Test.bat`. This is presence-only; it does not certify a playable game.
