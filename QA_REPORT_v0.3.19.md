# v0.3.19 — Deep QA report

**Change:** `First_Boot_Test.bat` now creates per-game `FirstBootReports/<game>_result.txt`, records ROM audit and emulator exit codes, and asks the Windows tester to confirm visible responsive combat. User confirmation is explicitly *manual evidence*, not automated proof. Unconfirmed playability exits code 8.

**Build/tests:** 14/14 portable C++ tests built using `g++ -std=c++17 -O0 -Wall -Wextra -Werror` and passed. The first test command omitted `external_roms.o` when linking `emulator_bridge_test`; correcting the test harness command resolved the link error, with no changes to production code. C++ core and backend source objects built with strict warnings.

**Static diagnostic checks:** original ROMs are not modified, and logs write only to `FirstBootReports/`. The batch script has not run on Windows. The Windows Visual Studio GUI compilation and live MAME boot are **not verified**. The script confirms ROM audit exit code and emulator process exit code; observed gameplay requires a manual answer. Current emulator backend is still separate-process MAME, not embedded emulation. Live achievements, gameplay cheats, input hook and pause overlay remain pending.

**Packaging:** source-only. No ROM archives, BIOS files, emulator binaries or full gameplay videos in this ZIP. Archived logo assets and gallery database remain present. ZIP CRC and included-path scan passed.
