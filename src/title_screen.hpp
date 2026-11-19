#pragma once
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <filesystem>
#include <functional>
namespace mcl { class GameLauncher; }
namespace mcl_title {
struct Callbacks {
 // Starts games from the fullscreen Arcade Select screen (owned by the caller).
 mcl::GameLauncher* launcher=nullptr;
 // Open the options / features window.
 std::function<void(HWND owner,bool cheatExUnlocked)> options;
};
// Fullscreen title screen + main menu. Returns when the player chooses EXIT or closes the window.
// Assets (all optional, fallbacks are drawn if missing), relative to appDir:
//   Logos\mcl_logo.png        title logo (transparent PNG, 1600x760 aspect)
//   earth_background.mp4      looping background video
//   Logos\Cards\<game>.png    Arcade Select card art (320x175)
//   Previews\<game>.mp4       15-second gameplay clip shown when a card flips
//   Audio\title_voice.mp3/.wav played when START is pressed on the title screen
int run(HINSTANCE inst,const std::filesystem::path& appDir,const Callbacks& callbacks);
}
#endif
