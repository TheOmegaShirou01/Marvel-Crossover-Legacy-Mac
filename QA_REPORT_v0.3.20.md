# Marvel: Crossover Legacy v0.3.20 — Deep QA report

## Change
Added `Windows_Preflight.bat` to validate configured MAME executable path, ROM directory, 8 game ZIP names, 3 BIOS ZIP names, required Gallery database and build script. Optional gameplay videos do not cause false failures. A report is written to `Windows_Preflight_Report.txt`. Added setup instructions and the warning that file presence is not game boot or a MAME audit.

## Verification
- Seven portable C++ production files compiled individually with `g++ -std=c++17 -O0 -Wall -Wextra -Werror` — PASS.
- Fourteen portable C++ regression tests compiled, linked, executed and passed — PASS (tests cover achievements, CHEAT EX, gallery, ROM paths, emulation interfaces, game allowlists, pause logic and ROM auditing).
- Static inspection of new Windows batch preflight — completed, including correction of delayed variable expansion pitfalls. **The batch script itself has not been executed on Windows.**
- Release ZIP CRC and source-only asset scan — PASS.

## Outstanding verification
- Visual Studio 2022 GUI build and Windows batch execution have not been tested in this Linux environment.
- Real MAME ROM audits, game boots, active input, pause overlay, cheats and achievement telemetry are not verified.
- Preflight checks filenames/presence only, not completeness, CRCs of contents, emulator compatibility or gameplay.

## Scope
No ROMs, BIOS archives, emulator binaries, or user-supplied gameplay videos are bundled. EX3 remains jukebox-only.
