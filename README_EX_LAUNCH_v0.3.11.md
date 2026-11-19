# Marvel: Crossover Legacy v0.3.11 — CHEAT EX emulator launch

The two approved bonus games, `sfexp` (Capcom ZN-1) and `sfex2p` (Capcom ZN-2), now have MAME launch specifications. Each requires its game ZIP plus its own BIOS set (`coh1000c.zip` or `coh3002c.zip`).

The GUI launch button is enabled after CHEAT EX is unlocked and a compatible external `mame.exe` path has been configured in `mame_executable.txt`. Game and BIOS archives can be loaded from `Runtime/ROMs` or the `rom_directory.txt` location; they are **not packaged** in this checkpoint.

Only EX Plus and EX2 Plus launch from CHEAT EX. EX3 is jukebox-only.

Limitations: MAME is started as a separate process; the launcher doesn't currently inject cheats, intercept custom pause controls, extract live achievement events, embed the game output, or verify successful ROM boot. No Windows runtime build has been performed. The file presence checks are not a complete ROM-set verification. No AI assets were used.
