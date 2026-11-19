#pragma once
#include "emulator_bridge.hpp"
#include "external_roms.hpp"
#include <filesystem>
#include <string>
#include <vector>
namespace mcl {
struct MameLaunchSpec {
 std::filesystem::path executable;
 RomSearchPath roms;
 std::string game;    // collection id (achievements, stats)
 std::string mameSet; // MAME romset actually launched (see mame_sets.hpp)
 std::vector<std::wstring> arguments;
};
// Builds a safe argument vector without invoking a command shell.
// Only explicitly supported CPS-2 game identifiers are accepted.
bool createCps2LaunchSpec(const std::filesystem::path& emulator,
                          const RomSearchPath& roms,
                          const std::string& game,
                          MameLaunchSpec& output,
                          std::string& reason);
// Validates MVC2 and its NAOMI BIOS before constructing MAME arguments.
bool createNaomiLaunchSpec(const std::filesystem::path& emulator,
 const RomSearchPath& roms,MameLaunchSpec& output,std::string& reason);
// ZN-1 and ZN-2 arcade games: require per-game ROM and BIOS archives.
bool createExLaunchSpec(const std::filesystem::path& emulator,
 const RomSearchPath& roms,const std::string& game,
 MameLaunchSpec& output,std::string& reason);
// Any supported game: dispatches to the CPS-2 / NAOMI / EX builder.
bool createLaunchSpec(const std::filesystem::path& emulator,
 const RomSearchPath& roms,const std::string& game,
 MameLaunchSpec& output,std::string& reason);
// Plain-text config: one absolute emulator executable path per line (first line used).
std::filesystem::path readMameExecutable(const std::filesystem::path& configFile);
#ifdef _WIN32
class MameBackend final : public EmulatorBackend {
 struct Private;
 Private* p_;
public:
 explicit MameBackend(std::filesystem::path emulator);
 ~MameBackend() override;
 MameBackend(const MameBackend&)=delete;
 MameBackend& operator=(const MameBackend&)=delete;
 bool available(const std::string& gameId) const override;
 bool start(const std::string& gameId,const RomSearchPath& roms) override;
 bool stop() override;
 bool pause(bool paused) override;
 bool running()const;
 // Best-effort default MAME F5 UI pause key. This does not confirm paused state.
 bool requestPauseHotkey();
};
#endif
}
