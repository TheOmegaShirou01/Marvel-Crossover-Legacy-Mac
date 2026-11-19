#include "core.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <cctype>
#include <cstdint>
#include <limits>
namespace mcl {
namespace {
std::string utcNow(){auto t=std::time(nullptr); std::tm out{};
#ifdef _WIN32
 gmtime_s(&out,&t);
#else
 gmtime_r(&t,&out);
#endif
 char b[32];std::strftime(b,sizeof b,"%Y-%m-%dT%H:%M:%SZ",&out);return b;}
bool validId(std::string s){return !s.empty()&&std::all_of(s.begin(),s.end(),[](unsigned char c){return std::isalnum(c)||c=='_'||c=='-';});}
std::vector<std::string> split(const std::string& s,char c){std::vector<std::string> out;std::istringstream in(s);std::string token;while(std::getline(in,token,c))out.push_back(token);return out;}
}
Collection::Collection(fs::path root):root_(std::move(root)){
 games_={
 {"xmcota","X-Men: Children of the Atom","1994","CPS-2","Capcom's mutant fighting game; Juggernaut and Magneto bosses, hidden Akuma."},
 {"msh","Marvel Super Heroes","1995","CPS-2","The six Infinity Gems give fighters unique abilities; Thanos is final boss."},
 {"xmvsf","X-Men vs. Street Fighter","1996","CPS-2","Two-character teams and Cross-Over Combinations; Apocalypse boss."},
 {"mshvsf","Marvel Super Heroes vs. Street Fighter","1997","CPS-2","Two-character crossover featuring hidden variants and Cyber-Akuma."},
 {"mvsc","Marvel vs. Capcom: Clash of Super Heroes","1998","CPS-2","Special Partners and Duo Team Attacks; Onslaught boss."},
 {"mvsc2","Marvel vs. Capcom 2: New Age of Heroes","2000","NAOMI","Three-character teams, assists, Hyper Combos and Abyss boss."},
 {"sfexp","Street Fighter EX Plus","1997","ZN-1","Hidden CHEAT EX arcade bonus title developed by Arika and Capcom."},
 {"sfex2p","Street Fighter EX2 Plus","1999","ZN-2","Hidden CHEAT EX arcade bonus sequel developed by Arika and Capcom."}
 };
 const std::vector<std::vector<std::pair<std::string,std::string>>> A={
 {{"Welcome to the X-Men","Win a match"},{"Mutant Power","Land an X-Power attack"},{"Taking Flight","Land a 3-hit air combo"},{"Untouchable Mutant","Win a round without taking damage"},{"Five Down","Defeat five opponents in one run"},{"Unstoppable Force","Defeat Juggernaut"},{"Master of Magnetism","Defeat Magneto"},{"The Mysterious Challenger","Encounter Akuma"},{"Akuma Unleashed","Clear Arcade as Akuma"},{"Juggernaut's Revenge","Clear Arcade as Juggernaut"},{"Magnetic Supremacy","Clear Arcade as Magneto"},{"Children of the Atom Master","Clear without a continue"}},
 {{"Earth's Mightiest","Win a match"},{"Gem Collector","Collect an Infinity Gem"},{"Power Overwhelming","Use Power Gem"},{"Time Manipulator","Use Time Gem"},{"Space Guardian","Use Space Gem"},{"Reality Breaker","Use Reality Gem"},{"Soul Survivor","Use Soul Gem"},{"Mind Over Matter","Use Mind Gem"},{"Infinity Specialist","Use each of the six gems"},{"Infinity Counter","Connect an Infinity Counter"},{"Defeat the Mad Titan","Defeat Thanos"},{"Infinity Gauntlet Master","Clear without a continue"}},
 {{"Worlds Collide","Win a match"},{"Tag Team Rookie","Tag between fighters"},{"Double Trouble","Defeat both enemy fighters"},{"Hyper Finish","Win using a Hyper Combo"},{"Cross-Over Power","Connect a Cross-Over Combination"},{"Mutant Alliance","Win with two X-Men"},{"Street Fighter Alliance","Win with two Street Fighters"},{"Perfect Partnership","Win without damage"},{"No Partner Left Behind","Win with both fighters alive"},{"Apocalypse Now","Defeat Apocalypse"},{"The Apocalypse Project","Clear as Apocalypse"},{"Ultimate Crossover","Clear without a continue"}},
 {{"Heroes Assemble","Win a match"},{"Call for Backup","Connect a Cross-Over Assist"},{"Double Impact","Connect a Cross-Over Combination"},{"Hyper Hero","Win with a Hyper Combo"},{"Perfect Victory","Win without damage"},{"Heroes Never Fall","Win five consecutive arcade matches"},{"Darkness Rises","Win as Dark Sakura"},{"Shadow Soldier","Win as Shadow Charlie"},{"Mechanical Menace","Win as Mech-Zangief"},{"Apocalypse Defeated","Defeat Apocalypse"},{"Cybernetic Nightmare","Defeat Cyber-Akuma"},{"Secret Warriors","Win with each supported secret fighter"}},
 {{"Let's Go Crazy!","Win a match"},{"Special Delivery","Connect a Special Partner attack"},{"Backup Requested","Call a Special Partner five times"},{"Power of Friendship","Connect a Cross-Over Combination"},{"Double Devastation","Activate a Duo Team Attack"},{"Perfect Tag Team","Win without damage"},{"Partner Finish","Defeat an opponent with a helper"},{"Cross-Over Champion","Win with Cross-Over Combination"},{"Secret Identity","Win as a secret fighter"},{"The Ultimate Enemy","Defeat Onslaught"},{"Heroes United","Clear with a Marvel/Capcom mixed team"},{"Clash of Champions","Clear without a continue"}},
 {{"New Age of Heroes","Win a match"},{"Triple Threat","Win with all three characters alive"},{"Backup Has Arrived","Connect an assist attack"},{"Everybody Switch!","Use all three fighters"},{"Hyper Combo Hero","Win with a Hyper Combo"},{"Delayed Reaction","Connect a Delayed Hyper Combo"},{"Triple Hyper","Connect a three-character Hyper Combo"},{"Untouchable Trio","Win without damage"},{"Marvel's Finest","Win ten matches with all-Marvel teams"},{"Capcom All-Stars","Win ten matches with all-Capcom teams"},{"Into the Abyss","Defeat Abyss's three forms"},{"New Age Legend","Clear without a continue"}}
 };
 const char* tiers[]={"Bronze","Bronze","Bronze","Silver","Silver","Silver","Gold","Silver","Gold","Gold","Gold","Platinum"};
 for(size_t g=0;g<A.size();++g) for(size_t a=0;a<A[g].size();++a){ std::ostringstream id;id<<games_[g].id<<"_"<<std::setfill('0')<<std::setw(2)<<(a+1);achievements_.push_back({id.str(),games_[g].id,A[g][a].first,A[g][a].second,tiers[a]});}
 // The CHEAT EX games have 12 dedicated achievements each.
 const std::vector<std::vector<std::pair<std::string,std::string>>> ex={
  {{"Welcome to EX","Win your first match"},{"Breaking Defenses","Successfully land a Guard Break"},{"Supercharged!","Connect a Super Combo"},{"Cancel Master","Connect two Super Combos through a Super Cancel"},{"Perfect Fighter","Win a round without taking damage"},{"Five in a Row","Win five consecutive Arcade Mode matches"},{"Dark Power","Win a match playing as Evil Ryu"},{"Bloody Destiny","Win a match playing as Bloody Hokuto"},{"Cybernetic Fighter","Win a match using Cycloid-beta"},{"Wireframe Warrior","Win a match using Cycloid-gamma"},{"Garuda's Downfall","Defeat Garuda in Arcade Mode"},{"The EX Legend","Complete Arcade Mode without continuing"}},
  {{"EX Returns","Win your first match"},{"Break Their Guard","Successfully land a Guard Break"},{"Excel Initiated","Activate EXCEL mode"},{"Combo Technician","Connect five hits in one EXCEL sequence"},{"Meteor Strike","Connect a Meteor Combo"},{"Super Cancel Expert","Connect two Super Combos through a Super Cancel"},{"Unbreakable Spirit","Win a round without taking damage"},{"The Bonus Challenger","Complete an available bonus challenge"},{"Satellite Destroyer","Destroy the satellite in the bonus stage"},{"Shadow Conqueror","Defeat Shadow Geist in Arcade Mode"},{"Ultimate Bison","Defeat Bison II in Arcade Mode"},{"EX2 Grandmaster","Complete Arcade Mode without continuing"}}
 };
 for(size_t g=0;g<ex.size();++g)for(size_t a=0;a<ex[g].size();++a){
  std::ostringstream id;id<<games_[g+6].id<<"_"<<std::setfill('0')<<std::setw(2)<<(a+1);
  achievements_.push_back({id.str(),games_[g+6].id,ex[g][a].first,ex[g][a].second,tiers[a]});
 }
 std::vector<std::pair<std::string,std::string>> collection={
 {"Insert Coin","Insert a virtual credit"},{"Arcade Explorer","Launch all six main games"},{"First Challenger","Win any arcade match"},{"Friendly Rivalry","Finish a two-player match"},{"The Secret Code","Enter the master code"},{"Cheat Master","Activate a cheat in each main game"},{"Arcade Veteran","Win 100 matches"},{"500 Victories","Win 500 matches"},{"Six Arcade Legends","Clear all six main games"},{"No Continues Allowed","Clear all six without continues"},{"Achievement Hunter","Earn all 72 main-game achievements"},{"The Ultimate Collection","Earn every other achievement"}};
 for(size_t i=0;i<collection.size();++i){std::ostringstream id;id<<"collection_"<<std::setfill('0')<<std::setw(2)<<(i+1);achievements_.push_back({id.str(),"collection",collection[i].first,collection[i].second,tiers[i]});}
 // Gallery records are loaded solely from Gallery/arcade_gallery.mclgallery.
 load();
 // Game-only binary gallery; source package ships Gallery/arcade_gallery.mclgallery.
 // UserData root is conventionally next to the executable; fallback also supported.
 galleryDataLoaded_=loadGalleryPackage(root_.parent_path()/"Gallery"/"arcade_gallery.mclgallery");
 if(!galleryDataLoaded_)galleryDataLoaded_=loadGalleryPackage(root_/"gallery"/"arcade_gallery.mclgallery");
}
std::vector<Media> Collection::media(const std::string& game,const std::string& type) const{
 std::vector<Media> result; if(!validId(game)||(type!="theater"&&type!="music"&&type!="art")) return result;
 fs::path folder=root_/"media"/game/type; std::error_code ec;
 if(!fs::is_directory(folder,ec))return result;
 for(auto const& e:fs::directory_iterator(folder,fs::directory_options::skip_permission_denied,ec)){
  if(!e.is_regular_file(ec)) continue;
  std::string ext=e.path().extension().string();std::transform(ext.begin(),ext.end(),ext.begin(),[](unsigned char c){return char(std::tolower(c));});
  bool allowed=(type=="theater"&&(ext==".mp4"||ext==".avi"||ext==".wmv"))||(type=="music"&&(ext==".wav"||ext==".mp3"||ext==".wma"))||(type=="art"&&(ext==".png"||ext==".jpg"||ext==".bmp"));
  if(allowed) result.push_back({game,type,e.path().stem().string(),e.path()});
 }
 std::sort(result.begin(),result.end(),[](auto const&a,auto const&b){return a.title<b.title;});return result;
}
Stats Collection::stats(const std::string& game) const {auto it=stats_.find(game);return it==stats_.end()?Stats{}:it->second;}
AchievementProgress Collection::progress(const std::string& id) const {auto it=progress_.find(id);return it==progress_.end()?AchievementProgress{}:it->second;}
bool Collection::recordEvent(const std::string& game,const std::string& event,int amount){
 if(amount<1||amount>1000000)return false;
 if(std::none_of(games_.begin(),games_.end(),[&](auto const&g){return g.id==game;}))return false;
 auto &s=stats_[game];
 if(event=="win")s.wins+=amount;else if(event=="loss")s.losses+=amount;else if(event=="coin")s.credits+=amount;else if(event=="launch")s.launches+=amount;else if(event=="clear")s.clears+=amount;else if(event=="play_seconds")s.seconds+=amount;else return false;
 bool firstClear=(event=="clear" && galleryUnlocked() && (s.clears==amount));
 bool ok=save(); if(ok && firstClear)galleryNotice_=true; return ok;
}
bool Collection::unlockAchievement(const std::string& id){
 auto it=std::find_if(achievements_.begin(),achievements_.end(),[&](auto const&a){return a.id==id;});if(it==achievements_.end())return false;
 auto& p=progress_[id];if(p.unlocked)return false;p.unlocked=true;p.timestamp=utcNow();return save();
}
std::vector<std::string> Collection::consumeAchievementNotices(){
 auto result=std::move(achievementNotices_);achievementNotices_.clear();return result;
}
bool Collection::processVerifiedEvent(const VerifiedEvent& e){
 // The actual emulator must establish origin/authenticity. This method rejects
 // impossible/unknown event types and does not synthesize boss/combo events.
 if(e.count!=1)return false;
 if(std::none_of(games_.begin(),games_.end(),[&](const Game& g){return g.id==e.game;}))return false;
 if(e.type!="coin"&&e.type!="launch"&&e.type!="match_win"&&e.type!="match_loss"&&
    e.type!="arcade_clear"&&e.type!="two_player_match")return false;
 if((e.type=="match_win"||e.type=="match_loss"||e.type=="arcade_clear")&&
    (!e.arcadeMode||!e.humanControlled))return false; // reject attract-mode/demo results
 std::string raw;
 if(e.type=="coin")raw="coin";
 else if(e.type=="launch")raw="launch";
 else if(e.type=="match_win")raw="win";
 else if(e.type=="match_loss")raw="loss";
 else if(e.type=="arcade_clear")raw="clear";
 if(!raw.empty()&&!recordEvent(e.game,raw))return false;
 const auto award=[&](const std::string& id){
  if(unlockAchievement(id))achievementNotices_.push_back(id);
 };
 if(e.type=="coin")award("collection_01");
 if(e.type=="match_win"){
  award("collection_03");
  if(e.game=="xmcota"||e.game=="msh"||e.game=="xmvsf"||e.game=="mshvsf"||
     e.game=="mvsc"||e.game=="mvsc2"||e.game=="sfexp"||e.game=="sfex2p")award(e.game+"_01");
  if(e.game=="mvsc2"&&e.allCharactersAlive)award("mvsc2_02");
  if(e.game=="xmvsf"&&e.allCharactersAlive)award("xmvsf_09");
  int total=0;for(const auto& g:games_)total+=stats(g.id).wins;
  if(total>=100)award("collection_07");
  if(total>=500)award("collection_08");
 }
 if(e.type=="two_player_match")award("collection_04");
 if(e.type=="launch"){
  bool all=true;for(int i=0;i<6;++i)all=all&&stats(games_[size_t(i)].id).launches>0;
  if(all)award("collection_02");
 }
 if(e.type=="arcade_clear"){
  // A no-continue clear is never awarded if a cheat was active.
  if(!e.cheatsEnabled&&!e.continued){
   noContinueClears_[e.game]=1;
   if(e.game=="xmcota"||e.game=="msh"||e.game=="xmvsf"||e.game=="mvsc"||e.game=="mvsc2"||e.game=="sfexp"||e.game=="sfex2p")award(e.game+"_12");
  }
  bool all=true;for(int i=0;i<6;++i)all=all&&stats(games_[size_t(i)].id).clears>0;
  if(all)award("collection_09");
 }
 // Derived completion trophies can follow any award; no circular award.
 bool allMain=true;
 for(int i=0;i<6;++i)for(int j=1;j<=12;++j){std::ostringstream id;id<<games_[size_t(i)].id<<"_"<<std::setfill('0')<<std::setw(2)<<j;allMain=allMain&&progress(id.str()).unlocked;}
 if(allMain)award("collection_11");
 bool allOther=true;for(const auto& a:achievements_)if(a.id!="collection_12")allOther=allOther&&progress(a.id).unlocked;
 if(allOther)award("collection_12");
 return true;
}

bool Collection::saveState(const std::string& game,int slot,const std::vector<unsigned char>& bytes){
 if(!validId(game)||slot<1||slot>10||bytes.empty()||bytes.size()>128*1024*1024||std::none_of(games_.begin(),games_.end(),[&](auto const&g){return g.id==game;}))return false;
 std::error_code ec;fs::create_directories(root_/"states"/game,ec);if(ec)return false;
 auto path=root_/"states"/game/("slot"+std::to_string(slot)+".state");auto tmp=path;tmp+=".tmp";
 {std::ofstream out(tmp,std::ios::binary);if(!out)return false;out.write(reinterpret_cast<const char*>(bytes.data()),std::streamsize(bytes.size()));if(!out)return false;}
 fs::rename(tmp,path,ec);if(ec){fs::remove(path,ec);ec.clear();fs::rename(tmp,path,ec);}return !ec;
}
bool Collection::loadState(const std::string& game,int slot,std::vector<unsigned char>& bytes) const{
 if(!validId(game)||slot<1||slot>10)return false;
 auto p=root_/"states"/game/("slot"+std::to_string(slot)+".state");std::error_code ec;auto size=fs::file_size(p,ec);if(ec||size==0||size>128*1024*1024)return false;
 std::ifstream in(p,std::ios::binary);if(!in)return false;std::vector<unsigned char> temp(size_t(size),0);in.read(reinterpret_cast<char*>(temp.data()),std::streamsize(size));if(!in)return false;bytes=std::move(temp);return true;
}
bool Collection::save() const {
 std::error_code ec;fs::create_directories(root_,ec);if(ec)return false;
 auto p=root_/"progress.tsv";auto tmp=p;tmp+=".tmp";std::ofstream out(tmp,std::ios::binary);if(!out)return false;
 out<<"MCL-STATE-1\n";
 for(auto const& [id,s]:stats_)out<<"S\t"<<id<<"\t"<<s.wins<<"\t"<<s.losses<<"\t"<<s.credits<<"\t"<<s.launches<<"\t"<<s.clears<<"\t"<<s.seconds<<"\n";
 for(auto const& [id,p]:progress_)if(p.unlocked)out<<"A\t"<<id<<"\t"<<p.timestamp<<"\n";
 out.close();if(!out)return false;fs::rename(tmp,p,ec);if(ec){fs::remove(p,ec);ec.clear();fs::rename(tmp,p,ec);}return !ec;
}
bool Collection::load(){
 std::ifstream in(root_/"progress.tsv");if(!in)return false;std::string line;std::getline(in,line);if(line!="MCL-STATE-1")return false;
 while(std::getline(in,line)){
 auto f=split(line,'\t');try{
 if(f.size()==8&&f[0]=="S"&&validId(f[1])&&std::any_of(games_.begin(),games_.end(),[&](auto const&g){return g.id==f[1];})){
 Stats s; s.wins=std::stoi(f[2]);s.losses=std::stoi(f[3]);s.credits=std::stoi(f[4]);s.launches=std::stoi(f[5]);s.clears=std::stoi(f[6]);s.seconds=std::stoll(f[7]);if(s.wins>=0&&s.losses>=0&&s.credits>=0&&s.launches>=0&&s.clears>=0&&s.seconds>=0)stats_[f[1]]=s;
 }else if(f.size()==3&&f[0]=="A"&&std::any_of(achievements_.begin(),achievements_.end(),[&](auto const&a){return a.id==f[1];}))progress_[f[1]]={true,f[2]};
 }catch(...){continue;}
 }
 return true;
}

bool Collection::galleryUnlocked() const {
 for(const auto& g:games_){auto it=stats_.find(g.id); if(it!=stats_.end() && it->second.clears>0)return true;}
 return false;
}
bool Collection::consumeGalleryUnlockNotice(){bool n=galleryNotice_;galleryNotice_=false;return n;}

// MCLGAL01: 8-byte marker + little-endian record count; each record u16 type,
// u32 field-count, and bounded UTF-8 length-prefixed fields; last 32 bytes are
// SHA256 for integrity (verified at packaging, not cryptographically in this
// lightweight C++ reader). No standard ZIP, image or ROM asset is re-labeled.
bool Collection::loadGalleryPackage(const fs::path& path){
 std::ifstream in(path,std::ios::binary|std::ios::ate); if(!in)return false;
 auto end=in.tellg();if(end<std::streampos(44) || end>std::streampos(16*1024*1024))return false;
 std::vector<unsigned char> v(size_t(end),0);in.seekg(0);
 if(!in.read(reinterpret_cast<char*>(v.data()),std::streamsize(v.size())))return false;
 if(std::string(reinterpret_cast<const char*>(v.data()),8)!="MCLGAL01")return false;
 size_t pos=8;const size_t limit=v.size()-32;
 auto u16=[&](uint16_t& n){if(pos+2>limit)return false; n=uint16_t(v[pos])|(uint16_t(v[pos+1])<<8);pos+=2;return true;};
 auto u32=[&](uint32_t& n){if(pos+4>limit)return false;n=uint32_t(v[pos])|(uint32_t(v[pos+1])<<8)|(uint32_t(v[pos+2])<<16)|(uint32_t(v[pos+3])<<24);pos+=4;return true;};
 auto field=[&](std::string& x){uint32_t n=0;if(!u32(n)||n>256*1024||n>limit-pos)return false;x.assign(reinterpret_cast<const char*>(v.data()+pos),size_t(n));pos+=n;return true;};
 uint32_t count=0;if(!u32(count)||count>5000)return false;
 std::vector<GalleryGame> gd;std::vector<Character> cd;
 for(uint32_t i=0;i<count;i++){
  uint16_t typ=0;uint32_t nf=0;if(!u16(typ)||!u32(nf))return false;
  if((typ==1&&nf!=8)||(typ==2&&nf!=7))return false;
  if(typ==1){std::string id,title,year,hw,dev,hist,mechanics,sources;
   if(!field(id)||!field(title)||!field(year)||!field(hw)||!field(dev)||!field(hist)||!field(mechanics)||!field(sources)||!validId(id))return false;
   gd.push_back({id,title,year,hw,dev,hist,mechanics,sources});
  }else if(typ==2){std::string id,game,name,bio,role,sources,img;
   if(!field(id)||!field(game)||!field(name)||!field(bio)||!field(role)||!field(sources)||!field(img)||!validId(id)||!validId(game))return false;
   cd.push_back({game,name,bio,role,img});
  }else return false;
 }
 if(pos!=limit||gd.size()!=8||cd.empty())return false;
 // Do not accept data for unknown games.
 for(const auto& g:gd){if(std::none_of(games_.begin(),games_.end(),[&](const Game& x){return x.id==g.id;}))return false;}
 for(const auto& c:cd){if(std::none_of(gd.begin(),gd.end(),[&](const GalleryGame& x){return x.id==c.game;}))return false;}
 galleryGames_=std::move(gd); characters_=std::move(cd);return true;
}

}
