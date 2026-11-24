@call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
@cl /nologo /utf-8 /std:c++17 /EHsc /Isrc /Fobuild_tests\ /Fe:build_tests\mvc2_cheat_live.exe build_tests\mvc2_cheat_live.cpp src\mame_backend.cpp src\external_roms.cpp src\cheats.cpp src\emulator_bridge.cpp src\core.cpp src\rom_audit.cpp src\autosave.cpp user32.lib
