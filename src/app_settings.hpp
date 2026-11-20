#pragma once
#include <filesystem>
namespace mcl {
// Player preferences from the OPTIONS screen, saved as MCL_UserData\settings.txt
// (one "key=value" per line; unknown keys and bad values are ignored).
struct AppSettings {
 int musicVolume=8;          // 0..10, 0 = menu music off
 int voiceVolume=10;         // 0..10, "Let's Go Crazy!" and other voice clips
 bool fullscreen=true;       // front-end window
 bool gamesFullscreen=true;  // MAME / Flycast window mode
 bool backgroundVideo=true;  // spinning Earth video behind the menus
 bool nowPlaying=true;       // "now playing" caption when a song starts
 bool load(const std::filesystem::path& file);  // false if missing (defaults kept)
 bool save(const std::filesystem::path& file) const;
};
}
