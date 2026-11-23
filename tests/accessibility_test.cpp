#include "../src/accessibility_lua.hpp"
#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>
int main(int argc,char** argv){
 // accessibility_test --lua <game> : print the generated script (used by the MAME test harness)
 if(argc==3&&std::string(argv[1])=="--lua"){std::fputs(mcl::buildOneButtonLua(argv[2],true,true).c_str(),stdout);return 0;}
 for(const char* g:{"xmcota","msh","xmvsf","mshvsf","mvsc","sfexp","sfex2p"}){
  const std::string all=mcl::buildOneButtonLua(g,true,true);
  assert(!all.empty()&&all.find("emu.add_machine_frame_notifier")!=std::string::npos);
  assert(all.find("MOVES.BUTTON1")!=std::string::npos&&all.find("SPECIALS, SUPERS = true, true")!=std::string::npos);
  assert(mcl::buildOneButtonLua(g,false,true).find("SPECIALS, SUPERS = false, true")!=std::string::npos);
 }
 assert(mcl::buildOneButtonLua("xmcota",false,false).empty());
 assert(mcl::buildOneButtonLua("mvsc2",true,true).empty()); // Flycast: done by the front end
 assert(mcl::buildOneButtonLua("sfexp",true,true).find("SUPER = 'double'")!=std::string::npos);
 assert(mcl::buildOneButtonLua("xmcota",true,true).find("SUPER = 'three'")!=std::string::npos);
 assert(mcl::buildOneButtonLua("mvsc",true,true).find("active=16726016")!=std::string::npos); // 0xFF3800 slot flag
 std::cout<<"PASS: 1-button specials/supers scripts for the 7 MAME games\n";
}
