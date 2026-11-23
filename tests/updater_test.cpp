#include "updater.hpp"
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
using namespace mcl;
int main(int argc,char** argv){
 if(argc==3&&std::string(argv[1])=="--check"){ // live check against GitHub (no download)
  Updater u(argv[2]);u.startCheck();
  while(u.state()==Updater::State::Checking)Sleep(100);
  const auto r=u.release();
  std::printf("state=%d tag=%s id=%lld asset=%s size=%llu build=%lld\n",static_cast<int>(u.state()),r.tag.c_str(),r.id,r.assetName.c_str(),r.assetSize,u.buildTimeUtc());
  std::wprintf(L"error=%ls\n",u.error().c_str());
  return 0;
 }
 // Shape of GitHub's /releases/latest answer (trimmed), with an uploader object that has its own "id".
 const std::string json=R"({"url":"x","id":265000123,"author":{"login":"someone","id":5},"tag_name":"BETA",
  "name":"Marvel: Crossover Legacy BETA","draft":false,"prerelease":false,"published_at":"2026-11-21T18:25:43Z",
  "assets":[{"id":1,"name":"notes.txt","size":10,"browser_download_url":"https://e/notes.txt"},
            {"id":2,"name":"Marvel_Crossover_Legacy.BETA.zip","size":1893732563,"browser_download_url":"https://github.com/o/r/releases/download/BETA/Marvel_Crossover_Legacy.BETA.zip"}],
  "body":"<img alt=\"HO\" src=\"https://x\" /> \u00e9 \ud83d\ude00"})";
 ReleaseInfo r;
 assert(parseLatestRelease(json,r));
 assert(r.id==265000123&&r.tag=="BETA"&&r.publishedAt=="2026-11-21T18:25:43Z");
 assert(r.assetName=="Marvel_Crossover_Legacy.BETA.zip"&&r.assetSize==1893732563ull);
 // an "update" zip wins over a bigger full package
 const std::string two=R"({"id":7,"tag_name":"v0.4.4","assets":[{"name":"Full.zip","size":900,"browser_download_url":"https://e/f.zip"},{"name":"MCL_Update_0.4.4.zip","size":50,"browser_download_url":"https://e/u.zip"}]})";
 assert(parseLatestRelease(two,r)&&r.assetName=="MCL_Update_0.4.4.zip");
 assert(!parseLatestRelease(R"({"id":7,"tag_name":"v1","assets":[]})",r)); // nothing to download
 assert(parseLatestRelease(R"({"id":8,"tag_name":"v0.4.3","assets":[{"name":"Marvel_Crossover_Legacy_v0.4.3.rar","size":1837544797,"browser_download_url":"https://e/x.rar"}]})",r)&&r.assetName=="Marvel_Crossover_Legacy_v0.4.3.rar");
 assert(!parseLatestRelease(R"({"id":8,"tag_name":"v1","assets":[{"name":"notes.txt","size":5,"browser_download_url":"https://e/n.txt"}]})",r));
 assert(!parseLatestRelease("not json",r));
 // versions
 assert(compareVersions("v0.4.4","0.4.3")==1&&compareVersions("0.4.3","v0.4.3")==0&&compareVersions("v0.5","0.10")==-1);
 assert(compareVersions("Release 1.0.0-beta2","0.9.9")==1);
 int n[4];assert(!versionNumbers("BETA",n));
 assert(parseIsoUtc("1970-01-02T00:00:00Z")==86400);
 assert(parseIsoUtc("2026-11-21T18:25:43Z")==1795285543LL);
 // newer?
 ReleaseInfo beta;beta.id=10;beta.tag="BETA";beta.publishedAt="2026-11-21T18:25:43Z";
 const long long pub=parseIsoUtc(beta.publishedAt);
 assert(!releaseIsNewer(beta,"0.4.3",pub+3600,0));   // built after it was published: we already have it or newer
 assert(releaseIsNewer(beta,"0.4.3",pub-3600,0));    // published after this build
 assert(!releaseIsNewer(beta,"0.4.3",pub-3600,10));  // ...but already installed
 ReleaseInfo v;v.id=11;v.tag="v0.4.4";v.publishedAt="2020-01-01T00:00:00Z";
 assert(releaseIsNewer(v,"0.4.3",pub,0));
 v.tag="v0.4.3";assert(!releaseIsNewer(v,"0.4.3",0,0));
 // the update keeps the player's things
 assert(keptOnUpdate("MCL_UserData/progress.tsv")&&keptOnUpdate("Logs/x.log")&&keptOnUpdate("rom_directory.txt"));
 assert(keptOnUpdate("Emulators/MAME 0.289/hiscore/xmcotau.hi")&&keptOnUpdate("Emulators/MAME 0.289/nvram/x/eeprom"));
 assert(keptOnUpdate("Emulators/Flycast 2.7/emu.cfg")&&keptOnUpdate("Emulators/Flycast 2.7/data/mvsc2.zip.eeprom"));
 assert(!keptOnUpdate("Emulators/MAME 0.289/plugins/hiscore/init.lua")&&!keptOnUpdate("MarvelCrossoverLegacy_Features.exe"));
 assert(!keptOnUpdate("ROMs/xmcotau.zip")&&!keptOnUpdate("Gallery/Art/a/full.png")&&!keptOnUpdate("Emulators/MAME 0.289/mame.exe"));
 // in-app install on a sandbox: replaces files (hidden ones too), keeps saves, hides everything, rolls back on failure
 {
  namespace fs=std::filesystem;
  const fs::path root=fs::temp_directory_path()/"mcl_install_test";
  std::error_code ec;
  for(fs::recursive_directory_iterator it(root,ec),end;!ec&&it!=end;it.increment(ec))SetFileAttributesW(it->path().c_str(),FILE_ATTRIBUTE_NORMAL);
  fs::remove_all(root,ec);
  const fs::path app=root/"app",src=root/"new";
  const auto put=[](const fs::path& f,const char* text){fs::create_directories(f.parent_path());std::ofstream(f,std::ios::trunc)<<text;};
  const auto get=[](const fs::path& f){std::ifstream in(f);std::string t;std::getline(in,t);return t;};
  put(app/"MarvelCrossoverLegacy_Features.exe","old");put(app/"README.txt","old");put(app/"Gallery/x.png","old");
  put(app/"earth_background.mp4","old");SetFileAttributesW((app/"earth_background.mp4").c_str(),FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM);
  put(app/"MCL_UserData/progress.tsv","MINE");put(app/"rom_directory.txt","MINE");put(app/"Emulators/MAME/hiscore/a.hi","MINE");
  const auto unpack=[&]{ // what a downloaded update holds (a failed attempt uses it up; the game downloads again)
   put(src/"MarvelCrossoverLegacy_Features.exe","new");put(src/"README.txt","new");put(src/"Gallery/x.png","new");put(src/"earth_background.mp4","new");
   put(src/"NewFolder/added.txt","new");put(src/"MCL_UserData/progress.tsv","THEIRS");put(src/"rom_directory.txt","THEIRS");put(src/"Emulators/MAME/hiscore/a.hi","THEIRS");
  };
  unpack();
  std::wstring err;double last=0;
  // 1) a file in use (opened without sharing) makes it fail, and everything is put back
  {
   HANDLE lock=CreateFileW((app/"Gallery/x.png").c_str(),GENERIC_READ,0,nullptr,OPEN_EXISTING,0,nullptr);
   assert(lock!=INVALID_HANDLE_VALUE);
   assert(!installUpdate(src,app,77,"v0.4.4",err,[&](double p){last=p;})&&!err.empty());
   CloseHandle(lock);
   assert(get(app/"MarvelCrossoverLegacy_Features.exe")=="old"&&get(app/"earth_background.mp4")=="old"&&get(app/"README.txt")=="old");
   assert(!fs::exists(app/"MCL_UserData/update_result.txt"));
  }
  // 2) success
  unpack();
  assert(installUpdate(src,app,77,"v0.4.4",err,[&](double p){last=p;}));
  assert(last==1.0);
  assert(get(app/"MarvelCrossoverLegacy_Features.exe")=="new"&&get(app/"Gallery/x.png")=="new"&&get(app/"earth_background.mp4")=="new"&&get(app/"NewFolder/added.txt")=="new");
  assert(get(app/"MCL_UserData/progress.tsv")=="MINE"&&get(app/"rom_directory.txt")=="MINE"&&get(app/"Emulators/MAME/hiscore/a.hi")=="MINE");
  assert(get(app/"MCL_UserData/installed_release.txt")=="77");
  const auto hidden=[](const fs::path& f){const DWORD a=GetFileAttributesW(f.c_str());return a!=INVALID_FILE_ATTRIBUTES&&(a&FILE_ATTRIBUTE_HIDDEN)&&(a&FILE_ATTRIBUTE_SYSTEM);};
  assert(hidden(app/"Gallery")&&hidden(app/"NewFolder")&&hidden(app/"earth_background.mp4")&&hidden(app/"MCL_UserData")&&hidden(app/"Emulators"));
  assert(!hidden(app/"MarvelCrossoverLegacy_Features.exe")&&!hidden(app/"README.txt"));
  UpdateResult res;assert(takeUpdateResult(app,res)&&res.ok&&res.version=="v0.4.4");
  assert(!takeUpdateResult(app,res)); // only once
  for(fs::recursive_directory_iterator it(root,ec),end;!ec&&it!=end;it.increment(ec))SetFileAttributesW(it->path().c_str(),FILE_ATTRIBUTE_NORMAL);
  fs::remove_all(root,ec);
 }
 // release notes -> plain text
 {
  const std::string md="<img width=\"1560\" alt=\"HO\" src=\"https://x\" />\r\n# Marvel: Crossover Legacy v0.4.4\r\n\r\n## New\r\n- **Automatic updates.** See [the page](https://e).\r\n- Use `X`.\r\n\r\n\r\nEnd";
  const std::string t=releaseNotesText(md);
  assert(t=="MARVEL: CROSSOVER LEGACY V0.4.4\n\nNEW\n\xE2\x80\xA2 Automatic updates. See the page.\n\xE2\x80\xA2 Use X.\n\nEnd");
  assert(releaseNotesText("a\nb\nc\nd",2)=="a\nb");
  assert(releaseNotesText("- one\n-\n- two")=="\xE2\x80\xA2 one\n\n\xE2\x80\xA2 two"); // empty bullets dropped
  ReleaseInfo rb;assert(parseLatestRelease(R"({"id":9,"tag_name":"v1","body":"Hi **there**","assets":[{"name":"g.zip","size":1,"browser_download_url":"https://e/g.zip"}]})",rb)&&rb.body=="Hi **there**");
 }
 std::puts("updater_test: all passed");
 return 0;
}
