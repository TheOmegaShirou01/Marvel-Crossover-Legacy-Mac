#pragma once
#include <string>
namespace mcl {
// OPTIONS > ACCESSIBILITY for the MAME games: a Lua script (appended to game_script.lua) that
// turns single buttons into whole moves, for both players:
//   1-button specials: LIGHT PUNCH  = quarter-circle forward + LP
//                      MEDIUM PUNCH = dragon punch (forward, down, down-forward) + MP
//                      LIGHT KICK   = quarter-circle back + LK
//                      MEDIUM KICK  = quarter-circle forward + MK
//   1-button supers:   STRONG PUNCH / STRONG KICK = the game's super motion:
//                      X-Men COTA, MSH: quarter-circle forward + all three punches / kicks
//                      XvSF, MSHvSF, MvC: quarter-circle forward + two punches / two kicks
//                      SF EX Plus, EX2 Plus: quarter-circle forward twice + HP / HK
//                      (the game only lets the super out when the meter allows it)
// "Forward" follows the fighter's facing, read from the game's memory (found by RAM tests:
// facing byte/bit per fighter slot; in tag games the slot whose "on screen" flag is set).
// The script reads the real buttons through MAME's input system and drives the game's inputs
// itself while those features are on. Returns "" for games it doesn't support.
std::string buildOneButtonLua(const std::string& game,bool specials,bool supers);
}
