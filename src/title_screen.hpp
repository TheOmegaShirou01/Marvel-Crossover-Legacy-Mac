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
namespace mcl_title {
struct Callbacks {
 // Open Arcade Select (owned by the title window). startInEx opens straight on the EX cards.
 // cheatsUnlocked: master cheat code entered this session (Marvel cheats apply).
 std::function<void(HWND owner,bool cheatExUnlocked,bool startInEx,bool cheatsUnlocked)> arcade;
 // Open the options / features window.
 std::function<void(HWND owner,bool cheatExUnlocked)> options;
};
// Fullscreen title screen + main menu. Returns when the player chooses EXIT or closes the window.
// Assets (all optional, fallbacks are drawn if missing), relative to appDir:
//   Logos\mcl_logo.png        title logo (transparent PNG, 1600x760 aspect)
//   earth_background.mp4      looping background video
//   Audio\title_voice.mp3/.wav played when START is pressed on the title screen
int run(HINSTANCE inst,const std::filesystem::path& appDir,const Callbacks& callbacks);
}
#endif
