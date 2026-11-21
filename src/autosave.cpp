#include "autosave.hpp"
#include <algorithm>
#include <ctime>
#include <fstream>
#include <iterator>
#include <system_error>
#include <vector>
namespace mcl {
namespace fs=std::filesystem;
namespace {
const char* const kSaveFiles[]={"progress.tsv","settings.txt","marvel_cheats.txt","cheat_ex_settings.txt"};
std::string readAll(const fs::path& p){
 std::ifstream in(p,std::ios::binary);
 return std::string(std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>());
}
std::vector<fs::path> backupsNewestFirst(const fs::path& userDir){
 std::vector<fs::path> v;std::error_code ec;
 const fs::path root=userDir/"Backups";
 if(!fs::is_directory(root,ec))return v;
 for(const auto& e:fs::directory_iterator(root,ec))if(e.is_directory(ec))v.push_back(e.path());
 std::sort(v.begin(),v.end(),[](const fs::path& a,const fs::path& b){return a.filename().string()>b.filename().string();});
 return v;
}
}
bool progressLooksValid(const std::string& text){return text.rfind("MCL-STATE-1\n",0)==0||text.rfind("MCL-STATE-1\r\n",0)==0;}
std::string backupStamp(){
 const std::time_t t=std::time(nullptr);std::tm lt{};
#ifdef _WIN32
 localtime_s(&lt,&t);
#else
 localtime_r(&t,&lt);
#endif
 char b[32];std::strftime(b,sizeof b,"%Y%m%d-%H%M%S",&lt);return b;
}
int backupUserData(const fs::path& userDir,const std::string& stamp,int keep){
 std::error_code ec;
 // Only back up a progress file that is actually good, so a damaged save never
 // pushes the last good copy out of the rotation.
 if(fs::exists(userDir/"progress.tsv",ec)&&!progressLooksValid(readAll(userDir/"progress.tsv")))return 0;
 const fs::path dest=userDir/"Backups"/stamp;
 int copied=0;
 for(const char* f:kSaveFiles){
  const fs::path src=userDir/f;
  if(!fs::is_regular_file(src,ec))continue;
  fs::create_directories(dest,ec);
  if(fs::copy_file(src,dest/f,fs::copy_options::overwrite_existing,ec))++copied;
 }
 if(copied==0){fs::remove(dest,ec);return 0;}
 auto all=backupsNewestFirst(userDir);
 for(size_t i=static_cast<size_t>(std::max(keep,1));i<all.size();++i)fs::remove_all(all[i],ec);
 return copied;
}
bool restoreIfNeeded(const fs::path& userDir){
 std::error_code ec;
 const fs::path p=userDir/"progress.tsv";
 if(fs::is_regular_file(p,ec)&&progressLooksValid(readAll(p)))return false;
 for(const auto& b:backupsNewestFirst(userDir)){
  if(!fs::is_regular_file(b/"progress.tsv",ec)||!progressLooksValid(readAll(b/"progress.tsv")))continue;
  if(fs::exists(p,ec))fs::rename(p,userDir/"progress.damaged.tsv",ec); // keep it for inspection
  ec.clear();
  return fs::copy_file(b/"progress.tsv",p,fs::copy_options::overwrite_existing,ec);
 }
 return false;
}
std::string hiscoreDatWithAliases(const std::string& dat){
 struct Alias{const char* have;const char* add;};
 static const Alias aliases[]={{"xmvsfur1:","xmvsfur2:"}};
 std::string out=dat;bool changed=false;
 for(const auto& a:aliases){
  if(out.find(std::string("\n")+a.add)!=std::string::npos)continue;
  const auto at=out.find(std::string("\n")+a.have);
  if(at==std::string::npos)continue;
  out.insert(at+1,std::string(a.add)+"\n");changed=true;
 }
 return changed?out:std::string();
}
bool ensureHiscoreDat(const fs::path& mame){
 std::error_code ec;
 const fs::path dir=mame.parent_path(),plugin=dir/"plugins"/"hiscore"/"hiscore.dat",target=dir/"hiscore.dat";
 if(!fs::is_regular_file(plugin,ec))return false;
 const std::string current=readAll(fs::is_regular_file(target,ec)?target:plugin);
 const std::string patched=hiscoreDatWithAliases(current);
 if(patched.empty())return true; // already complete
 std::ofstream out(target,std::ios::binary|std::ios::trunc);out<<patched;
 return static_cast<bool>(out);
}
}
