@call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
@cl /nologo /utf-8 /std:c++17 /EHsc /Isrc /Fobuild_tests\ /Fe:build_tests\gen_cheat_lua.exe build_tests\gen_cheat_lua.cpp src\cheats.cpp
