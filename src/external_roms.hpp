#pragma once
#include <filesystem>
#include <string>
#include <vector>
namespace mcl {
// Ordered list of folders MAME searches for ROM sets (MAME -rompath "a;b;c").
// Lets original user archives stay untouched in one folder while separately
// supplied files (keys, device ROMs) live in another.
struct RomSearchPath {
 std::vector<std::filesystem::path> dirs;
 RomSearchPath()=default;
 RomSearchPath(std::filesystem::path single){if(!single.empty())dirs.push_back(std::move(single));}
 bool empty() const {return dirs.empty();}
 // True if at least one folder exists.
 bool anyDirectoryExists() const;
 // "<set>.zip" file or "<set>" folder in any search directory (what MAME accepts).
 bool hasSet(const std::string& set) const;
 // First existing path of a file named `name` across directories; empty if none.
 std::filesystem::path findFile(const std::string& name) const;
 // Semicolon-joined absolute paths for MAME's -rompath option.
 std::wstring mameRompath() const;
};
struct RomCheck {
 std::string filename;
 bool present=false;
 bool looksLikeZip=false;
};
struct RomReadiness {
 std::filesystem::path directory;
 bool configured=false;
 std::vector<RomCheck> files;
 bool readyForFileLevelCheck() const;
};
// Settings filename: rom_directory.txt, UTF-8, one folder per line (blank lines
// and lines starting with '#' are ignored; at most 16 folders).
// Paths relative to configuration directory are supported.
// Never modifies, extracts, copies or redistributes game archives.
std::filesystem::path readRomDirectory(const std::filesystem::path& configFile); // first folder only
RomSearchPath readRomSearchPath(const std::filesystem::path& configFile);
// Prefer explicit configuration; otherwise use application-local Runtime/ROMs.
std::filesystem::path resolveRuntimeRomDirectory(const std::filesystem::path& applicationDirectory);
RomSearchPath resolveRuntimeRomSearchPath(const std::filesystem::path& applicationDirectory);
RomReadiness checkExternalRomFiles(const RomSearchPath& search,const std::string& gameId);
}
