#include "music.hpp"
#include <algorithm>
#include <cstdio>
#include <cctype>
#include <fstream>
#include <sstream>
namespace mcl {
std::vector<Track> loadMusicCatalog(const std::filesystem::path& file){
 std::vector<Track> out;
 std::ifstream in(file,std::ios::binary);
 std::string line;
 while(std::getline(in,line)){
  if(!line.empty()&&line.back()=='\r')line.pop_back();
  if(line.empty()||line[0]=='#')continue;
  std::vector<std::string> f;std::stringstream ss(line);std::string cell;
  while(std::getline(ss,cell,'\t'))f.push_back(cell);
  if(f.size()<5)continue;
  Track t;t.game=f[0];t.path=f[1];t.name=f[4];
  try{t.startMs=std::stoi(f[2]);t.endMs=std::stoi(f[3]);}catch(...){continue;}
  if(t.game.empty()||t.path.empty()||t.endMs<=t.startMs||t.path.find("..")!=std::string::npos)continue;
  out.push_back(std::move(t));
 }
 return out;
}
static std::string lower(std::string s){for(auto& c:s)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));return s;}
bool isMenuMusic(const Track& t){
 if(t.endMs-t.startMs<25000)return false;
 const std::string n=lower(t.name),p=lower(t.path);
 for(const char* bad:{"continue","game over","hurry","here comes","victory","versus","results","stage clear",
                      "ranking","unused","other_audio/","adx_cont","adx_over","adx_here","adx_wins","adx_rank"})
  if(n.find(bad)!=std::string::npos||p.find(bad)!=std::string::npos)return false;
 return true;
}
const std::vector<std::string>& marvelMusicGames(){static const std::vector<std::string> g{"xmcota","msh","xmvsf","mshvsf","mvsc","mvsc2"};return g;}
const std::vector<std::string>& exMusicGames(){static const std::vector<std::string> g{"sfexp","sfex2p"};return g;}
std::string trackCaption(const Track& t){
 std::string n=t.name;
 // "Street Fighter EX Plus Alpha - Arabesque" -> "Arabesque"
 const auto dash=n.find(" - ");if(dash!=std::string::npos)n=n.substr(dash+3);
 // Raw rip names (SLUS-00257_MUS01_..., T-1226G_05, adx_s040) have no title.
 const bool raw=n.rfind("SLUS-",0)==0||n.rfind("T-",0)==0||n.rfind("adx_",0)==0;
 if(raw){
  if(n.rfind("adx_",0)==0){ // MvC2 Dreamcast ADX names
   static const std::map<std::string,std::string> mvc2={{"adx_menu","Menu"},{"adx_selc","Character Select"},
    {"adx_open","Opening"},{"adx_staf","Staff Roll"},{"adx_capl","Capcom Logo"},{"adx_netw","Network"}};
   const auto it=mvc2.find(n.substr(0,8));if(it!=mvc2.end())return it->second;
   if(n.size()>=8&&n[4]=='s'&&std::isxdigit(static_cast<unsigned char>(n[6])))
    return "Stage Theme "+std::to_string(std::stoi(n.substr(6,1),nullptr,16)+1);
  }
  // PlayStation / Saturn rips carry no song titles.
  const auto part=t.name.find(" (part ");
  return "Original Soundtrack"+(part!=std::string::npos?t.name.substr(part):std::string());
 }
 return n;
}
const std::vector<std::string>& jukeboxGames(){
 static const std::vector<std::string> g{"xmcota","msh","xmvsf","mshvsf","mvsc","mvsc2","sfexp","sfex2p","sfex3"};return g;
}
std::vector<Track> jukeboxTracks(const std::vector<Track>& all,const std::string& game){
 std::vector<Track> v;
 for(const auto& t:all)if(t.game==game)v.push_back(t);
 std::stable_sort(v.begin(),v.end(),[](const Track& a,const Track& b){return a.path!=b.path?a.path<b.path:a.startMs<b.startMs;});
 return v;
}
std::map<std::string,std::string> loadTrackTitles(const std::filesystem::path& file){
 std::map<std::string,std::string> m;
 std::ifstream in(file);std::string line;
 while(std::getline(in,line)){
  if(!line.empty()&&line.back()=='\r')line.pop_back();
  if(line.empty()||line[0]=='#')continue;
  const auto tab=line.find('\t');
  if(tab==std::string::npos||tab==0||tab+1>=line.size())continue;
  m[line.substr(0,tab)]=line.substr(tab+1);
 }
 return m;
}
std::string jukeboxTitle(const Track& t,int number,const std::map<std::string,std::string>& titles){
 const auto it=titles.find(t.path);
 if(it!=titles.end())return it->second;
 const std::string cap=trackCaption(t);
 if(cap.rfind("Original Soundtrack",0)!=0)return cap;
 char b[16];std::snprintf(b,sizeof b,"Track %02d",number);
 return b;
}
MusicRotation::MusicRotation(const std::vector<Track>& all,const std::vector<std::string>& games,std::uint32_t seed):rng_(seed){
 for(const auto& t:all)
  if(isMenuMusic(t)&&std::find(games.begin(),games.end(),t.game)!=games.end())tracks_.push_back(t);
 for(const auto& t:tracks_)plays_[t.game]=0;
}
void MusicRotation::refill(const std::string& game){
 auto& bag=bags_[game];bag.clear();
 for(std::size_t i=0;i<tracks_.size();++i)if(tracks_[i].game==game&&i!=lastTrack_)bag.push_back(i);
 if(bag.empty())for(std::size_t i=0;i<tracks_.size();++i)if(tracks_[i].game==game)bag.push_back(i);
 std::shuffle(bag.begin(),bag.end(),rng_);
}
const Track* MusicRotation::next(){
 if(tracks_.empty())return nullptr;
 // Least-played game, avoiding the previous game when possible; random among ties.
 std::vector<std::string> cands;int best=1<<30;
 for(const auto& [g,n]:plays_){
  if(plays_.size()>1&&g==lastGame_)continue;
  if(n<best){best=n;cands.clear();}
  if(n==best)cands.push_back(g);
 }
 std::uniform_int_distribution<std::size_t> d(0,cands.size()-1);
 const std::string game=cands[d(rng_)];
 auto& bag=bags_[game];
 if(bag.empty())refill(game);
 std::size_t idx=bag.back();bag.pop_back();
 if(idx==lastTrack_&&!bag.empty()){std::swap(idx,bag.back());}
 lastTrack_=idx;lastGame_=game;++plays_[game];
 return &tracks_[idx];
}
const Track* MusicRotation::pick(const std::string& path){
 for(std::size_t i=0;i<tracks_.size();++i)if(tracks_[i].path==path){
  auto& bag=bags_[tracks_[i].game];
  bag.erase(std::remove(bag.begin(),bag.end(),i),bag.end());
  lastTrack_=i;lastGame_=tracks_[i].game;++plays_[lastGame_];
  return &tracks_[i];
 }
 return nullptr;
}
}
