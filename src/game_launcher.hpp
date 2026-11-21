#pragma once
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#ifdef _WIN32
#include "core.hpp"
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
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
 // True once after a game ended through the pause menu's EXIT GAME > YES.
 bool takeExitSignal();
 // Close the running game (used by the controller quick-exit combo).
 bool requestClose();
 // True once MAME has finished loading (first frame); Flycast games are ready at once.
 bool gameReady() const;
 void bringToFront();
 // MvC2 (Flycast) pause screen support: the running game, its window, freeze/unfreeze,
 // whether the NAOMI program is advancing (0 = Flycast's own menu is open) and key taps.
 bool flycastGame() const;
 HWND gameWindow() const;
 bool freezeGame(bool frozen);
 int gameAdvancing();
 bool tapGameKey(unsigned vk);
 void startCoverGuard(HWND cover);
 void stopCoverGuard();
 // The master cheat code was entered in the front end (The Secret Code achievement).
 void awardMasterCode();
 const Collection& collection() const {return model_;}
 // True once, right after the first arcade clear opens the Gallery.
 bool takeGalleryUnlock(){return model_.consumeGalleryUnlockNotice();}
 // Achievements unlocked since the last call (ids), for the front end's popup.
 std::vector<std::string> takeAchievementNotices();
 // OPTIONS > GAMES START IN: window instead of fullscreen.
 void setGamesWindowed(bool windowed){gamesWindowed_=windowed;}
private:
 std::filesystem::path dir_;
 Collection& model_;
 std::unique_ptr<MameBackend> emulator_;
 std::unique_ptr<EmulatorBridge> session_;
 bool gamesWindowed_=false;
 bool needsReadySignal_=false;
 std::string game_;
 void ingestDetectedEvents();
};
}
#endif
