<#
Builds a portable, self-contained copy of Marvel: Crossover Legacy:

  <Out>\Marvel_Crossover_Legacy_v<Version>\
     MarvelCrossoverLegacy_Features.exe  + Gallery, Logos, Fonts, Audio, Previews, Music
     Emulators\MAME 0.289\  Emulators\Flycast 2.7\   (copied as installed, incl. ui.ini,
                                                       hiscore.dat, controller files)
     ROMs\         the user's original ROM/BIOS zips (copied unchanged)
     ROMs\extra\   the supplementary files (keys, qsound, repaired MvC2 set)
     MCL_UserData\ the player's save, settings, controls and backups (optional)
     Source\       source checkpoint zip (git archive of HEAD)
     mame_executable.txt / flycast_executable.txt / rom_directory.txt  -> relative paths

Usage (from the project folder, after Build_VS2022.bat):
  powershell -ExecutionPolicy Bypass -File tools\make_package.ps1 -Version 0.4.0 -Out "..\..\Packages"
#>
param(
  [string]$Version = "0.4.0",
  [string]$Out = "..\..\Packages",
  [switch]$NoUserData,
  [switch]$Zip
)
$ErrorActionPreference = "Stop"
$utf8 = New-Object System.Text.UTF8Encoding $false   # no BOM: the app reads these files byte for byte
function Write-Text($path, $lines) { [IO.File]::WriteAllText($path, (($lines | ForEach-Object { $_ }) -join "`r`n") + "`r`n", $utf8) }
$proj = (Resolve-Path ".").Path
$bin = Join-Path $proj "bin"
function First-Line($f) { (Get-Content $f | Where-Object { $_ -and -not $_.StartsWith("#") } | Select-Object -First 1).Trim() }
$mameExe = First-Line (Join-Path $bin "mame_executable.txt")
$flyExe  = First-Line (Join-Path $bin "flycast_executable.txt")
$romDirs = Get-Content (Join-Path $bin "rom_directory.txt") | Where-Object { $_ -and -not $_.StartsWith("#") } | ForEach-Object { $_.Trim() }
if (-not (Test-Path $mameExe)) { throw "MAME not found: $mameExe" }
New-Item -ItemType Directory -Force $Out | Out-Null
$dest = Join-Path (Resolve-Path $Out).Path ("Marvel_Crossover_Legacy_v" + $Version)
Write-Host "Packaging to $dest"
function Copy-Tree($from, $to, [string[]]$excludeDirs = @(), [string[]]$excludeFiles = @()) {
  $args = @($from, $to, "/E", "/NFL", "/NDL", "/NJH", "/NJS", "/NP", "/R:1", "/W:1")
  if ($excludeDirs.Count) { $args += "/XD"; $args += $excludeDirs }
  if ($excludeFiles.Count) { $args += "/XF"; $args += $excludeFiles }
  & robocopy @args | Out-Null
  if ($LASTEXITCODE -ge 8) { throw "copy failed: $from" }
}
# 1. the app (bin) without logs, build leftovers and the user's data
Copy-Tree $bin $dest @("Logs", "MCL_UserData") @("app.res", "mame_executable.txt", "flycast_executable.txt", "rom_directory.txt", "*.rar", "*.7z")
# 2. music (the app looks for Music next to the exe)
Copy-Tree (Join-Path $proj "Music") (Join-Path $dest "Music")
# 3. emulators
$mameDir = Split-Path $mameExe -Parent
Copy-Tree $mameDir (Join-Path $dest ("Emulators\" + (Split-Path $mameDir -Leaf))) @("snap")
if ($flyExe -and (Test-Path $flyExe)) {
  $flyDir = Split-Path $flyExe -Parent
  Copy-Tree $flyDir (Join-Path $dest ("Emulators\" + (Split-Path $flyDir -Leaf)))
}
# 4. ROMs and BIOS (first folder = originals, the rest = extra), copied unchanged
$romTargets = @()
for ($i = 0; $i -lt $romDirs.Count; $i++) {
  $name = if ($i -eq 0) { "ROMs" } else { "ROMs\extra" + $(if ($i -gt 1) { $i } else { "" }) }
  Copy-Tree $romDirs[$i] (Join-Path $dest $name)
  $romTargets += $name
}
# Every ROM the games use now lives inside the package. Where an extra folder holds a
# repaired copy of an original (e.g. mvsc2.zip, naomi.zip), the package keeps only the
# repaired one so no emulator can pick the broken file. (The user's originals are untouched.)
$romsRoot = Join-Path $dest "ROMs"
foreach ($t in $romTargets | Select-Object -Skip 1) {
  Get-ChildItem (Join-Path $dest $t) -File | ForEach-Object {
    $orig = Join-Path $romsRoot $_.Name
    if (Test-Path $orig) { Remove-Item $orig -Confirm:$false; Write-Host "  using repaired $($_.Name) from $t" }
  }
}
# "xmcotau (1).zip" is only the source of xmcotau.key (already in extra\xmcotau); not needed by any game
Get-ChildItem $romsRoot -File | Where-Object { $_.Name -match '\(\d+\)\.zip$' } | Remove-Item -Confirm:$false
# 5. relative configuration
Write-Text (Join-Path $dest "mame_executable.txt") ("Emulators\" + (Split-Path $mameDir -Leaf) + "\" + (Split-Path $mameExe -Leaf))
if ($flyExe) { Write-Text (Join-Path $dest "flycast_executable.txt") ("Emulators\" + (Split-Path (Split-Path $flyExe -Parent) -Leaf) + "\" + (Split-Path $flyExe -Leaf)) }
Write-Text (Join-Path $dest "rom_directory.txt") (@("# ROM/BIOS folders, relative to this file") + $romTargets)
# 6. the player's progress (save, settings, controls, backups)
if (-not $NoUserData -and (Test-Path (Join-Path $bin "MCL_UserData"))) {
  Copy-Tree (Join-Path $bin "MCL_UserData") (Join-Path $dest "MCL_UserData") @() @("game_*.txt", "game_script.lua")
}
# 7. source checkpoint
New-Item -ItemType Directory -Force (Join-Path $dest "Source") | Out-Null
& git archive --format=zip -o (Join-Path $dest ("Source\MCL_v" + $Version + "_SOURCE.zip")) HEAD
# 8. read-me
@"
Marvel: Crossover Legacy v$Version  (portable)

Run MarvelCrossoverLegacy_Features.exe. Everything it needs is in this folder:
emulators (MAME 0.289, Flycast 2.7), ROMs and BIOS, music, gallery art and sounds.
Paths are relative, so the whole folder can be moved or copied to another PC.

Your progress, settings and controls live in MCL_UserData (auto-saved, with dated
backups in MCL_UserData\Backups). High scores are saved by MAME in
Emulators\MAME 0.289\hiscore.

Source code: Source\MCL_v${Version}_SOURCE.zip
"@ | Out-File -Encoding utf8 (Join-Path $dest "README.txt")
$size = (Get-ChildItem $dest -Recurse -File | Measure-Object Length -Sum).Sum
Write-Host ("Package ready: {0:N1} GB in {1}" -f ($size / 1GB), $dest)
if ($Zip) {
  $zipPath = $dest + ".zip"
  if (Test-Path $zipPath) { Remove-Item $zipPath }
  Compress-Archive -Path $dest -DestinationPath $zipPath -CompressionLevel Fastest
  Write-Host "Zip: $zipPath"
}
