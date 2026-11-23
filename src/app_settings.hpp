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
 // OPTIONS > ACCESSIBILITY. Nothing below works unless `accessibility` is on.
 bool accessibility=false;
 bool oneButtonSpecials=true; // LP / LK / MP / MK perform a special move
 bool oneButtonSupers=true;   // HP / HK perform a super (when the meter allows it)
 bool cheatMenuKey=true;      // X opens the CHEATS menu (no code needed)
 bool cheatExMenuKey=true;    // C opens the CHEAT EX menu (no code needed)
 bool specialsOn() const {return accessibility&&oneButtonSpecials;}
 bool supersOn() const {return accessibility&&oneButtonSupers;}
 bool load(const std::filesystem::path& file);  // false if missing (defaults kept)
 bool save(const std::filesystem::path& file) const;
};
}
