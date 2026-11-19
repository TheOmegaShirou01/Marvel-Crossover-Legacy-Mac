# Private ROM/Bios checkpoint — v0.3.6

17 user-uploaded original ZIP archives are bundled byte-for-byte in `ROMs/`: 14 game/region/key archives and 3 BIOS sets. Every ZIP has been CRC tested and individually SHA-256 hashed. See `rom_manifest_v0.3.6.json`.

Use `python verify_roms.py` in the extracted source folder to verify integrity and detect missing files. `Build_VS2022.bat` checks that every required filename is present, and copies these archives to `bin/ROMs/` on a successful build.

These are original supplied ROM sets, not modified or decrypted replacements. Inclusion does NOT prove boot compatibility. The present prototype does NOT include playable emulator integration. No EX3 game ROM is included: EX3 remains jukebox-only.

These user-supplied files are included for private development; do not distribute the ROMs or BIOS publicly without appropriate rights.
