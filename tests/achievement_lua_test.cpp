#include "achievement_lua.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
// Checks the generated detector script; with a folder argument it also writes each
// game's script there (used for headless MAME verification runs).
int main(int argc,char** argv){
 const char* games[]={"xmcota","msh","xmvsf","mshvsf","mvsc","sfexp","sfex2p"};
 for(const char* g:games){
  const auto p=mcl::detectorProfile(g);
  assert(p.game==g&&!p.p1.empty()&&p.p1.size()==p.p2.size()&&p.timer&&p.stage);
  const std::string lua=mcl::buildAchievementLua(g,"C:/x/game_events.txt");
  assert(lua.find("MCL_EV = {file=[[C:/x/game_events.txt]]")==0);
  assert(lua.find("add_machine_frame_notifier")!=std::string::npos);
  assert(lua.find("write_u")==std::string::npos); // read-only: never pokes the game
  if(argc>1){std::ofstream(std::string(argv[1])+"/"+g+".lua")<<lua;}
 }
 assert(mcl::buildAchievementLua("mvsc2","x").empty()); // Flycast: no Lua hook
 assert(mcl::detectorProfile("mshvsf").bosses.size()==2);
 const auto ev=mcl::parseDetectedEvents("coin -1 0 0\nmatch_win 3 1 1\nboss_defeated 7 0 0 onslaught\ngarbage\n\n");
 assert(ev.size()==3);
 assert(ev[1].type=="match_win"&&ev[1].stage==3&&ev[1].continued&&ev[1].allAlive);
 assert(ev[2].detail=="onslaught"&&!ev[2].continued);
 // character ids from the MAME cheat database
 assert(mcl::characterName("mshvsf",0x2A)=="Dark Sakura"&&mcl::characterName("sfexp",0x12)=="Evil Ryu");
 assert(mcl::characterName("mvsc",0x99).empty()&&mcl::characterName("mvsc2",0).empty());
 const auto team=mcl::charactersFromDetail("xmvsf","c=00,12");
 assert(team.size()==2&&team[0]=="Wolverine"&&team[1]=="Ryu");
 assert(mcl::charactersFromDetail("xmvsf","onslaught").empty());
 assert(mcl::buildAchievementLua("xmcota","x").find("enc=0xFF4451, encv=0x18")!=std::string::npos);
 std::cout<<"PASS: detector scripts for 7 games, read-only, event parsing\n";
}
