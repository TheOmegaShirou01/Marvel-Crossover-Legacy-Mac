#include "../src/emulator_bridge.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
namespace fs=std::filesystem;
class MockBackend:public mcl::EmulatorBackend {
public:
 bool enabled=false,failStart=false,failStop=false,failPause=false;
 bool available(const std::string&)const override{return enabled;}
 bool start(const std::string&,const mcl::RomSearchPath&) override{return !failStart;}
 bool stop() override{return !failStop;}
 bool pause(bool)override{return !failPause;}
};
int main(){
 const fs::path dir=fs::temp_directory_path()/"mcl_038_runtime_test";fs::remove_all(dir);fs::create_directories(dir/"ROMs");
 mcl::Collection core(dir);MockBackend backend;
 mcl::EmulatorBridge session(core,backend,dir/"ROMs");
 assert(!session.prepare("sfex3").ok); // never playable
 assert(!session.prepare("sfexp").ok);
 backend.enabled=true;
 assert(!session.prepare("sfexp").ok);
 for(auto name:{"sfexp.zip","coh1000c.zip"}){
  std::ofstream out(dir/"ROMs"/name,std::ios::binary);out<<"PK\003\004fake";
 }
 assert(session.prepare("sfexp").ok);
 assert(session.state()==mcl::RuntimeState::Prepared);
 assert(!session.onVerifiedGameplayEvent({"sfexp","match_win",true,true}));
 backend.failStart=true;assert(!session.start().ok);backend.failStart=false;
 assert(session.start().ok);
 assert(!session.onVerifiedGameplayEvent({"sfex2p","match_win",true,true}));
 assert(session.onVerifiedGameplayEvent({"sfexp","match_win",true,true}));
 assert(core.progress("sfexp_01").unlocked);
 assert(session.pause().ok);
 assert(!session.onVerifiedGameplayEvent({"sfexp","guard_break",true,true}));
 assert(session.resume().ok);
 session.onCheatApplicationStatus(true);
 assert(session.onVerifiedGameplayEvent({"sfexp","perfect_round",true,true}));
 assert(core.progress("sfexp_05").unlocked); // trophies unlock with cheats on
 session.onContinueUsed();
 assert(session.onVerifiedGameplayEvent({"sfexp","arcade_clear",true,true}));
 assert(!core.progress("sfexp_12").unlocked); // continue guard
 backend.failStop=true;assert(!session.stop().ok);backend.failStop=false;
 assert(session.stop().ok);assert(!session.onVerifiedGameplayEvent({"sfexp","match_win",true,true}));
 fs::remove_all(dir);
 std::cout<<"PASS: backend availability, ROM dependencies, session lifecycle, trophies with cheats, continue safeguard, EX3 rejection\n";
}
