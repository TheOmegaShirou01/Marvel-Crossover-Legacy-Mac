# v0.3.20 — Windows preflight (source-only)

Run `Windows_Preflight.bat` from an extracted project directory. Configure `mame_executable.txt` and `rom_directory.txt` with one full Windows path per file, or place your ROMs in `Runtime\ROMs` at runtime (not in the source ZIP). The script checks presence of the eight named game ZIPs, three hardware BIOS ZIPs, the MAME executable, and required build/gallery assets. It writes `Windows_Preflight_Report.txt` alongside the script.

This tool does **not** modify ROMs or BIOS; run gameplay; determine if a ZIP has the correct chips; perform a `-verifyroms` audit; or establish that the VS2022 application compiles. After a passing preflight, run `First_Boot_Test.bat xmcota` for the actual ROM audit and observed Windows boot test.

**Compatibility caveat:** Users with only a regional clone (`xmcotau.zip`) rather than `xmcota.zip` may see a missing parent set. Preserve original filenames and supply the correctly matched parent/clone set according to the MAME revision; do not rename ZIPs to conceal missing data. Some BIOS sets can differ by MAME release.
