#pragma once
#include <filesystem>
#include <string>
#include <vector>
namespace mcl {
// First-run / new-PC setup: finds the player's ROMs and emulators on their own PC and
// writes rom_directory.txt, mame_executable.txt and flycast_executable.txt next to the
// exe. Runs at startup only when the current settings don't work, so a configured or
// packaged install (relative paths) is never touched.
//
// A folder counts as a ROM folder when it holds at least one of the collection's ROM
// zips (game sets, their parents, BIOS and device sets such as qsound.zip/naomi.zip).
// The search looks at the game folder and its parents, the MAME folder's roms, and the
// player's Desktop, Downloads and Documents, a few levels deep, skipping system folders.
const std::vector<std::string>& knownRomZips();
// Folders under `roots` (to `depth` levels) that contain known ROM zips, most complete first.
std::vector<std::filesystem::path> findRomFolders(const std::vector<std::filesystem::path>& roots,int depth);
// How many of the eight collection games have their ROM set in these folders.
int gamesFound(const std::vector<std::filesystem::path>& folders);
// First file named `exeName` under `roots` (to `depth` levels); empty if none.
std::filesystem::path findExecutable(const std::vector<std::filesystem::path>& roots,const std::wstring& exeName,int depth);
struct AutoConfigResult { bool romsChanged=false,mameChanged=false,flycastChanged=false; int games=0; };
// Checks the config files in appDir and repairs what doesn't work. `searchRoots` overrides
// the default places (used by tests).
AutoConfigResult autoConfigure(const std::filesystem::path& appDir,const std::vector<std::filesystem::path>& searchRoots={});
}
