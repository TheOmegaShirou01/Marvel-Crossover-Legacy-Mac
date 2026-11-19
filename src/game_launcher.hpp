#pragma once
#ifdef _WIN32
#include "core.hpp"
#include <filesystem>
#include <memory>
#include <string>
namespace mcl {
class MameBackend;
class EmulatorBridge;
// Starts a collection game in its emulator (MAME, or Flycast for MvC2) without blocking,
// with the player's cheats when their code was entered this session, and explains a
// failed start from the emulator's log. Configuration is re-read on every launch:
//   bin\mame_executable.txt, bin\flycast_executable.txt, bin\rom_directory.txt,
//   bin\MCL_UserData\marvel_cheats.txt (Marvel + EX cheat switches).
class GameLauncher {
public:
 GameLauncher(std::filesystem::path appDir,Collection& model);
 ~GameLauncher();
 GameLauncher(const GameLauncher&)=delete;
 GameLauncher& operator=(const GameLauncher&)=delete;
 struct Result { bool ok=false; bool cheats=false; std::wstring message; };
 Result launch(const std::string& game,bool cheatsUnlocked,bool cheatExUnlocked);
 bool running() const;
 // Call regularly while a game runs. Returns true once, when the game has ended;
 // failureMessage is filled when the emulator could not run it.
 bool pollEnded(std::wstring& failureMessage);
private:
 std::filesystem::path dir_;
 Collection& model_;
 std::unique_ptr<MameBackend> emulator_;
 std::unique_ptr<EmulatorBridge> session_;
};
}
#endif
