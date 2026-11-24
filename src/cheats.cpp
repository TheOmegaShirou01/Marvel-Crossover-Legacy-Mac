#include "cheats.hpp"
#include <cstdio>
#include <fstream>
#include <sstream>
namespace mcl {
namespace {
using V=std::vector<Poke>;
Poke b(std::uint32_t a,std::uint32_t v){return {a,1,v};}
Poke bIf(std::uint32_t a,std::uint32_t v,std::uint32_t ifAddr,std::uint8_t ifValue,std::uint32_t ifNonZero=0){return {a,1,v,ifAddr,ifValue,ifNonZero};}
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
   // Secret characters (verified in matches on xmcotau): the character byte is held from the select screen on.
   {"p1_secret","Secret Character P1",V{},{{"Juggernaut",V{b(0xFF4051,0x14)}},{"Magneto",V{b(0xFF4051,0x16)}},{"Akuma",V{b(0xFF4051,0x18)}}}},
   {"p2_secret","Secret Character P2",V{},{{"Juggernaut",V{b(0xFF4451,0x14)}},{"Magneto",V{b(0xFF4451,0x16)}},{"Akuma",V{b(0xFF4451,0x18)}}}},
   {"akuma_boss","Fight Akuma (secret boss)",V{b(0xFF4921,0x18)}},
  }},
  {"msh",{ // set mshud
   {"time","Infinite Time",V{b(0xFF4808,0x99)}},
   {"p1_energy","Infinite Health P1",V{b(0xFF4191,0x90)}},
   {"p2_energy","Infinite Health P2",V{b(0xFF4591,0x90)}},
   {"p1_power","Max Power P1",V{b(0xFF4198,0x03),b(0xFF4195,0x90)}},
   {"p2_power","Max Power P2",V{b(0xFF4598,0x03),b(0xFF4595,0x90)}},
   // Secret characters (verified in matches on mshud). The byte is held only while that player
   // is really playing (FF4003 / FF4403 = 1): the attract demo crashes on Thanos otherwise.
   // Anita (unfinished test character) has no select-screen portrait, so her byte is only held
   // once the selection is confirmed (FF4920 != 0); from there she loads with her own graphics.
   {"p1_secret","Secret Character P1",V{},{{"Dr. Doom",V{bIf(0xFF4051,0x14,0xFF4003,1)}},{"Thanos",V{bIf(0xFF4051,0x16,0xFF4003,1)}},{"Anita",V{bIf(0xFF4051,0x18,0xFF4003,1,0xFF4920)}}}},
   {"p2_secret","Secret Character P2",V{},{{"Dr. Doom",V{bIf(0xFF4451,0x14,0xFF4403,1)}},{"Thanos",V{bIf(0xFF4451,0x16,0xFF4403,1)}},{"Anita",V{bIf(0xFF4451,0x18,0xFF4403,1,0xFF4920)}}}},
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
   // Secret characters incl. Norimaro (verified in matches on mshvsfu1d)
   {"p1_secret","Secret Character P1",V{},{{"Norimaro",V{b(0xFF3853,0x22)}},{"Mech-Akuma",V{b(0xFF3853,0x26)}},{"Mech-Zangief",V{b(0xFF3853,0x28)}},{"Dark Sakura",V{b(0xFF3853,0x2A)}},{"Shadow",V{b(0xFF3853,0x2C)}},{"U.S. Agent",V{b(0xFF3853,0x2E)}},{"Mephisto",V{b(0xFF3853,0x30)}},{"Armored Spider-Man",V{b(0xFF3853,0x32)}}}},
   {"p1_secret2","Secret Partner P1",V{},{{"Norimaro",V{b(0xFF4053,0x22)}},{"Mech-Akuma",V{b(0xFF4053,0x26)}},{"Mech-Zangief",V{b(0xFF4053,0x28)}},{"Dark Sakura",V{b(0xFF4053,0x2A)}},{"Shadow",V{b(0xFF4053,0x2C)}},{"U.S. Agent",V{b(0xFF4053,0x2E)}},{"Mephisto",V{b(0xFF4053,0x30)}},{"Armored Spider-Man",V{b(0xFF4053,0x32)}}}},
   {"p2_secret","Secret Character P2",V{},{{"Norimaro",V{b(0xFF3C53,0x22)}},{"Mech-Akuma",V{b(0xFF3C53,0x26)}},{"Mech-Zangief",V{b(0xFF3C53,0x28)}},{"Dark Sakura",V{b(0xFF3C53,0x2A)}},{"Shadow",V{b(0xFF3C53,0x2C)}},{"U.S. Agent",V{b(0xFF3C53,0x2E)}},{"Mephisto",V{b(0xFF3C53,0x30)}},{"Armored Spider-Man",V{b(0xFF3C53,0x32)}}}},
   {"p2_secret2","Secret Partner P2",V{},{{"Norimaro",V{b(0xFF4453,0x22)}},{"Mech-Akuma",V{b(0xFF4453,0x26)}},{"Mech-Zangief",V{b(0xFF4453,0x28)}},{"Dark Sakura",V{b(0xFF4453,0x2A)}},{"Shadow",V{b(0xFF4453,0x2C)}},{"U.S. Agent",V{b(0xFF4453,0x2E)}},{"Mephisto",V{b(0xFF4453,0x30)}},{"Armored Spider-Man",V{b(0xFF4453,0x32)}}}},
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
   // Secret characters (verified in matches on mvscu)
   {"p1_secret","Secret Character P1",V{},{{"Roll",V{b(0xFF3053,0x10)}},{"Red Venom",V{b(0xFF3053,0x24)}},{"Orange Hulk",V{b(0xFF3053,0x26)}},{"Gold War Machine",V{b(0xFF3053,0x28)}},{"Shadow Lady",V{b(0xFF3053,0x2A)}},{"Lilith",V{b(0xFF3053,0x2C)}}}},
   {"p1_secret2","Secret Partner P1",V{},{{"Roll",V{b(0xFF3853,0x10)}},{"Red Venom",V{b(0xFF3853,0x24)}},{"Orange Hulk",V{b(0xFF3853,0x26)}},{"Gold War Machine",V{b(0xFF3853,0x28)}},{"Shadow Lady",V{b(0xFF3853,0x2A)}},{"Lilith",V{b(0xFF3853,0x2C)}}}},
   {"p2_secret","Secret Character P2",V{},{{"Roll",V{b(0xFF3453,0x10)}},{"Red Venom",V{b(0xFF3453,0x24)}},{"Orange Hulk",V{b(0xFF3453,0x26)}},{"Gold War Machine",V{b(0xFF3453,0x28)}},{"Shadow Lady",V{b(0xFF3453,0x2A)}},{"Lilith",V{b(0xFF3453,0x2C)}}}},
   {"p2_secret2","Secret Partner P2",V{},{{"Roll",V{b(0xFF3C53,0x10)}},{"Red Venom",V{b(0xFF3C53,0x24)}},{"Orange Hulk",V{b(0xFF3C53,0x26)}},{"Gold War Machine",V{b(0xFF3C53,0x28)}},{"Shadow Lady",V{b(0xFF3C53,0x2A)}},{"Lilith",V{b(0xFF3C53,0x2C)}}}},
  }},
  // CHEAT EX games (ZN-1 / ZN-2, PlayStation-based RAM addresses).
  {"sfexp",{ // set sfexp (USA 970407)
   {"time","Infinite Time",V{b(0x31D5CC,0x63)}},
   {"p1_energy","Infinite Health P1",V{b(0x3249E4,0xC8)}},
   {"p2_energy","Infinite Health P2",V{b(0x325E58,0xC8)}},
   {"p1_power","Max Special P1",V{b(0x3249E5,0x96)}},
   {"p2_power","Max Special P2",V{b(0x325E59,0x96)}},
   // Secret characters (verified in matches on sfexp)
   {"p1_secret","Secret Character P1",V{},{{"Kairi",V{b(0x32358A,0x00)}},{"Akuma",V{b(0x32358A,0x10)}},{"Evil Ryu",V{b(0x32358A,0x12)}},{"Bloody Hokuto",V{b(0x32358A,0x13)}},{"M. Bison",V{b(0x32358A,0x14)}},{"Garuda",V{b(0x32358A,0x15)}},{"Cycloid-Gamma",V{b(0x32358A,0x16)}},{"Cycloid-Beta",V{b(0x32358A,0x17)}}}},
   {"p2_secret","Secret Character P2",V{},{{"Kairi",V{b(0x3249FE,0x00)}},{"Akuma",V{b(0x3249FE,0x10)}},{"Evil Ryu",V{b(0x3249FE,0x12)}},{"Bloody Hokuto",V{b(0x3249FE,0x13)}},{"M. Bison",V{b(0x3249FE,0x14)}},{"Garuda",V{b(0x3249FE,0x15)}},{"Cycloid-Gamma",V{b(0x3249FE,0x16)}},{"Cycloid-Beta",V{b(0x3249FE,0x17)}}}},
  }},
  {"sfex2p",{ // set sfex2p
   {"time","Infinite Time",V{b(0x2AE5BC,0x63)}},
   {"p1_energy","Infinite Health P1",V{b(0x2B68A8,0xC8)}},
   {"p2_energy","Infinite Health P2",V{b(0x2B80AC,0xC8)}},
   {"p1_power","Max Special P1",V{b(0x2B68A9,0x96)}},
   {"p2_power","Max Special P2",V{b(0x2B80AD,0x8D)}},
   // Secret characters (verified in matches on sfex2p)
   {"p1_secret","Secret Character P1",V{},{{"Garuda",V{b(0x01F010,0x15)}},{"Shadowgeist",V{b(0x01F010,0x16)}},{"Kairi",V{b(0x01F010,0x17)}}}},
   {"p2_secret","Secret Character P2",V{},{{"Garuda",V{b(0x01F011,0x15)}},{"Shadowgeist",V{b(0x01F011,0x16)}},{"Kairi",V{b(0x01F011,0x17)}}}},
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
const std::vector<std::string>& exCheatGames(){
 static const std::vector<std::string> g{"sfexp","sfex2p"};return g;
}
bool CheatSettings::load(){
 on_.clear();choice_.clear();std::ifstream in(file_);if(!in)return false;
 std::string game,id;
 while(in>>game>>id){
  const auto eq=id.find('=');
  if(eq!=std::string::npos){ // choice cheat: "game id=n"
   int n=0;try{n=std::stoi(id.substr(eq+1));}catch(...){continue;}
   setChoice(game,id.substr(0,eq),n);continue;
  }
  for(const auto& c:cheatsFor(game))if(id==c.id&&!c.isChoice()){on_[game].insert(id);break;} // ignore unknown entries
 }
 return true;
}
int CheatSettings::choice(const std::string& game,const std::string& id) const{
 auto g=choice_.find(game);if(g==choice_.end())return 0;
 auto c=g->second.find(id);return c==g->second.end()?0:c->second;
}
void CheatSettings::setChoice(const std::string& game,const std::string& id,int option){
 for(const auto& c:cheatsFor(game)){
  if(id!=c.id||!c.isChoice())continue;
  if(option<=0||option>static_cast<int>(c.choices.size())){auto g=choice_.find(game);if(g!=choice_.end()){g->second.erase(id);if(g->second.empty())choice_.erase(g);}}
  else choice_[game][id]=option;
  return;
 }
}
bool CheatSettings::save() const{
 std::error_code ec;if(file_.has_parent_path())std::filesystem::create_directories(file_.parent_path(),ec);
 std::ofstream out(file_,std::ios::trunc);if(!out)return false;
 for(const auto& [game,ids]:on_)for(const auto& id:ids)out<<game<<' '<<id<<'\n';
 for(const auto& [game,ids]:choice_)for(const auto& [id,n]:ids)out<<game<<' '<<id<<'='<<n<<'\n';
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
bool CheatSettings::anyEnabled(const std::string& game) const{
 auto it=on_.find(game);if(it!=on_.end()&&!it->second.empty())return true;
 auto c=choice_.find(game);return c!=choice_.end()&&!c->second.empty();
}
std::vector<Poke> CheatSettings::activePokes(const std::string& game) const{
 std::vector<Poke> out;
 for(const auto& c:cheatsFor(game)){
  if(c.isChoice()){const int n=choice(game,c.id);if(n>0){const auto& p=c.choices[static_cast<size_t>(n-1)].pokes;out.insert(out.end(),p.begin(),p.end());}}
  else if(enabled(game,c.id))out.insert(out.end(),c.pokes.begin(),c.pokes.end());
 }
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
 char buf[128];
 for(const auto& p:pokes){std::snprintf(buf,sizeof buf," {0x%06X, %u, 0x%X, 0x%06X, 0x%X, 0x%06X},\n",p.addr,static_cast<unsigned>(p.width),p.value,p.ifAddr,static_cast<unsigned>(p.ifValue),p.ifNonZero);s<<buf;}
 s<<"}\n";
 s<<"mcl_cheat_subscription = emu.add_machine_frame_notifier(function()\n";
 s<<"  for _, p in ipairs(pokes) do\n";
 s<<"    if (p[4] ~= 0 and mem:read_u8(p[4]) ~= p[5]) or (p[6] ~= 0 and mem:read_u8(p[6]) == 0) then -- guarded poke: not now\n";
 s<<"    elseif p[2] == 1 then mem:write_u8(p[1], p[3])\n";
 s<<"    elseif p[2] == 2 then mem:write_u16(p[1], p[3])\n";
 s<<"    else mem:write_u32(p[1], p[3]) end\n";
 s<<"  end\n";
 s<<"end)\n";
 return s.str();
}
}
