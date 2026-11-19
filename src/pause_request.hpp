#pragma once
#include <cstdint>
namespace mcl {
enum class PauseRequestFailure { None, GameNotRunning, NoWindow, NotForeground, InputBlocked };
struct PauseRequestGate {
 bool gameRunning=false;
 bool processWindowFound=false;
 bool processForeground=false;
 bool keyInjectionSucceeded=false;
};
inline PauseRequestFailure pauseRequestResult(const PauseRequestGate& g){
 if(!g.gameRunning)return PauseRequestFailure::GameNotRunning;
 if(!g.processWindowFound)return PauseRequestFailure::NoWindow;
 if(!g.processForeground)return PauseRequestFailure::NotForeground;
 if(!g.keyInjectionSucceeded)return PauseRequestFailure::InputBlocked;
 return PauseRequestFailure::None;
}
// A successfully injected hotkey does NOT confirm that emulation is paused.
}
