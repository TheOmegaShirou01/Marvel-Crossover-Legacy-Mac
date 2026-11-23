#include "../src/cheats.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
namespace fs=std::filesystem;
int main(int argc,char** argv){
 // Every Marvel CHEATS game has cheats with unique ids and at least one poke each.
 for(const auto& g:mcl::marvelCheatGames()){
  const auto& list=mcl::cheatsFor(g);
  assert(list.size()>=5);
  std::set<std::string> ids;
  for(const auto& c:list){
   assert(ids.insert(c.id).second);
   if(c.isChoice()){for(const auto& o:c.choices){assert(!o.pokes.empty());for(const auto& p:o.pokes)assert(p.width==1&&p.addr>=0xFF0000);}}
   else{assert(!c.pokes.empty());for(const auto& p:c.pokes)assert(p.width==1&&p.addr>=0xFF0000);}
  }
 }
 assert(mcl::cheatsFor("mvsc2").empty()&&mcl::cheatsFor("sfex3").empty());
 // Settings round trip, unknown entries ignored.
 const auto dir=fs::temp_directory_path()/"mcl_cheats_test";fs::remove_all(dir);
 {
  mcl::CheatSettings s(dir/"marvel_cheats.txt");
  assert(!s.load()&&!s.anyEnabled("xmcota"));
  s.set("xmcota","time",true);s.set("xmcota","p1_energy",true);s.set("xmcota","bogus",true);s.set("mvsc","all_chars",true);
  assert(s.save());
  {std::ofstream f(dir/"marvel_cheats.txt",std::ios::app);f<<"msh not_a_cheat\nnope time\n";}
  mcl::CheatSettings t(dir/"marvel_cheats.txt");assert(t.load());
  assert(t.enabled("xmcota","time")&&t.enabled("xmcota","p1_energy")&&!t.enabled("xmcota","bogus"));
  assert(!t.anyEnabled("msh")&&t.anyEnabled("mvsc"));
  const auto pokes=t.activePokes("xmcota");
  assert(pokes.size()==2&&pokes[0].addr==0xFF4808&&pokes[0].value==0x99&&pokes[1].addr==0xFF4191);
  t.set("xmcota","time",false);t.set("xmcota","p1_energy",false);assert(!t.anyEnabled("xmcota"));
 }
 // CHEAT EX switches.
 assert(mcl::exCheatPokes("sfexp",true,true,true).size()==3);
 assert(mcl::exCheatPokes("sfex2p",false,false,true).size()==1&&mcl::exCheatPokes("sfex2p",false,false,true)[0].addr==0x2AE5BC);
 assert(mcl::exCheatPokes("sfexp",false,false,false).empty()&&mcl::exCheatPokes("xmcota",true,true,true).empty());
 // Lua output.
 const std::string lua=mcl::buildCheatLua("xmcota\nevil",{{0xFF4808,1,0x99},{0xFF4191,1,0x8F}});
 assert(lua.find("{0xFF4808, 1, 0x99}")!=std::string::npos&&lua.find("{0xFF4191, 1, 0x8F}")!=std::string::npos);
 assert(lua.find("add_machine_frame_notifier")!=std::string::npos&&lua.find("\nevil")==std::string::npos);
 // Optional: dump real scripts for an in-MAME check (tests run without arguments too).
 if(argc>1&&argv[1][0]!='\0'){
  const fs::path out=argv[1];fs::create_directories(out);
  mcl::CheatSettings s(dir/"x.txt");s.set("xmcota","time",true);s.set("xmcota","p1_energy",true);
  std::ofstream(out/"xmcotau.lua")<<mcl::buildCheatLua("xmcota",s.activePokes("xmcota"));
  std::ofstream(out/"sfexp.lua")<<mcl::buildCheatLua("sfexp",mcl::exCheatPokes("sfexp",true,false,true));
 }
 // Secret characters: choice cheats round trip and become pokes
 {
  fs::create_directories(dir);
  mcl::CheatSettings s(dir/"choice.txt");
  s.setChoice("xmcota","p1_secret",3);s.setChoice("mshvsf","p1_secret",1);s.set("xmcota","time",true);
  s.setChoice("xmcota","p2_secret",9); // out of range -> off
  assert(s.choice("xmcota","p1_secret")==3&&s.choice("xmcota","p2_secret")==0&&s.anyEnabled("mshvsf"));
  assert(s.save());
  mcl::CheatSettings t(dir/"choice.txt");assert(t.load());
  assert(t.choice("xmcota","p1_secret")==3&&t.choice("mshvsf","p1_secret")==1&&t.enabled("xmcota","time"));
  bool akuma=false,norimaro=false;
  for(const auto& p:t.activePokes("xmcota"))akuma=akuma||(p.addr==0xFF4051&&p.value==0x18);
  for(const auto& p:t.activePokes("mshvsf"))norimaro=norimaro||(p.addr==0xFF3853&&p.value==0x22);
  assert(akuma&&norimaro);
  t.setChoice("xmcota","p1_secret",0);assert(t.choice("xmcota","p1_secret")==0);
  for(const char* g:{"xmcota","mshvsf","mvsc","sfexp","sfex2p"}){
   bool found=false;for(const auto& c:mcl::cheatsFor(g))found=found||(std::string(c.id)=="p1_secret"&&c.isChoice());
   assert(found);
  }
 }
 fs::remove_all(dir);
 std::cout<<"PASS: cheat tables, settings round trip, CHEAT EX mapping, Lua generation, secret characters\n";
}
