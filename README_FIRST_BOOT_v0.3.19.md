# First-boot evidence checkpoint v0.3.19

`First_Boot_Test.bat` now writes `FirstBootReports/<game>_result.txt` and keeps two existing MAME logs. It runs the ROM audit, launches the selected game, records exit status, and asks the person using Windows to confirm **playable combat**. Exit code `0` means the operator confirmed a working game; exit `8` means no confirmation. The tool does not auto-certify gameplay based only on a process exit. The result is *manual confirmation*, not a captured emulator telemetry event.

Test one game at a time with `First_Boot_Test.bat xmcota` (then the others). Keep MAME and the game ZIPs in the runtime folder or configure the two `.txt` paths. The file writes diagnostics only to `FirstBootReports` and never alters ROMs/BIOS. This source checkpoint contains neither ROMs, BIOS, nor video libraries.

**Outstanding:** Native Windows compilation and real MAME gameplay tests; code-level hardware-independent tests cannot prove actual game boot. Input remapping, in-emulator cheats, achievement telemetry, and pause overlays also remain incomplete.
