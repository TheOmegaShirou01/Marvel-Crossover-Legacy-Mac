# v0.3.11 QA — CHEAT EX launch adapter

## Completed
- MAME ZN-1 (`sfexp`) requires `sfexp.zip` and `coh1000c.zip`.
- MAME ZN-2 (`sfex2p`) requires `sfex2p.zip` and `coh3002c.zip`.
- Approved CHEAT EX screen now enables its launch control for these games only after the unlock code.
- Process start/exit flows reuse existing Windows MAME backend and session bridge.
- EX3 remains excluded from the playable runtime.
- Source checkpoint excludes all ROMs, BIOS ZIPs, emulator binaries and full-size movie assets.

## Verification
- 10 of 10 portable C++ unit/regression tests passed under GCC C++17 `-Wall -Wextra -Werror -pedantic`, including new EX backend launch-spec tests.
- Test runner initially omitted `external_roms.cpp` for the NAOMI test; corrected test command and reran all 10 successfully. No project-code change was required.
- Real Windows Visual Studio 2022 build not available in this Linux environment; Windows GUI code has not been compiled or tested here.
- Real MAME startup, ROM audit, GUI video rendering, pause/cheat integration, and live achievement detection not validated.

## Important limitations
- The ZIP filename presence checks do not establish full ROM/BIOS correctness or game bootability.
- A separately supplied and configured MAME executable is still required. No emulator is bundled.
- The EX cheat settings are UI/configuration only; applying them in emulator memory remains future work.
