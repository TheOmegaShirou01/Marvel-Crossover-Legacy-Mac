#include "emulator_bridge.hpp"
#include <algorithm>
#include <system_error>
namespace mcl {
RuntimeResult EmulatorBridge::prepare(const std::string& id){
 if(state_!=RuntimeState::Idle)return {false,"Stop current session first"};
 if(std::none_of(model_.games().begin(),model_.games().end(),[&](const Game& g){return g.id==id;}))return {false,"Unknown game"};
 std::error_code ec;
 if(romDirectory_.empty()||!std::filesystem::is_directory(romDirectory_,ec))return {false,"External ROM directory not configured"};
 if(!backend_.available(id))return {false,"No compatible emulator backend connected"};
 // Presence is necessary but not sufficient. Backends must validate real sets and BIOS.
 if(id=="sfexp"||id=="sfex2p"){
  if(!checkExternalRomFiles(romDirectory_,id).readyForFileLevelCheck())return {false,"Missing EX game ROM or BIOS"};
 } else {
  const auto f=romDirectory_/(id+".zip");
  if(!std::filesystem::is_regular_file(f,ec))return {false,"Game ROM archive missing"};
  if(id=="mvsc2"&&!std::filesystem::is_regular_file(romDirectory_/"naomi.zip",ec))return {false,"NAOMI BIOS missing"};
 }
 game_=id;cheats_=false;continued_=false;state_=RuntimeState::Prepared;
 return {true,"Prepared for backend launch; content not yet validated"};
}
RuntimeResult EmulatorBridge::start(){
 if(state_!=RuntimeState::Prepared)return {false,"Game not prepared"};
 if(!backend_.start(game_,romDirectory_))return {false,"Backend declined launch"};
 state_=RuntimeState::Running;return {true,"Backend running"};
}
RuntimeResult EmulatorBridge::pause(){
 if(state_!=RuntimeState::Running)return {false,"Game not running"};
 if(!backend_.pause(true))return {false,"Backend pause failed"};
 state_=RuntimeState::Paused;return {true,"Paused"};
}
RuntimeResult EmulatorBridge::resume(){
 if(state_!=RuntimeState::Paused)return {false,"Game not paused"};
 if(!backend_.pause(false))return {false,"Backend resume failed"};
 state_=RuntimeState::Running;return {true,"Running"};
}
RuntimeResult EmulatorBridge::stop(){
 if(state_==RuntimeState::Idle)return {false,"No session"};
 if(state_==RuntimeState::Prepared){state_=RuntimeState::Idle;game_.clear();return {true,"Preparation cancelled"};}
 if(!backend_.stop())return {false,"Backend stop failed"};
 state_=RuntimeState::Idle;game_.clear();cheats_=false;continued_=false;return {true,"Stopped"};
}
bool EmulatorBridge::onVerifiedGameplayEvent(VerifiedEvent e){
 if(state_!=RuntimeState::Running||e.game!=game_)return false;
 // Runtime flags cannot be cleared by an incoming achievement event.
 e.cheatsEnabled=e.cheatsEnabled||cheats_;
 e.continued=e.continued||continued_;
 return model_.processVerifiedEvent(e);
}
bool EmulatorBridge::onCheatApplicationStatus(bool active){
 if(state_!=RuntimeState::Running&&state_!=RuntimeState::Paused)return false;
 cheats_=cheats_||active; // once cheats were used, mastery safeguards stay active for this run
 return true;
}
void EmulatorBridge::onContinueUsed(){if(state_==RuntimeState::Running||state_==RuntimeState::Paused)continued_=true;}
}
