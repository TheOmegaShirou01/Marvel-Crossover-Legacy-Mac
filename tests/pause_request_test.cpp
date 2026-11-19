#include "../src/pause_request.hpp"
#include <cassert>
#include <iostream>
int main(){
 using namespace mcl;
 PauseRequestGate g{};
 assert(pauseRequestResult(g)==PauseRequestFailure::GameNotRunning);
 g.gameRunning=true;
 assert(pauseRequestResult(g)==PauseRequestFailure::NoWindow);
 g.processWindowFound=true;
 assert(pauseRequestResult(g)==PauseRequestFailure::NotForeground);
 g.processForeground=true;
 assert(pauseRequestResult(g)==PauseRequestFailure::InputBlocked);
 g.keyInjectionSucceeded=true;
 assert(pauseRequestResult(g)==PauseRequestFailure::None);
 std::cout<<"PASS: MAME pause request failure guards and success semantics\n";
}
