# v0.3.18 Deep QA — First Boot Diagnostic

## Changes
- Added `First_Boot_Test.bat`, an explicit eight-game allowlist and BIOS-presence verification.
- Runs MAME `-verifyroms`, gates game startup on its success, then launches MAME with `-window -verbose`.
- Creates per-game audit and boot logs under `FirstBootReports`.
- Keeps the source checkpoint free of ROMs, BIOS data, and emulator executables.

## Verification performed
- MAME launch configuration portable C++ test: **PASS**, compiled with `-std=c++17 -Wall -Wextra -Werror`.
- Eight-game ROM-audit portable C++ test: **PASS**, same strict flags.
- Batch static validation: **PASS** (eight IDs, three required BIOS archive names, verify/start/logging branches).
- Forbidden ROM/BIOS/executable archive scan: **PASS**.
- ZIP CRC verification: reported at packaging.

## Not verified
- Remaining portable regression tests: full rerun exceeded the execution limit. Prior milestone results are historical and do not count as a fresh full run.
- Windows batch execution, Visual Studio 2022 GUI compilation, real MAME audit, full game boot, controls, audio, cheats, pause, achievements: **NOT VERIFIED**.
- MAME exit code 0 does **not** certify playable gameplay; on-device user testing is required.
