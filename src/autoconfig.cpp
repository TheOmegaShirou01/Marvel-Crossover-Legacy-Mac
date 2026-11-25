#include "autoconfig.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include "external_roms.hpp"
#include "mame_backend.hpp"
#include "mame_sets.hpp"
#include <algorithm>
#include <cctype>
#include <cwctype>
#include <cstdlib>
#include <fstream>
#include <map>
#include <set>
#include <system_error>
namespace mcl {
namespace fs=std::filesystem;
namespace {
std::string lower(std::string s){for(auto& c:s)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));return s;}
// Wide-string version for file names: names with characters outside the ANSI code page
// (emoji, accented or Asian names) must never throw.
std::wstring wlower(std::wstring s){for(auto& c:s)c=static_cast<wchar_t>(std::towlower(c));return s;}
bool skipDir(const fs::path& p){
 const std::wstring n=wlower(p.filename().wstring());
 static const std::set<std::wstring> skip={L"windows",L"program files",L"program files (x86)",L"programdata",L"$recycle.bin",
  L"appdata",L"node_modules",L".git",L"system volume information",L"music",L"previews",L"obj",L"bin_obj",L"cache",L"snap"};
 return skip.count(n)>0||(!n.empty()&&n[0]==L'.');
}
template<class F> void walk(const fs::path& root,int depth,F&& visit){
 std::error_code ec;
 if(depth<0||!fs::is_directory(root,ec))return;
 visit(root);
 if(depth==0)return;
 fs::directory_iterator it(root,fs::directory_options::skip_permission_denied,ec),end;
 for(;!ec&&it!=end;it.increment(ec)){
  std::error_code e2;
  if(it->is_directory(e2)&&!it->is_symlink(e2)&&!skipDir(it->path()))walk(it->path(),depth-1,visit);
 }
}
// Paths inside the game folder are stored relative to it, so the folder can be extracted
// or moved anywhere; anything elsewhere on the PC is stored as found.
std::string storePath(const fs::path& appDir,const fs::path& p){
 std::error_code ec;
 const auto rel=fs::relative(p,appDir,ec);
 if(!ec&&!rel.empty()&&rel.u8string().rfind("..",0)!=0)return rel.u8string();
 return p.u8string();
}
std::string homeDir(){
#ifdef _WIN32
 char* v=nullptr;size_t n=0;std::string h;
 if(_dupenv_s(&v,&n,"USERPROFILE")==0&&v){h=v;free(v);}
 return h;
#else
 const char* v=std::getenv("HOME");return v?v:"";
#endif
}
// The setting files are hidden + system at runtime (main_win.cpp); Windows refuses to
// overwrite such files, so their attributes are cleared before writing.
void writable(const fs::path& p){
#ifdef _WIN32
 std::error_code ec;
 if(fs::exists(p,ec))SetFileAttributesW(p.c_str(),FILE_ATTRIBUTE_NORMAL);
#else
 (void)p;
#endif
}
std::vector<fs::path> defaultRoots(const fs::path& appDir){
 std::vector<fs::path> r{appDir,appDir.parent_path(),appDir.parent_path().parent_path()};
 const std::string home=homeDir();
 if(!home.empty())for(const char* sub:{"Desktop","Downloads","Documents","OneDrive/Desktop"})r.push_back(fs::path(home)/sub);
 return r;
}
}
const std::vector<std::string>& knownRomZips(){
 static const std::vector<std::string> v=[]{
  std::vector<std::string> z;
  for(const char* id:{"xmcota","msh","xmvsf","mshvsf","mvsc","mvsc2","sfexp","sfex2p"}){
   if(const auto* s=findMameSet(id)){
    z.push_back(std::string(s->set)+".zip");
    if(*s->parent)z.push_back(std::string(s->parent)+".zip");
    if(*s->bios)z.push_back(std::string(s->bios)+".zip");
   }
  }
  for(const char* extra:{"qsound.zip","qsound_hle.zip","naomi.zip","coh1000c.zip","coh3002c.zip"})z.push_back(extra);
  std::sort(z.begin(),z.end());z.erase(std::unique(z.begin(),z.end()),z.end());
  return z;
 }();
 return v;
}
std::vector<fs::path> findRomFolders(const std::vector<fs::path>& roots,int depth){
 std::map<std::wstring,std::pair<fs::path,int>> found; // canonical-ish key -> folder, hits
 std::set<std::wstring> known;for(const auto& z:knownRomZips())known.insert(wlower(fs::u8path(z).wstring()));
 for(const auto& root:roots){
  walk(root,depth,[&](const fs::path& dir){
   int hits=0;std::error_code ec;
   for(fs::directory_iterator it(dir,fs::directory_options::skip_permission_denied,ec),end;!ec&&it!=end;it.increment(ec)){
    std::error_code e2;
    if(it->is_regular_file(e2)&&known.count(wlower(it->path().filename().wstring())))++hits;
   }
   // key folders that hold ROM files plus key sub-folders (xmcotau\xmcotau.key style)
   if(hits>0){std::error_code e3;const auto key=wlower(fs::weakly_canonical(dir,e3).wstring());found[key]={dir,hits};}
  });
 }
 std::vector<std::pair<fs::path,int>> v;for(auto& kv:found)v.push_back(kv.second);
 std::stable_sort(v.begin(),v.end(),[](const auto& a,const auto& b){return a.second>b.second;});
 std::vector<fs::path> out;for(auto& p:v)out.push_back(p.first);
 return out;
}
int gamesFound(const std::vector<fs::path>& folders){
 RomSearchPath s;s.dirs=folders;int n=0;
 for(const char* id:{"xmcota","msh","xmvsf","mshvsf","mvsc","mvsc2","sfexp","sfex2p"}){
  const auto* set=findMameSet(id);if(set&&s.hasSet(set->set))++n;
 }
 return n;
}
fs::path findExecutable(const std::vector<fs::path>& roots,const std::wstring& exeName,int depth){
 fs::path hit;
 for(const auto& root:roots){
  walk(root,depth,[&](const fs::path& dir){
   if(!hit.empty())return;
   std::error_code ec;const auto p=dir/exeName;
   if(fs::is_regular_file(p,ec))hit=p;
  });
  if(!hit.empty())break;
 }
 return hit;
}
AutoConfigResult autoConfigure(const fs::path& appDir,const std::vector<fs::path>& searchRoots){
 AutoConfigResult r;std::error_code ec;
 const auto roots=searchRoots.empty()?defaultRoots(appDir):searchRoots;
 // Emulators first: their folders are also good places to look for ROMs.
 fs::path mame=readMameExecutable(appDir/"mame_executable.txt");
 if(mame.empty()||!fs::is_regular_file(mame,ec)){
  const auto found=findExecutable(roots,L"mame.exe",4);
  if(!found.empty()){(writable(appDir/"mame_executable.txt"),std::ofstream(appDir/"mame_executable.txt",std::ios::trunc))<<storePath(appDir,found)<<"\n";mame=found;r.mameChanged=true;}
 }
 fs::path fly=readMameExecutable(appDir/"flycast_executable.txt");
 if(fly.empty()||!fs::is_regular_file(fly,ec)){
  const auto found=findExecutable(roots,L"flycast.exe",4);
  if(!found.empty()){(writable(appDir/"flycast_executable.txt"),std::ofstream(appDir/"flycast_executable.txt",std::ios::trunc))<<storePath(appDir,found)<<"\n";r.flycastChanged=true;}
 }
 // ROMs: keep the configured folders when every game is already there.
 auto current=readRomSearchPath(appDir/"rom_directory.txt").dirs;
 current.erase(std::remove_if(current.begin(),current.end(),[](const fs::path& p){std::error_code e;return !fs::is_directory(p,e);}),current.end());
 const int have=gamesFound(current);
 r.games=have;
 if(have==8)return r;
 // ROMs shipped inside the game folder come first; when they cover every game the rest
 // of the PC is not searched at all.
 auto found=findRomFolders({appDir},4);
 if(gamesFound(found)<8){
  auto romRoots=roots;
  if(!mame.empty())romRoots.insert(romRoots.begin(),mame.parent_path()/"roms");
  for(const auto& f:findRomFolders(romRoots,4))found.push_back(f);
 }
 // existing working folders first, then new ones that add something
 std::vector<fs::path> merged=current;
 for(const auto& f:found){
  bool dup=false;for(const auto& m:merged){std::error_code e;if(fs::equivalent(m,f,e))dup=true;}
  if(!dup)merged.push_back(f);
 }
 const int now=gamesFound(merged);
 if(now>have){
  writable(appDir/"rom_directory.txt");
  std::ofstream out(appDir/"rom_directory.txt",std::ios::trunc);
  out<<"# Found automatically on this PC (Marvel: Crossover Legacy first-run setup)\n";
  for(const auto& f:merged)out<<storePath(appDir,f)<<"\n";
  r.romsChanged=true;
 }
 r.games=std::max(now,have);
 return r;
}
}
