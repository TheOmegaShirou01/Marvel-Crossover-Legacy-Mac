#include "app_settings.hpp"
#include <algorithm>
#include <fstream>
#include <string>
namespace mcl {
namespace {
bool parseInt(const std::string& v,int lo,int hi,int& out){
 if(v.empty()||v.size()>3)return false;
 for(char c:v)if(c<'0'||c>'9')return false;
 out=std::clamp(std::stoi(v),lo,hi);return true;
}
bool parseBool(const std::string& v,bool& out){
 if(v=="1"||v=="on"||v=="yes"){out=true;return true;}
 if(v=="0"||v=="off"||v=="no"){out=false;return true;}
 return false;
}
}
bool AppSettings::load(const std::filesystem::path& file){
 std::ifstream in(file);if(!in)return false;
 std::string line;
 while(std::getline(in,line)){
  if(!line.empty()&&line.back()=='\r')line.pop_back();
  const auto eq=line.find('=');if(eq==std::string::npos)continue;
  const std::string k=line.substr(0,eq),v=line.substr(eq+1);
  if(k=="music_volume")parseInt(v,0,10,musicVolume);
  else if(k=="voice_volume")parseInt(v,0,10,voiceVolume);
  else if(k=="fullscreen")parseBool(v,fullscreen);
  else if(k=="games_fullscreen")parseBool(v,gamesFullscreen);
  else if(k=="background_video")parseBool(v,backgroundVideo);
  else if(k=="now_playing")parseBool(v,nowPlaying);
 }
 return true;
}
bool AppSettings::save(const std::filesystem::path& file) const{
 std::error_code ec;if(file.has_parent_path())std::filesystem::create_directories(file.parent_path(),ec);
 std::ofstream out(file,std::ios::trunc);if(!out)return false;
 out<<"music_volume="<<musicVolume<<"\nvoice_volume="<<voiceVolume
    <<"\nfullscreen="<<(fullscreen?1:0)<<"\ngames_fullscreen="<<(gamesFullscreen?1:0)
    <<"\nbackground_video="<<(backgroundVideo?1:0)<<"\nnow_playing="<<(nowPlaying?1:0)<<"\n";
 return static_cast<bool>(out);
}
}
