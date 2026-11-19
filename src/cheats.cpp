#include "cheats.hpp"
#include <cstdio>
#include <fstream>
#include <sstream>
namespace mcl {
namespace {
using V=std::vector<Poke>;
Poke b(std::uint32_t a,std::uint32_t v){return {a,1,v};}
const std::map<std::string,std::vector<CheatDef>>& table(){
 static const std::map<std::string,std::vector<CheatDef>> t={
  {"xmcota",{ // set xmcotau
   {"time","Infinite Time",V{b(0xFF4808,0x99)}},
   {"p1_energy","Infinite Health P1",V{b(0xFF4191,0x8F)}},
   {"p2_energy","Infinite Health P2",V{b(0xFF4591,0x8F)}},
   {"p1_power","Max Power P1",V{b(0xFF4198,0x02),b(0xFF4195,0x8E)}},
   {"p2_power","Max Power P2",V{b(0xFF4598,0x02),b(0xFF4595,0x8E)}},
   {"p1_nofaint","Never Dizzy P1",V{b(0xFF40FD,0x00)}},
   {"p2_nofaint","Never Dizzy P2",V{b(0xFF44FD,0x00)}},
  }},
  {"msh",{ // set mshud
   {"time","Infinite Time",V{b(0xFF4808,0x99)}},
   {"p1_energy","Infinite Health P1",V{b(0xFF4191,0x90)}},
   {"p2_energy","Infinite Health P2",V{b(0xFF4591,0x90)}},
   {"p1_power","Max Power P1",V{b(0xFF4198,0x03),b(0xFF4195,0x90)}},
   {"p2_power","Max Power P2",V{b(0xFF4598,0x03),b(0xFF4595,0x90)}},
  }},
  {"xmvsf",{ // set xmvsfur2
   {"time","Infinite Time",V{b(0xFF5008,0x99)}},
   {"p1_energy","Infinite Health P1 (both)",V{b(0xFF4211,0x90),b(0xFF421B,0x90),b(0xFF4A11,0x90),b(0xFF4A1B,0x90)}},
   {"p2_energy","Infinite Health P2 (both)",V{b(0xFF4611,0x90),b(0xFF461B,0x90),b(0xFF4E11,0x90),b(0xFF4E1B,0x90)}},
   {"p1_power","Max Power P1",V{b(0xFF4214,0x03),b(0xFF4213,0x90)}},
   {"p2_power","Max Power P2",V{b(0xFF4614,0x03),b(0xFF4613,0x90)}},
   {"p1_apocalypse","Play as Apocalypse P1",V{b(0xFF4053,0x10),b(0xFF4853,0x10),b(0xFF50B1,0x10),b(0xFF50B3,0x10),b(0xFF5113,0x00)}},
   {"p2_apocalypse","Play as Apocalypse P2",V{b(0xFF4453,0x10),b(0xFF4C53,0x10),b(0xFF50B5,0x10),b(0xFF50B7,0x10),b(0xFF5113,0x00)}},
  }},
  {"mshvsf",{ // set mshvsfu1d
   {"time","Infinite Time",V{b(0xFF4808,0x99)}},
   {"p1_energy","Infinite Health P1 (both)",V{b(0xFF3A51,0x90),b(0xFF3A5B,0x90),b(0xFF4251,0x90),b(0xFF425B,0x90)}},
   {"p2_energy","Infinite Health P2 (both)",V{b(0xFF3E51,0x90),b(0xFF3E5B,0x90),b(0xFF4651,0x90),b(0xFF465B,0x90)}},
   {"p1_power","Max Power P1",V{b(0xFF3A54,0x03),b(0xFF4254,0x03)}},
   {"p2_power","Max Power P2",V{b(0xFF3E54,0x03),b(0xFF4654,0x03)}},
   {"p1_apocalypse","Play as Apocalypse P1",V{b(0xFF3853,0x00),b(0xFF4053,0x00),b(0xFF48B1,0x00),b(0xFF48B3,0x00),b(0xFF4913,0x00)}},
   {"p2_apocalypse","Play as Apocalypse P2",V{b(0xFF3C53,0x00),b(0xFF4453,0x00),b(0xFF48B5,0x00),b(0xFF48B7,0x00),b(0xFF4913,0x00)}},
  }},
  {"mvsc",{ // set mvscu
   {"time","Infinite Time",V{b(0xFF4008,0x99)}},
   {"p1_energy","Infinite Health P1 (both)",V{b(0xFF3271,0x90),b(0xFF327B,0x90),b(0xFF3A71,0x90),b(0xFF3A7B,0x90)}},
   {"p2_energy","Infinite Health P2 (both)",V{b(0xFF3671,0x90),b(0xFF367B,0x90),b(0xFF3E71,0x90),b(0xFF3E7B,0x90)}},
   {"p1_power","Max Hyper Combo P1",V{b(0xFF3274,0x03),b(0xFF3A74,0x03)}},
   {"p2_power","Max Hyper Combo P2",V{b(0xFF3674,0x03),b(0xFF3E74,0x03)}},
   {"p1_strikers","Infinite Partner Strikes P1",V{b(0xFF32C6,0x09)}},
   {"p2_strikers","Infinite Partner Strikes P2",V{b(0xFF36C6,0x09)}},
   {"all_chars","Unlock Hidden Characters",V{b(0xFF4199,0x40),b(0xFF41C9,0x40)}},
   {"p1_onslaught","Play as Onslaught P1 (Vs.)",V{b(0xFF40B1,0x22),b(0xFF40B3,0x22),b(0xFF3003,0x01),b(0xFF3803,0x01),b(0xFF415E,0x22),b(0xFF415F,0x22)}},
   {"p2_onslaught","Play as Onslaught P2 (Vs.)",V{b(0xFF40B5,0x22),b(0xFF40B7,0x22),b(0xFF3403,0x01),b(0xFF3C03,0x01),b(0xFF415E,0x22),b(0xFF415F,0x22)}},
  }},
 };
 return t;
}
const std::vector<CheatDef> kNone;
}
const std::vector<CheatDef>& cheatsFor(const std::string& game){
 const auto& t=table();auto it=t.find(game);return it==t.end()?kNone:it->second;
}
const std::vector<std::string>& marvelCheatGames(){
 static const std::vector<std::string> g{"xmcota","msh","xmvsf","mshvsf","mvsc"};return g;
}
bool CheatSettings::load(){
 on_.clear();std::ifstream in(file_);if(!in)return false;
 std::string game,id;
 while(in>>game>>id){
  for(const auto& c:cheatsFor(game))if(id==c.id){on_[game].insert(id);break;} // ignore unknown entries
 }
 return true;
}
bool CheatSettings::save() const{
 std::error_code ec;if(file_.has_parent_path())std::filesystem::create_directories(file_.parent_path(),ec);
 std::ofstream out(file_,std::ios::trunc);if(!out)return false;
 for(const auto& [game,ids]:on_)for(const auto& id:ids)out<<game<<' '<<id<<'\n';
 return static_cast<bool>(out);
}
bool CheatSettings::enabled(const std::string& game,const std::string& id) const{
 auto it=on_.find(game);return it!=on_.end()&&it->second.count(id)>0;
}
void CheatSettings::set(const std::string& game,const std::string& id,bool on){
 bool known=false;for(const auto& c:cheatsFor(game))if(id==c.id)known=true;
 if(!known)return;
 if(on)on_[game].insert(id);else{auto it=on_.find(game);if(it!=on_.end()){it->second.erase(id);if(it->second.empty())on_.erase(it);}}
}
bool CheatSettings::anyEnabled(const std::string& game) const{auto it=on_.find(game);return it!=on_.end()&&!it->second.empty();}
std::vector<Poke> CheatSettings::activePokes(const std::string& game) const{
 std::vector<Poke> out;
 for(const auto& c:cheatsFor(game))if(enabled(game,c.id))out.insert(out.end(),c.pokes.begin(),c.pokes.end());
 return out;
}
std::vector<Poke> exCheatPokes(const std::string& game,bool health,bool power,bool time){
 std::vector<Poke> out;
 if(game=="sfexp"){ // USA 970407
  if(time)out.push_back(b(0x31D5CC,0x63));
  if(health)out.push_back(b(0x3249E4,0xC8));
  if(power)out.push_back(b(0x3249E5,0x96));
 }else if(game=="sfex2p"){ // set sfex2p
  if(time)out.push_back(b(0x2AE5BC,0x63));
  if(health)out.push_back(b(0x2B68A8,0xC8));
  if(power)out.push_back(b(0x2B68A9,0x96));
 }
 return out;
}
std::string buildCheatLua(const std::string& title,const std::vector<Poke>& pokes){
 std::ostringstream s;
 std::string safe;for(char c:title)if(c!='\n'&&c!='\r')safe+=c;
 s<<"-- Marvel: Crossover Legacy cheats: "<<safe<<"\n";
 s<<"-- Generated by the frontend for this launch only. Rewrites memory every frame.\n";
 s<<"local cpu = manager.machine.devices[':maincpu']\n";
 s<<"if not cpu then return end\n";
 s<<"local mem = cpu.spaces['program']\n";
 s<<"local pokes = {\n";
 char buf[64];
 for(const auto& p:pokes){std::snprintf(buf,sizeof buf," {0x%06X, %u, 0x%X},\n",p.addr,static_cast<unsigned>(p.width),p.value);s<<buf;}
 s<<"}\n";
 s<<"mcl_cheat_subscription = emu.add_machine_frame_notifier(function()\n";
 s<<"  for _, p in ipairs(pokes) do\n";
 s<<"    if p[2] == 1 then mem:write_u8(p[1], p[3])\n";
 s<<"    elseif p[2] == 2 then mem:write_u16(p[1], p[3])\n";
 s<<"    else mem:write_u32(p[1], p[3]) end\n";
 s<<"  end\n";
 s<<"end)\n";
 return s.str();
}
}
