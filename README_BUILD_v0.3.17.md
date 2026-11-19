# Marvel: Crossover Legacy v0.3.17 — Windows build validation

Run `Build_VS2022.bat` from a Windows machine with Visual Studio 2022 C++ desktop tools. This is **source only**, and ROM/BIOS archives are never included or copied.

The build requires C++ sources, the gallery database, and card bitmap assets. It optionally copies `earth_background.mp4` and `Previews/*.mp4` if present. Missing optional videos show warnings, not a failed build. Install supplied media separately beside the compiled application as `bin/earth_background.mp4` and `bin/Previews/<gameid>.mp4`.

To run games, configure an external compatible MAME executable and the ROM directory. Use `Runtime/ROMs` as an internal runtime location if desired; never put ROM archives in a source checkpoint.

**Not verified:** Native MSVC build, MAME ROM compatibility/booting, in-game custom pause overlay, gameplay achievement hooks. The new build-script behavior is statically validated, not executed under Windows.
