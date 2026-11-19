#include "cheat_ex_settings.hpp"
#include <fstream>
#include <stdexcept>
#include <system_error>
namespace mcl {
static std::size_t index(const std::string& id){
 if(id=="sfexp")return 0;
 if(id=="sfex2p")return 1;
 throw std::invalid_argument("Only EX Plus and EX2 Plus have CHEAT EX settings");
}
ExCheatOptions& ExCheatSettings::options(const std::string& id){return games_.at(index(id));}
const ExCheatOptions& ExCheatSettings::options(const std::string& id) const{return games_.at(index(id));}
bool ExCheatSettings::load(){
 games_={};std::ifstream input(file_,std::ios::binary);
 if(!input)return false;
 std::string marker,a,b,extra;
 if(!std::getline(input,marker)||marker!="MCL_EX_CHEATS_1"||!std::getline(input,a)||!std::getline(input,b)||std::getline(input,extra))return false;
 auto parse=[](const std::string& line,const std::string& id,ExCheatOptions& opts){
  if(line.size()!=id.size()+5||line.compare(0,id.size(),id)!=0||line[id.size()]!='=')return false;
  for(std::size_t i=0;i<4;++i){auto ch=line[id.size()+1+i];if(ch!='0'&&ch!='1')return false;opts.flags[i]=ch=='1';}
  return true;
 };
 ExCheatOptions first,second;
 if(!parse(a,"sfexp",first)||!parse(b,"sfex2p",second))return false;
 games_[0]=first;games_[1]=second;return true;
}
bool ExCheatSettings::save()const{
 std::error_code ec;
 std::filesystem::create_directories(file_.parent_path(),ec);
 if(ec)return false;
 // Generate complete output before modifying existing settings.
 auto tmp=file_;tmp+=".tmp";
 {
  std::ofstream out(tmp,std::ios::binary|std::ios::trunc);
  if(!out)return false;
  out<<"MCL_EX_CHEATS_1\n";
  for(std::size_t i=0;i<2;++i){out<<(i==0?"sfexp=":"sfex2p=");for(bool v:games_[i].flags)out<<(v?'1':'0');out<<'\n';}
  out.flush();if(!out){out.close();std::filesystem::remove(tmp,ec);return false;}
 }
 // On Windows a destination that already exists must be replaced explicitly.
 std::filesystem::remove(file_,ec);ec.clear();
 std::filesystem::rename(tmp,file_,ec);
 if(ec){std::filesystem::remove(tmp,ec);return false;}
 return true;
}
}
