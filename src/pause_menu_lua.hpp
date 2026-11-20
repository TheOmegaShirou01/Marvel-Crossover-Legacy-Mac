#pragma once
#include <string>
namespace mcl {
// In-game pause menu that runs inside MAME (autoboot Lua), drawn over the game:
//   START + SELECT (P1 Start + Coin 1, held 150 ms)  -> game pauses, menu appears
//   CONTINUE / EXIT GAME  (up/down, Button 1 or Start = choose, Button 2 = resume)
//   EXIT GAME -> "ARE YOU SURE?"  YES / NO (NO is preselected and resumes at once)
//   YES -> white flash, writes "exit" to signalFile, MAME quits back to the frontend
// Inputs use MAME's own mapping, so keyboard (1 + 5) and any mapped controller work.
// Testing hook: if a global MCL_TEST_PRESSED(token, frame) exists it replaces real input.
// readyFile (optional) is written on the first emulated frame, i.e. once ROM loading is
// done, so the frontend can cover MAME's "Loading Machine" box until then.
std::string buildPauseMenuLua(const std::string& signalFileUtf8,const std::string& readyFileUtf8={});
}
