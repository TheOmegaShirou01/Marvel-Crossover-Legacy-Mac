# v0.3.21 — Deep QA results

- Newly implemented read-only `rom_preflight.py` handles nine set IDs (eight games plus `xmcotau` regional clone). The new `First_Boot_Test.bat` supports `xmcotau` with parent presence enforcement.
- Python preflight unit tests: **5 / 5 PASS**.
- Portable C++ test executables: **14 / 14 COMPILED and PASS** (`g++ -std=c++17 -O0 -Wall -Wextra -Werror`, portable sources linked). First parallel run timed out; remaining three tests compiled and passed separately. All 14 executables were run successfully. Temporary binaries removed from package.
- User-supplied `xmcotau.zip` and `xmcota.zip`: ZIP CRC checks **PASS**, but neither contains the required named security key; the supplied `xmcotau (1).zip` has the key. This discrepancy needs resolution *without changing user originals* before assuming a MAME audit will pass.
- Windows GUI MSVC compilation: **NOT TESTED**. Actual MAME `-verifyroms`: **NOT TESTED** (MAME not installed in this environment). Playable combat, controls, audio, pause and achievements: **NOT TESTED**.
- Source-only archive integrity: checked during packaging. No game ROMs, BIOS files or executables included.
