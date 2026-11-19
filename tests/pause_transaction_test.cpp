#include "../src/pause_transaction.hpp"
#include <cassert>
int main(){
 using namespace mcl;
 PauseTransaction t;
 assert(!t.canShowOverlay());
 assert(t.begin(false,true)==PauseCommand::Error);
 assert(t.begin(true,false)==PauseCommand::Error);
 assert(t.begin(true,true)==PauseCommand::SendPauseKey);
 assert(t.requestDelivered(true)==PauseCommand::None);
 assert(!t.canShowOverlay());
 assert(t.confirmPaused(false)==PauseCommand::Error);
 assert(t.resume()==PauseCommand::Error);
 assert(t.confirmPaused(true)==PauseCommand::ShowMenu);
 assert(t.canShowOverlay());
 assert(t.resume()==PauseCommand::SendPauseKey);
 assert(t.confirmResumed(false)==PauseCommand::Error);
 assert(t.confirmResumed(true)==PauseCommand::HideMenu);
 assert(t.phase()==PausePhase::Idle);
 t.begin(true,true);assert(t.requestDelivered(false)==PauseCommand::Error);
 assert(t.phase()==PausePhase::Failed);t.reset();
 assert(t.phase()==PausePhase::Idle);
}
