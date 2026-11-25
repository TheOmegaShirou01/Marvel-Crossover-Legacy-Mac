#!/bin/bash
# Makes the built "Marvel Crossover Legacy.app" self-contained (run on macOS after cmake --build):
#   - SDL3 libraries copied into Contents/Frameworks
#   - MAME (Homebrew build) with its plugins and shaders in Contents/Resources/MAME
#   - Flycast.app (official v2.7 release, $FLYCAST_APP) in Contents/Resources/Flycast
#   - app icon, emulator path files, ad-hoc signature
# Game content (logos, music, videos, gallery, ROMs) is added later on Windows by
# tools/make_mac_package.py; it never goes to GitHub.
set -euo pipefail
APP="$1"
EXE="$APP/Contents/MacOS/Marvel Crossover Legacy"
RES="$APP/Contents/Resources"
FW="$APP/Contents/Frameworks"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$RES" "$FW"

# icon from Design/Icon (512 px PNG)
ICONSET="$(mktemp -d)/AppIcon.iconset"; mkdir -p "$ICONSET"
SRC="$ROOT/Design/Icon/Marvel_Crossover_Legacy_Icon_512.png"
for s in 16 32 128 256 512; do
  sips -z $s $s "$SRC" --out "$ICONSET/icon_${s}x${s}.png" >/dev/null
  d=$((s*2)); sips -z $d $d "$SRC" --out "$ICONSET/icon_${s}x${s}@2x.png" >/dev/null
done
iconutil -c icns "$ICONSET" -o "$RES/AppIcon.icns"

# SDL3 + SDL3_ttf/image/mixer and their dependencies
dylibbundler -od -b -x "$EXE" -d "$FW" -p "@executable_path/../Frameworks/"

# MAME
MAMEP="$(brew --prefix mame)"
mkdir -p "$RES/MAME"
cp "$MAMEP/bin/mame" "$RES/MAME/mame"
chmod 755 "$RES/MAME/mame"
for d in plugins hash bgfx artwork language keymaps ini samples; do
  [ -e "$MAMEP/share/mame/$d" ] && cp -R "$MAMEP/share/mame/$d" "$RES/MAME/"
done
cp "$MAMEP/share/mame/"*.bdf "$RES/MAME/" 2>/dev/null || true
dylibbundler -od -b -x "$RES/MAME/mame" -d "$RES/MAME/libs" -p "@executable_path/libs/"
# EX games: MAME's imperfect-emulation warning would wait for a key press
printf 'skip_warnings             1\n' > "$RES/MAME/ui.ini"

# Flycast (MvC2)
mkdir -p "$RES/Flycast"
cp -R "${FLYCAST_APP:-/Applications/Flycast.app}" "$RES/Flycast/Flycast.app"
codesign --force --deep --sign - "$RES/Flycast/Flycast.app"   # ad-hoc (the release is not notarized)

# where the front end finds them (relative to its folder)
printf 'MAME/mame\n' > "$RES/mame_executable.txt"
printf 'Flycast/Flycast.app\n' > "$RES/flycast_executable.txt"

# ad-hoc signature (needed for Apple Silicon); not notarized
codesign --force --sign - "$RES/MAME/mame"
find "$RES/MAME/libs" -name '*.dylib' -exec codesign --force --sign - {} \;
find "$FW" -name '*.dylib' -exec codesign --force --sign - {} \;
codesign --force --sign - "$EXE"
echo "bundled: $APP"
