#pragma once
#include <filesystem>
#include <string>
#include <vector>
namespace mcl {
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
// Settings filename: rom_directory.txt, UTF-8 path on the first line.
// Paths relative to configuration directory are supported.
// Never modifies, extracts, copies or redistributes game archives.
std::filesystem::path readRomDirectory(const std::filesystem::path& configFile);
// Prefer explicit configuration; otherwise use application-local Runtime/ROMs.
std::filesystem::path resolveRuntimeRomDirectory(const std::filesystem::path& applicationDirectory);
RomReadiness checkExternalRomFiles(const std::filesystem::path& directory,const std::string& gameId);
}
