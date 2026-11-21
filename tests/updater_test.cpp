#include "updater.hpp"
#include <cassert>
#include <cstdio>
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
 if(argc==7&&std::string(argv[1])=="--script"){std::fputs(installerScript(argv[2],argv[3],std::stoul(argv[4]),std::stoll(argv[5]),argv[6]).c_str(),stdout);return 0;}
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
 // installer keeps the player's things
 const std::string s=installerScript("C:\\U\\files\\MCL","C:\\Games\\MCL",1234,11,"C:\\Games\\MCL\\Update");
 assert(s.find("PID eq 1234")!=std::string::npos);
 assert(s.find("\"C:\\U\\files\\MCL\\MCL_UserData\"")!=std::string::npos);
 assert(s.find("rom_directory.txt")!=std::string::npos);
 assert(s.find("installed_release.txt\" echo 11")!=std::string::npos);
 assert(s.find("if errorlevel 8 goto failed")!=std::string::npos);
 std::puts("updater_test: all passed");
 return 0;
}
