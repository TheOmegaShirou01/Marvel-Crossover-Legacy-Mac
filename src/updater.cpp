#include "updater.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <sstream>
#include <vector>
#ifdef _WIN32
#include <winhttp.h>
#endif
namespace mcl {
namespace fs=std::filesystem;
// ---------- a small JSON reader (enough for GitHub's release objects) ----------
namespace {
struct JVal {
 enum T {Null,Bool,Num,Str,Arr,Obj} t=Null;
 double num=0;bool b=false;std::string s;
 std::vector<JVal> arr;std::map<std::string,JVal> obj;
 const JVal* get(const std::string& k) const {auto it=obj.find(k);return it==obj.end()?nullptr:&it->second;}
};
struct JParser {
 const std::string& s;size_t i=0;int depth=0;
 explicit JParser(const std::string& text):s(text){}
 void ws(){while(i<s.size()&&std::isspace(static_cast<unsigned char>(s[i])))++i;}
 bool str(std::string& out){
  if(i>=s.size()||s[i]!='"')return false;++i;
  while(i<s.size()&&s[i]!='"'){
   char c=s[i++];
   if(c=='\\'&&i<s.size()){
    const char e=s[i++];
    switch(e){
     case 'n':out+='\n';break;case 't':out+='\t';break;case 'r':out+='\r';break;
     case 'b':out+='\b';break;case 'f':out+='\f';break;
     case 'u':{
      if(i+4>s.size())return false;
      unsigned cp=static_cast<unsigned>(std::strtoul(s.substr(i,4).c_str(),nullptr,16));i+=4;
      if(cp>=0xD800&&cp<0xDC00&&i+6<=s.size()&&s[i]=='\\'&&s[i+1]=='u'){
       const unsigned lo=static_cast<unsigned>(std::strtoul(s.substr(i+2,4).c_str(),nullptr,16));
       if(lo>=0xDC00&&lo<0xE000){cp=0x10000+((cp-0xD800)<<10)+(lo-0xDC00);i+=6;}
      }
      if(cp<0x80)out+=static_cast<char>(cp);
      else if(cp<0x800){out+=static_cast<char>(0xC0|(cp>>6));out+=static_cast<char>(0x80|(cp&0x3F));}
      else if(cp<0x10000){out+=static_cast<char>(0xE0|(cp>>12));out+=static_cast<char>(0x80|((cp>>6)&0x3F));out+=static_cast<char>(0x80|(cp&0x3F));}
      else{out+=static_cast<char>(0xF0|(cp>>18));out+=static_cast<char>(0x80|((cp>>12)&0x3F));out+=static_cast<char>(0x80|((cp>>6)&0x3F));out+=static_cast<char>(0x80|(cp&0x3F));}
      break;}
     default:out+=e;
    }
   }else out+=c;
  }
  if(i>=s.size())return false;++i;return true;
 }
 bool val(JVal& v){
  ws();if(i>=s.size()||++depth>64)return false;
  bool ok=true;const char c=s[i];
  if(c=='{'){
   v.t=JVal::Obj;++i;ws();
   if(i<s.size()&&s[i]=='}'){++i;}
   else for(;;){
    ws();std::string k;if(!str(k)){ok=false;break;}
    ws();if(i>=s.size()||s[i]!=':'){ok=false;break;}++i;
    JVal child;if(!val(child)){ok=false;break;}
    v.obj[k]=std::move(child);
    ws();if(i<s.size()&&s[i]==','){++i;continue;}
    if(i<s.size()&&s[i]=='}'){++i;break;}
    ok=false;break;
   }
  }else if(c=='['){
   v.t=JVal::Arr;++i;ws();
   if(i<s.size()&&s[i]==']'){++i;}
   else for(;;){
    JVal child;if(!val(child)){ok=false;break;}
    v.arr.push_back(std::move(child));
    ws();if(i<s.size()&&s[i]==','){++i;continue;}
    if(i<s.size()&&s[i]==']'){++i;break;}
    ok=false;break;
   }
  }else if(c=='"'){v.t=JVal::Str;ok=str(v.s);}
  else if(s.compare(i,4,"true")==0){v.t=JVal::Bool;v.b=true;i+=4;}
  else if(s.compare(i,5,"false")==0){v.t=JVal::Bool;i+=5;}
  else if(s.compare(i,4,"null")==0){i+=4;}
  else{
   const size_t st=i;
   while(i<s.size()&&(std::isdigit(static_cast<unsigned char>(s[i]))||s[i]=='-'||s[i]=='+'||s[i]=='.'||s[i]=='e'||s[i]=='E'))++i;
   if(i==st)ok=false;else{v.t=JVal::Num;v.num=std::strtod(s.substr(st,i-st).c_str(),nullptr);}
  }
  --depth;return ok;
 }
};
std::string lower(std::string s){for(auto& c:s)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));return s;}
bool endsWith(const std::string& s,const std::string& e){return s.size()>=e.size()&&s.compare(s.size()-e.size(),e.size(),e)==0;}
}
bool parseLatestRelease(const std::string& json,ReleaseInfo& out){
 JVal root;JParser p(json);
 if(!p.val(root)||root.t!=JVal::Obj)return false;
 const auto str=[&](const JVal& o,const char* k){const JVal* v=o.get(k);return v&&v->t==JVal::Str?v->s:std::string();};
 const JVal* id=root.get("id");if(!id||id->t!=JVal::Num)return false;
 if(const JVal* d=root.get("draft");d&&d->b)return false;
 ReleaseInfo r;r.id=static_cast<long long>(id->num);r.tag=str(root,"tag_name");r.name=str(root,"name");r.publishedAt=str(root,"published_at");
 const JVal* assets=root.get("assets");
 if(!assets||assets->t!=JVal::Arr)return false;
 int best=-1;bool bestUpdate=false;double bestSize=-1;
 for(size_t k=0;k<assets->arr.size();++k){
  const JVal& a=assets->arr[k];
  const std::string n=lower(str(a,"name"));
  if(!endsWith(n,".zip")||str(a,"browser_download_url").empty())continue;
  const bool upd=n.find("update")!=std::string::npos;
  const JVal* sz=a.get("size");const double size=sz&&sz->t==JVal::Num?sz->num:0;
  if(best<0||(upd&&!bestUpdate)||(upd==bestUpdate&&size>bestSize)){best=static_cast<int>(k);bestUpdate=upd;bestSize=size;}
 }
 if(best<0)return false;
 const JVal& a=assets->arr[static_cast<size_t>(best)];
 r.assetName=str(a,"name");r.assetUrl=str(a,"browser_download_url");r.assetSize=static_cast<unsigned long long>(bestSize);
 out=r;return true;
}
bool versionNumbers(const std::string& text,int out[4]){
 int n=0;size_t i=0;
 for(int k=0;k<4;++k)out[k]=0;
 while(i<text.size()&&n<4){
  if(std::isdigit(static_cast<unsigned char>(text[i]))){
   int v=0;while(i<text.size()&&std::isdigit(static_cast<unsigned char>(text[i]))){v=std::min(v*10+(text[i]-'0'),99999999);++i;}
   out[n++]=v;
   if(i<text.size()&&text[i]=='.')++i;else break; // only the first dotted run ("v1.2.3-beta2" -> 1.2.3)
  }else ++i;
 }
 return n>0;
}
int compareVersions(const std::string& a,const std::string& b){
 int x[4],y[4];versionNumbers(a,x);versionNumbers(b,y);
 for(int k=0;k<4;++k){if(x[k]!=y[k])return x[k]<y[k]?-1:1;}
 return 0;
}
long long parseIsoUtc(const std::string& iso){
 int Y=0,M=0,D=0,h=0,m=0,s=0;
 if(sscanf_s(iso.c_str(),"%d-%d-%dT%d:%d:%d",&Y,&M,&D,&h,&m,&s)<3||Y<1970||M<1||M>12||D<1||D>31)return 0;
 // days from civil (Howard Hinnant)
 const int y=Y-(M<=2);const int era=(y>=0?y:y-399)/400;const unsigned yoe=static_cast<unsigned>(y-era*400);
 const unsigned doy=static_cast<unsigned>((153*(M+(M>2?-3:9))+2)/5+D-1);const unsigned doe=yoe*365+yoe/4-yoe/100+doy;
 const long long days=static_cast<long long>(era)*146097+static_cast<long long>(doe)-719468;
 return days*86400+h*3600+m*60+s;
}
bool releaseIsNewer(const ReleaseInfo& r,const std::string& appVersion,long long buildUtc,long long installedId){
 if(r.id==0||r.id==installedId)return false;
 int v[4];
 if(versionNumbers(r.tag,v))return compareVersions(r.tag,appVersion)>0;
 const long long pub=parseIsoUtc(r.publishedAt);
 return pub>0&&buildUtc>0&&pub>buildUtc;
}
std::string installerScript(const fs::path& source,const fs::path& app,unsigned long pid,long long releaseId,const fs::path& cleanup){
 const auto q=[](const fs::path& p){return "\""+p.u8string()+"\"";};
 std::ostringstream o;
 o<<"@echo off\r\nchcp 65001 >nul\r\n"
    "rem Marvel: Crossover Legacy updater (written by the game)\r\n"
    "title Marvel: Crossover Legacy - installing update\r\n"
    "echo Installing the Marvel: Crossover Legacy update, please wait...\r\n"
    ":wait\r\n"
    "tasklist /FI \"PID eq "<<pid<<"\" 2>nul | find \""<<pid<<"\" >nul && (ping -n 2 127.0.0.1 >nul & goto wait)\r\n"
    // the game hides its content; unhide the top level so files can be replaced (it hides them again on start)
    "attrib -h -s "<<q(app/"*")<<" /d >nul 2>&1\r\n";
 // Kept from the current install: progress/settings/backups, logs, ROM and emulator paths,
 // MAME's per-game settings, saves and high scores, Flycast's settings, saves and mappings.
 std::vector<fs::path> keepDirs={source/"MCL_UserData",source/"Logs"};
 std::vector<fs::path> keepFiles={"rom_directory.txt","mame_executable.txt","flycast_executable.txt"};
 std::error_code ec;
 for(fs::directory_iterator it(source/"Emulators",ec),end;!ec&&it!=end;it.increment(ec)){
  if(!it->is_directory(ec))continue;
  for(const char* d:{"cfg","nvram","hiscore","diff","memcard","data","mappings"}){
   // MAME's own plugins\hiscore is program code, not scores: only top-level folders are kept.
   keepDirs.push_back(it->path()/d);
  }
  keepFiles.push_back(it->path()/"emu.cfg");
 }
 o<<"robocopy "<<q(source)<<" "<<q(app)<<" /E /R:2 /W:1 /NFL /NDL /NJH /NJS /NP /XD";
 for(const auto& d:keepDirs)o<<" "<<q(d);
 o<<" /XF";
 for(const auto& f:keepFiles)o<<" "<<q(f);
 o<<"\r\nif errorlevel 8 goto failed\r\n"
    "if not exist "<<q(app/"MCL_UserData")<<" mkdir "<<q(app/"MCL_UserData")<<"\r\n"
    ">"<<q(app/"MCL_UserData"/"installed_release.txt")<<" echo "<<releaseId<<"\r\n"
    "rmdir /s /q "<<q(cleanup)<<" >nul 2>&1\r\n"
    "start \"\" "<<q(app/"MarvelCrossoverLegacy_Features.exe")<<"\r\n"
    "exit /b 0\r\n"
    ":failed\r\n"
    "echo The update could not be copied. Your game was not changed.\r\n"
    "ping -n 9 127.0.0.1 >nul\r\n"
    "start \"\" "<<q(app/"MarvelCrossoverLegacy_Features.exe")<<"\r\n"
    "exit /b 1\r\n";
 return o.str();
}

#ifdef _WIN32
namespace {
std::wstring wideUtf8(const std::string& s){
 if(s.empty())return {};
 const int n=MultiByteToWideChar(CP_UTF8,0,s.data(),static_cast<int>(s.size()),nullptr,0);
 std::wstring w(static_cast<size_t>(n),L'\0');MultiByteToWideChar(CP_UTF8,0,s.data(),static_cast<int>(s.size()),w.data(),n);return w;
}
struct Inet {HINTERNET h=nullptr;~Inet(){if(h)WinHttpCloseHandle(h);} };
// GET a URL; body goes to `sink` chunk by chunk. Follows GitHub's redirect to its file host.
bool httpGet(const std::wstring& url,const std::function<bool(const char*,DWORD)>& sink,unsigned long long& total,std::wstring& err){
 URL_COMPONENTS uc{};uc.dwStructSize=sizeof uc;wchar_t host[256]{},path[4096]{};
 uc.lpszHostName=host;uc.dwHostNameLength=255;uc.lpszUrlPath=path;uc.dwUrlPathLength=4095;
 if(!WinHttpCrackUrl(url.c_str(),0,0,&uc)){err=L"bad address";return false;}
 Inet s,c,r;
 s.h=WinHttpOpen(L"MarvelCrossoverLegacy-Updater/1.0",WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);
 if(!s.h)s.h=WinHttpOpen(L"MarvelCrossoverLegacy-Updater/1.0",WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);
 if(!s.h){err=L"no internet access";return false;}
 WinHttpSetTimeouts(s.h,15000,15000,30000,60000);
 c.h=WinHttpConnect(s.h,host,uc.nPort,0);if(!c.h){err=L"could not reach the server";return false;}
 r.h=WinHttpOpenRequest(c.h,L"GET",path,nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,uc.nScheme==INTERNET_SCHEME_HTTPS?WINHTTP_FLAG_SECURE:0);
 if(!r.h){err=L"could not reach the server";return false;}
 const wchar_t* hdr=L"Accept: application/vnd.github+json, application/octet-stream\r\n";
 if(!WinHttpSendRequest(r.h,hdr,static_cast<DWORD>(-1L),WINHTTP_NO_REQUEST_DATA,0,0,0)||!WinHttpReceiveResponse(r.h,nullptr)){err=L"no internet connection";return false;}
 DWORD status=0,len=sizeof status;
 WinHttpQueryHeaders(r.h,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&status,&len,WINHTTP_NO_HEADER_INDEX);
 if(status==404){err=L"no release found";return false;}
 if(status!=200){err=L"the server answered "+std::to_wstring(status);return false;}
 wchar_t cl[32]{};len=sizeof cl;total=0;
 if(WinHttpQueryHeaders(r.h,WINHTTP_QUERY_CONTENT_LENGTH,WINHTTP_HEADER_NAME_BY_INDEX,cl,&len,WINHTTP_NO_HEADER_INDEX))total=std::wcstoull(cl,nullptr,10);
 std::vector<char> buf(1<<20);
 for(;;){
  DWORD got=0;
  if(!WinHttpReadData(r.h,buf.data(),static_cast<DWORD>(buf.size()),&got)){err=L"the download was interrupted";return false;}
  if(got==0)break;
  if(!sink(buf.data(),got)){err=L"cancelled";return false;}
 }
 return true;
}
bool runHidden(const std::wstring& cmd,const fs::path& cwd,DWORD timeoutMs,DWORD& exitCode){
 STARTUPINFOW si{};si.cb=sizeof si;PROCESS_INFORMATION pi{};
 std::wstring line=cmd;
 if(!CreateProcessW(nullptr,line.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,cwd.c_str(),&si,&pi))return false;
 const DWORD w=WaitForSingleObject(pi.hProcess,timeoutMs);
 if(w!=WAIT_OBJECT_0)TerminateProcess(pi.hProcess,1);
 GetExitCodeProcess(pi.hProcess,&exitCode);CloseHandle(pi.hThread);CloseHandle(pi.hProcess);
 return w==WAIT_OBJECT_0;
}
// The folder in the unzipped release that holds the game's exe (the zip may have a top folder).
fs::path findGameRoot(const fs::path& dir){
 std::error_code ec;
 if(fs::is_regular_file(dir/"MarvelCrossoverLegacy_Features.exe",ec))return dir;
 for(fs::recursive_directory_iterator it(dir,ec),end;!ec&&it!=end;it.increment(ec)){
  if(it.depth()>3){it.disable_recursion_pending();continue;}
  if(it->path().filename()==L"MarvelCrossoverLegacy_Features.exe")return it->path().parent_path();
 }
 return {};
}
long long readInstalledId(const fs::path& file){std::ifstream in(file);long long v=0;in>>v;return v;}
}
Updater::~Updater(){cancel_=true;join();}
void Updater::join(){if(worker_.joinable())worker_.join();}
void Updater::fail(const std::wstring& why){{std::lock_guard<std::mutex> l(m_);error_=why;}state_=State::Failed;}
long long Updater::buildTimeUtc() const{
 const auto* base=reinterpret_cast<const unsigned char*>(GetModuleHandleW(nullptr));
 const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
 if(!dos||dos->e_magic!=IMAGE_DOS_SIGNATURE)return 0;
 const auto* nt=reinterpret_cast<const IMAGE_NT_HEADERS*>(base+dos->e_lfanew);
 return nt->Signature==IMAGE_NT_SIGNATURE?static_cast<long long>(nt->FileHeader.TimeDateStamp):0;
}
void Updater::startCheck(){
 const State s=state_;
 if(s==State::Checking||s==State::Downloading||s==State::Extracting)return;
 join();cancel_=false;state_=State::Checking;
 worker_=std::thread([this]{
  std::string body;unsigned long long total=0;std::wstring err;
  const bool ok=httpGet(std::wstring(L"https://")+kUpdateHost+kUpdatePath,[&](const char* d,DWORD n){body.append(d,n);return body.size()<(4u<<20);},total,err);
  if(!ok){fail(err==L"no release found"?L"no release has been published yet":err);return;}
  ReleaseInfo r;
  if(!parseLatestRelease(body,r)){fail(L"the latest release has no .zip to download");return;}
  {std::lock_guard<std::mutex> l(m_);rel_=r;}
  const long long installed=readInstalledId(dir_/"MCL_UserData"/"installed_release.txt");
  bool newer=releaseIsNewer(r,kAppVersion,buildTimeUtc(),installed);
  wchar_t force[8]{};if(GetEnvironmentVariableW(L"MCL_UPDATE_FORCE",force,8)>0&&force[0]==L'1')newer=true; // testing only
  state_=newer?State::Available:State::UpToDate;
 });
}
void Updater::startDownload(){
 if(state_!=State::Available)return;
 join();cancel_=false;progress_=0;state_=State::Downloading;
 worker_=std::thread([this]{
  const ReleaseInfo r=release();
  const fs::path work=dir_/"Update";
  std::error_code ec;fs::remove_all(work,ec);fs::create_directories(work,ec);
  SetFileAttributesW(work.c_str(),FILE_ATTRIBUTE_HIDDEN);
  // room for the zip and its unpacked copy
  ULARGE_INTEGER freeBytes{};
  if(GetDiskFreeSpaceExW(work.c_str(),&freeBytes,nullptr,nullptr)&&r.assetSize>0&&freeBytes.QuadPart<r.assetSize*21/10+(200ull<<20)){
   const double need=static_cast<double>(r.assetSize*21/10)/(1<<30);
   wchar_t msg[160];swprintf(msg,160,L"not enough free disk space (about %.1f GB is needed)",need);
   fs::remove_all(work,ec);fail(msg);return;
  }
  const fs::path zip=work/"update.zip";
  {
   std::ofstream out(zip,std::ios::binary|std::ios::trunc);
   if(!out){fail(L"could not write the download");return;}
   unsigned long long done=0,total=r.assetSize;std::wstring err;
   const bool ok=httpGet(wideUtf8(r.assetUrl),[&](const char* d,DWORD n){
     out.write(d,n);done+=n;
     if(total)progress_=std::min(1.0,static_cast<double>(done)/static_cast<double>(total));
     return !cancel_&&static_cast<bool>(out);},total,err);
   out.close();
   if(!ok||(r.assetSize&&done!=r.assetSize)){fs::remove_all(work,ec);if(cancel_){state_=State::Available;return;}fail(ok?L"the download was incomplete":err);return;}
  }
  state_=State::Extracting;
  const fs::path unpacked=work/"files";fs::create_directories(unpacked,ec);
  DWORD code=1;
  // Windows 10/11 include tar (bsdtar), which unpacks .zip files.
  const std::wstring cmd=L"tar.exe -xf \""+zip.wstring()+L"\" -C \""+unpacked.wstring()+L"\"";
  if(!runHidden(cmd,work,30*60*1000,code)||code!=0){fs::remove_all(work,ec);fail(L"the update could not be unpacked");return;}
  fs::remove(zip,ec);
  const fs::path root=findGameRoot(unpacked);
  if(root.empty()){fs::remove_all(work,ec);fail(L"the download does not contain the game");return;}
  source_=root;progress_=1;state_=State::Ready;
 });
}
void Updater::cancel(){cancel_=true;}
bool Updater::launchInstaller(){
 if(state_!=State::Ready)return false;
 wchar_t tmp[MAX_PATH+1]{};GetTempPathW(MAX_PATH,tmp);
 const fs::path script=fs::path(tmp)/L"mcl_update.cmd";
 {std::ofstream out(script,std::ios::binary|std::ios::trunc);
  if(!out)return false;
  out<<installerScript(source_,dir_,GetCurrentProcessId(),release().id,dir_/"Update");}
 std::wstring cmd=L"cmd.exe /c \""+script.wstring()+L"\"";
 STARTUPINFOW si{};si.cb=sizeof si;si.dwFlags=STARTF_USESHOWWINDOW;si.wShowWindow=SW_SHOWMINNOACTIVE;PROCESS_INFORMATION pi{};
 if(!CreateProcessW(nullptr,cmd.data(),nullptr,nullptr,FALSE,CREATE_NEW_CONSOLE,nullptr,dir_.c_str(),&si,&pi))return false;
 CloseHandle(pi.hThread);CloseHandle(pi.hProcess);
 return true;
}
#endif
}
