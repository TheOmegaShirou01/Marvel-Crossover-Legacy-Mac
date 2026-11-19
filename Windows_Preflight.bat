@echo off
setlocal EnableExtensions DisableDelayedExpansion
cd /d "%~dp0"
REM Read-only preflight. This does not run MAME or touch ROM/BIOS files.
set "FAIL=0"
set "REPORT=%~dp0Windows_Preflight_Report.txt"
>"%REPORT%" echo MARVEL: CROSSOVER LEGACY v0.3.20 - WINDOWS PREFLIGHT
>>"%REPORT%" echo NOTE: File presence is not a successful MAME audit or boot verification.
echo.
echo ==== MARVEL: CROSSOVER LEGACY - WINDOWS PREFLIGHT ====
echo.
set "MAME_FILE=%~dp0mame_executable.txt"
if not exist "%MAME_FILE%" if exist "%~dp0bin\mame_executable.txt" set "MAME_FILE=%~dp0bin\mame_executable.txt"
if not exist "%MAME_FILE%" (
 echo [MISSING] mame_executable.txt
 >>"%REPORT%" echo MAME=NOT_CONFIGURED
 set "FAIL=1"
) else (
 call :check_mame
)
set "ROM_FILE=%~dp0rom_directory.txt"
if not exist "%ROM_FILE%" if exist "%~dp0bin\rom_directory.txt" set "ROM_FILE=%~dp0bin\rom_directory.txt"
set "ROMS="
if exist "%ROM_FILE%" set /p "ROMS="<"%ROM_FILE%"
if not defined ROMS if exist "%~dp0Runtime\ROMs\" set "ROMS=%~dp0Runtime\ROMs"
if not defined ROMS if exist "%~dp0bin\Runtime\ROMs\" set "ROMS=%~dp0bin\Runtime\ROMs"
if not defined ROMS (
 echo [MISSING] ROM directory. Configure rom_directory.txt or Runtime\ROMs.
 >>"%REPORT%" echo ROM_DIRECTORY=NOT_CONFIGURED
 set "FAIL=1"
 goto :after_roms
)
if not exist "%ROMS%\" (
 echo [MISSING] Configured ROM directory does not exist
 >>"%REPORT%" echo ROM_DIRECTORY=MISSING
 set "FAIL=1"
 goto :after_roms
)
echo [FOUND] ROM directory
echo.
echo Checking 8 game ZIPs and 3 BIOS ZIPs (presence only)...
>>"%REPORT%" echo ROM_DIRECTORY=FOUND
for %%G in (xmcota msh xmvsf mshvsf mvsc mvsc2 sfexp sfex2p) do call :check_zip %%G.zip
for %%G in (naomi coh1000c coh3002c) do call :check_zip %%G.zip
:after_roms
echo.
if exist "%~dp0Build_VS2022.bat" (echo [FOUND] Visual Studio build script) else (echo [MISSING] Build script & set "FAIL=1")
if exist "%~dp0Gallery\arcade_gallery.mclgallery" (echo [FOUND] Gallery database) else (echo [MISSING] Gallery database & set "FAIL=1")
if exist "%~dp0earth_background.mp4" (echo [OPTIONAL] Earth video present) else (echo [OPTIONAL] Earth video not included in source-only checkpoint)
if exist "%~dp0Previews\*.mp4" (echo [OPTIONAL] Gameplay preview clips present) else (echo [OPTIONAL] Gameplay previews not included in source-only checkpoint)
echo.
if "%FAIL%"=="0" (
 echo PREFLIGHT PASSED: Required paths and file NAMES are present.
 echo This does NOT prove ROM validity, game boot, or Windows build success.
 >>"%REPORT%" echo PREFLIGHT=PASS_PRESENCE_ONLY
) else (
 echo PREFLIGHT INCOMPLETE: See missing items above.
 >>"%REPORT%" echo PREFLIGHT=INCOMPLETE
)
echo Report: "%REPORT%"
exit /b %FAIL%
:check_mame
set "MAME="
set /p "MAME="<"%MAME_FILE%"
if not defined MAME (
 echo [MISSING] Empty MAME executable path
 >>"%REPORT%" echo MAME=EMPTY_PATH
 set "FAIL=1"
 exit /b 0
)
if not exist "%MAME%" (
 echo [MISSING] MAME executable at configured path
 >>"%REPORT%" echo MAME=MISSING_EXECUTABLE
 set "FAIL=1"
) else (
 echo [FOUND] MAME executable
 >>"%REPORT%" echo MAME=FOUND
)
exit /b 0
:check_zip
if exist "%ROMS%\%~1" (
 echo [FOUND] %~1
 >>"%REPORT%" echo %~1=FOUND
) else (
 echo [MISSING] %~1
 >>"%REPORT%" echo %~1=MISSING
 set "FAIL=1"
)
exit /b 0
