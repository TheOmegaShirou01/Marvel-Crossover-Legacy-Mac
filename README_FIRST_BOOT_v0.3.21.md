# v0.3.21: first boot readiness checkpoint

New `rom_preflight.py` performs **read-only** ZIP inspection with optional full member CRC checks. It understands all eight playable games plus the `xmcotau` USA clone, requiring `xmcota.zip` as its parent. Optional `--verify-mame` calls MAME's own ROM audit (never starts gameplay). JSON reports distinguish ZIP validity from MAME audit results and gameplay verification. `First_Boot_Test.bat xmcotau` also accepts the USA clone and checks for its parent. `Windows_Preflight.bat` retains its original eight IDs and is not a full ROM audit.

Example on Windows with Python installed:

`py -3 rom_preflight.py --rom-dir "C:\ArcadeROMs" --game xmcotau --crc --report xmcotau_report.json`

For actual MAME audit, add `--mame "C:\mame\mame.exe" --verify-mame`. After passing that, `First_Boot_Test.bat xmcotau` can attempt launching the original arcade ROM; it still requires user observation and the Windows MAME runtime.

**Observed in supplied archive copies:** `xmcotau.zip` and `xmcota.zip` passed ZIP CRC checks, but neither contains `xmcotau.key`. The user also supplied `xmcotau (1).zip`, which contains that key; since it is not the expected MAME archive name, do not assume MAME will find it automatically. This checkpoint does not copy, rename, merge, or modify that file. Boot compatibility is unverified.

All downloadable source checkpoints exclude ROMs and BIOS. EX3 remains jukebox music only. No AI-generated game assets.
