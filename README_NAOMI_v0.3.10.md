# v0.3.10 — NAOMI launch source checkpoint
- Adds Marvel vs. Capcom 2 (`mvsc2`) to the supported MAME launch interface.
- Requires both `mvsc2.zip` and `naomi.zip` to be present before creating a launch command.
- Allows an app-local `Runtime/ROMs` directory or `rom_directory.txt` path for runtime use. None of these game files are included in source snapshots.
- Main six-card selection now enables the MVC2 launch button when configured MAME is found.
- All ROM/BIOS checks are file-presence checks only; no real boot verified.
- Actual Windows compilation, MAME process runtime, controls, achievement memory hooks and game-specific cheat activation are still pending.
- Street Fighter EX3 remains music-only; EX Plus/EX2 remain exclusive to the hidden CHEAT EX menu.
