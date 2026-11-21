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
// Flycast (NAOMI) launch: fullscreen, logging to flycast.log in Flycast's folder.
// Uses the LAST search folder that holds <set>.zip, so a repaired copy placed in a
// later folder (e.g. Runtime\ROMs_extra) overrides the original archive. Flycast
// loads the BIOS (naomi.zip) from that same folder.
bool createFlycastLaunchSpec(const std::filesystem::path& flycast,
 const RomSearchPath& roms,const std::string& game,
 MameLaunchSpec& output,std::string& reason);
// Any supported game: dispatches to the CPS-2 / NAOMI / EX builder.
bool createLaunchSpec(const std::filesystem::path& emulator,
 const RomSearchPath& roms,const std::string& game,
 MameLaunchSpec& output,std::string& reason);
// Plain-text config: one absolute emulator executable path per line (first line used).
std::filesystem::path readMameExecutable(const std::filesystem::path& configFile);
// Picks the lines of MAME's console output that explain a failed start
// (missing/wrong ROM files, fatal errors), at most maxLines.
std::vector<std::string> summarizeMameLog(const std::string& log,std::size_t maxLines=10);
// True when an emulator log says the game could not run. Flycast exits with code 0
// even when it refuses to start a game, so the log is the reliable signal.
bool logShowsFailedStart(const std::string& log);
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
 // Ask the emulator window to close (like clicking X) without waiting; the process
 // then ends normally and the frontend notices it through running()/lastExit().
 bool requestClose();
 // Put the emulator's main window in front (after the frontend's loading cover).
 bool bringToFront();
 bool pause(bool paused) override;
 bool running()const;
 // Best-effort default MAME F5 UI pause key. This does not confirm paused state.
 bool requestPauseHotkey();
 // Launches MAME, or Flycast for games mapped to it. Name kept for history.
 // MAME's stdout/stderr for each launch is written here (overwritten per launch).
 void setLogFile(std::filesystem::path file);
 // Flycast executable for games mapped to Emulator::Flycast (MvC2).
 void setFlycastExecutable(std::filesystem::path file);
 // Lua cheat script passed to MAME as -autoboot_script on the next start (empty = none).
 // Ignored for Flycast games.
 void setCheatScript(std::filesystem::path file);
 // Start games in a window instead of fullscreen (OPTIONS > GAMES START IN).
 void setWindowed(bool windowed);
 // Player's controls (OPTIONS > CONTROLS SETUP): load MAME\ctrlr\<name>.cfg with -ctrlr and
 // read controllers through DirectInput so pad numbers match the setup screen. Empty = off.
 void setControllerFile(const std::wstring& name);
 // Why the last start() returned false (missing files, emulator not found...).
 std::string lastStartError() const;
 std::filesystem::path logFile() const;
 // After the process has ended (and before stop()), its exit code and run time.
 bool lastExit(unsigned long& exitCode,double& secondsRan) const;
};
#endif
}
