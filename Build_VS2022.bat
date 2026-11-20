@echo off
setlocal
cd /d "%~dp0"
set "VSPATH="
for %%V in ("%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe" "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe") do if exist %%V set "VSPATH=%%~V"
if not defined VSPATH (
 echo ERROR: Visual Studio vswhere.exe was not found.
 exit /b 1
)
set "VSROOT="
for /f "usebackq delims=" %%P in (`"%VSPATH%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSROOT=%%P"
if not defined VSROOT (
 echo ERROR: Visual Studio 2022 C++ tools are not installed.
 exit /b 1
)
call "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b 1
if not exist bin mkdir bin
REM Embed the app icon (Design\Icon) as a Windows resource.
rc /nologo /fo bin\app.res src\app.rc
if errorlevel 1 (echo ERROR: icon resource failed & exit /b 1)
cl /nologo /utf-8 /std:c++17 /EHsc /W4 /DUNICODE /D_UNICODE /DWIN32_LEAN_AND_MEAN /O2 /Fe:bin\MarvelCrossoverLegacy_Features.exe src\main_win.cpp src\core.cpp src\arcade_select.cpp src\cheat_ex_settings.cpp src\cheat_ex_settings_win.cpp src\external_roms.cpp src\emulator_bridge.cpp src\mame_backend.cpp src\rom_audit.cpp src\pause_controls.cpp src\pause_menu_win.cpp src\lightning.cpp src\earth_video.cpp src\title_screen.cpp src\cheats.cpp src\game_launcher.cpp src\music.cpp src\music_player.cpp src\pause_menu_lua.cpp src\app_settings.cpp bin\app.res /link /SUBSYSTEM:WINDOWS comctl32.lib shell32.lib strmiids.lib ole32.lib user32.lib gdi32.lib d2d1.lib dwrite.lib windowscodecs.lib xinput.lib winmm.lib mfplat.lib mfreadwrite.lib mfuuid.lib propsys.lib
if errorlevel 1 (
 echo BUILD FAILED
 exit /b 1
)
REM Optional media are supplied in a separate assets pack; absence must not break C++ build.
if exist "earth_background.mp4" (
 copy /Y "earth_background.mp4" "bin\earth_background.mp4" >nul
 if errorlevel 1 echo WARNING: Could not copy Earth background video.
) else (echo WARNING: Earth background missing; selection window will use fallback.)
if not exist "bin\Gallery" mkdir "bin\Gallery"
copy /Y "Gallery\arcade_gallery.mclgallery" "bin\Gallery\arcade_gallery.mclgallery" >nul
if errorlevel 1 (
 echo ERROR: Gallery package was not installed to bin\Gallery
 exit /b 1
)
if not exist "bin\Previews" mkdir "bin\Previews"
if exist "Previews\*.mp4" (
 copy /Y "Previews\*.mp4" "bin\Previews\" >nul
 if errorlevel 1 echo WARNING: Could not copy some gameplay previews.
) else (echo WARNING: No gameplay previews found; install optional media pack.)
if not exist "bin\Logos" mkdir "bin\Logos"
if not exist "bin\Logos\Cards" mkdir "bin\Logos\Cards"
copy /Y "Logos\Cards\*.bmp" "bin\Logos\Cards\" >nul
copy /Y "Logos\Cards\*.png" "bin\Logos\Cards\" >nul
if errorlevel 1 (echo ERROR: Missing logo card bitmaps & exit /b 1)
REM Title logo (generated from Design\Logo\gen_logo.py, committed as PNG).
copy /Y "Design\Logo\mcl_logo.png" "bin\Logos\mcl_logo.png" >nul
if errorlevel 1 echo WARNING: Title logo missing; title screen will draw text instead.
REM Optional title voice (user-supplied, not in source checkpoints): Audio\title_voice.mp3 or .wav
if not exist "bin\Audio" mkdir "bin\Audio"
if exist "Audio\title_voice.*" (copy /Y "Audio\title_voice.*" "bin\Audio\" >nul) else (echo WARNING: No title voice in Audio\; PRESS START will be silent.)
REM IMPORTANT: ROMs/BIOS are external. Do not copy or package them.
REM Optional: create bin\rom_directory.txt with ONE path to your external ROM folder.
REM This script does not write to ROM archives and does not install them.
REM Marvel vs. Capcom font for the intro, extracted from the user's own mvsc.zip (never committed).
if not exist "bin\Fonts\mvc_font.png" (
 py -3 tools\extract_mvc_font.py --rom-list bin\rom_directory.txt --out bin\Fonts\mvc_font.png
 if errorlevel 1 echo WARNING: MvC font not extracted; the intro will use the built-in font.
)
echo BUILD SUCCESS: bin\MarvelCrossoverLegacy_Features.exe
endlocal
