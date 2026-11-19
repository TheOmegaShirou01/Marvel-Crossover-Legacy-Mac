@echo off
setlocal EnableExtensions DisableDelayedExpansion
cd /d "%~dp0"
REM First boot diagnostic; no writes to ROMs, no BIOS/ROM bundling.
set "GAME=%~1"
if not defined GAME set "GAME=xmcota"
set "VALID="
for %%G in (xmcota xmcotau msh xmvsf mshvsf mvsc mvsc2 sfexp sfex2p) do if /I "%GAME%"=="%%G" set "VALID=1"
if not defined VALID (
 echo ERROR: Unsupported game ID "%GAME%".
 echo Usage: First_Boot_Test.bat [xmcota^|xmcotau^|msh^|xmvsf^|mshvsf^|mvsc^|mvsc2^|sfexp^|sfex2p]
 exit /b 2
)
set "MAME_FILE=%~dp0mame_executable.txt"
if not exist "%MAME_FILE%" if exist "%~dp0bin\mame_executable.txt" set "MAME_FILE=%~dp0bin\mame_executable.txt"
if not exist "%MAME_FILE%" (
 echo ERROR: Configure the path to mame.exe in mame_executable.txt, one line only.
 exit /b 3
)
set /p "MAME="<"%MAME_FILE%"
if not defined MAME (echo ERROR: Empty emulator path. & exit /b 3)
if not exist "%MAME%" (echo ERROR: Emulator not found: "%MAME%" & exit /b 3)
set "ROMS="
set "ROM_FILE=%~dp0rom_directory.txt"
if not exist "%ROM_FILE%" if exist "%~dp0bin\rom_directory.txt" set "ROM_FILE=%~dp0bin\rom_directory.txt"
if exist "%ROM_FILE%" set /p "ROMS="<"%ROM_FILE%"
if not defined ROMS if exist "%~dp0Runtime\ROMs\" set "ROMS=%~dp0Runtime\ROMs"
if not defined ROMS if exist "%~dp0bin\Runtime\ROMs\" set "ROMS=%~dp0bin\Runtime\ROMs"
if not defined ROMS (
 echo ERROR: Set rom_directory.txt or create Runtime\ROMs.
 exit /b 4
)
if not exist "%ROMS%\" (echo ERROR: ROM directory not found: "%ROMS%" & exit /b 4)
if not exist "%ROMS%\%GAME%.zip" (echo ERROR: Missing "%GAME%.zip" in ROM directory. & exit /b 5)
if /I "%GAME%"=="xmcotau" if not exist "%ROMS%\xmcota.zip" (echo ERROR: Parent xmcota.zip is missing. & exit /b 5)
if /I "%GAME%"=="mvsc2" if not exist "%ROMS%\naomi.zip" (echo ERROR: NAOMI BIOS archive missing. & exit /b 5)
if /I "%GAME%"=="sfexp" if not exist "%ROMS%\coh1000c.zip" (echo ERROR: ZN-1 BIOS archive missing. & exit /b 5)
if /I "%GAME%"=="sfex2p" if not exist "%ROMS%\coh3002c.zip" (echo ERROR: ZN-2 BIOS archive missing. & exit /b 5)
if not exist "%~dp0FirstBootReports" mkdir "%~dp0FirstBootReports"
set "AUDIT_LOG=%~dp0FirstBootReports\%GAME%_rom_audit.log"
set "BOOT_LOG=%~dp0FirstBootReports\%GAME%_boot.log"
set "STATUS_LOG=%~dp0FirstBootReports\%GAME%_result.txt"
>"%STATUS_LOG%" echo GAME=%GAME%
>>"%STATUS_LOG%" echo ROM_AUDIT=NOT_RUN
>>"%STATUS_LOG%" echo EMULATOR_EXIT=NOT_RUN
>>"%STATUS_LOG%" echo GAMEPLAY=UNVERIFIED
echo [1/2] Auditing %GAME% in MAME...
"%MAME%" -verifyroms "%GAME%" -rompath "%ROMS%" >"%AUDIT_LOG%" 2>&1
set "AUDIT_RESULT=%ERRORLEVEL%"
>>"%STATUS_LOG%" echo ROM_AUDIT_EXIT_CODE=%AUDIT_RESULT%
type "%AUDIT_LOG%"
if not "%AUDIT_RESULT%"=="0" (
 echo FAILED: MAME ROM verification returned code %AUDIT_RESULT%.
 echo See "%AUDIT_LOG%"
 exit /b 6
)
echo.
echo [2/2] Starting %GAME% in a MAME WINDOW. Close MAME normally to finish.
echo NOTE: Exit code 0 still does not prove gameplay works. Observe and report boot screen and controls.
"%MAME%" "%GAME%" -rompath "%ROMS%" -window -verbose >"%BOOT_LOG%" 2>&1
set "BOOT_RESULT=%ERRORLEVEL%"
>>"%STATUS_LOG%" echo EMULATOR_EXIT_CODE=%BOOT_RESULT%
echo Emulator process exited with code %BOOT_RESULT%.
echo Boot log: "%BOOT_LOG%"
echo ROM audit log: "%AUDIT_LOG%"
if not "%BOOT_RESULT%"=="0" exit /b 7
echo.
echo REQUIRED MANUAL CHECK: Did this game reach a playable fight with working controls,
echo correct graphics/audio speed, and a clean exit? [Y/N]
choice /C YN /N /M "Playable verified? [Y/N]: "
if errorlevel 2 (
 >>"%STATUS_LOG%" echo GAMEPLAY=NOT_CONFIRMED
 echo Game not yet verified playable. Please provide the boot log for debugging.
 exit /b 8
)
>>"%STATUS_LOG%" echo GAMEPLAY=USER_CONFIRMED
>>"%STATUS_LOG%" echo NOTE=Manual operator confirmation only; advanced features remain unverified
 echo Manual gameplay verification saved to "%STATUS_LOG%".
exit /b 0
