#include "updater.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
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
  if(!(endsWith(n,".zip")||endsWith(n,".rar")||endsWith(n,".7z"))||str(a,"browser_download_url").empty())continue; // Windows' tar unpacks all three
  const bool upd=n.find("update")!=std::string::npos;
  const JVal* sz=a.get("size");const double size=sz&&sz->t==JVal::Num?sz->num:0;
  if(best<0||(upd&&!bestUpdate)||(upd==bestUpdate&&size>bestSize)){best=static_cast<int>(k);bestUpdate=upd;bestSize=size;}
 }
 if(best<0)return false;
 const JVal& a=assets->arr[static_cast<size_t>(best)];
 r.body=str(root,"body");
 r.assetName=str(a,"name");r.assetUrl=str(a,"browser_download_url");r.assetSize=static_cast<unsigned long long>(bestSize);
 out=r;return true;
}
std::string releaseNotesText(const std::string& md,size_t maxLines){
 // GitHub release notes are Markdown (often with an <img> banner): keep the words only.
 std::string out;size_t lines=0;
 std::string text=md;
 for(size_t lt;(lt=text.find('<'))!=std::string::npos;){ // drop HTML tags
  const size_t gt=text.find('>',lt);if(gt==std::string::npos)break;
  text.erase(lt,gt-lt+1);
 }
 std::string line;std::istringstream in(text);bool blank=true;
 while(std::getline(in,line)&&lines<maxLines){
  if(!line.empty()&&line.back()=='\r')line.pop_back();
  size_t i=0;while(i<line.size()&&(line[i]==' '||line[i]=='\t'))++i;
  std::string l=line.substr(i);
  bool heading=false;
  while(!l.empty()&&l[0]=='#'){l.erase(0,1);heading=true;}
  if(l.size()>=2&&(l[0]=='-'||l[0]=='*'||l[0]=='+')&&l[1]==' ')l="\xE2\x80\xA2 "+l.substr(2); // bullet
  for(const char* mark:{"**","__","`"})for(size_t at;(at=l.find(mark))!=std::string::npos;)l.erase(at,std::strlen(mark));
  for(size_t at;(at=l.find("]("))!=std::string::npos;){ // [text](url) -> text
   const size_t open=l.rfind('[',at),close=l.find(')',at);
   if(open==std::string::npos||close==std::string::npos)break;
   l=l.substr(0,open)+l.substr(open+1,at-open-1)+l.substr(close+1);
  }
  while(!l.empty()&&l[0]==' ')l.erase(0,1);
  while(!l.empty()&&l.back()==' ')l.pop_back();
  if(heading)for(auto& c:l)c=static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  if(l=="\xE2\x80\xA2"||l=="-"||l=="*"||l=="+")l.clear(); // a bullet with nothing after it
  if(l.empty()){if(!blank&&!out.empty()){out+="\n";++lines;}blank=true;continue;}
  out+=l+"\n";++lines;blank=false;
 }
 while(!out.empty()&&out.back()=='\n')out.pop_back();
 return out;
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
bool keptOnUpdate(const fs::path& relative){
 std::vector<std::wstring> parts;
 for(const auto& p:relative)parts.push_back(p.wstring());
 if(parts.empty())return true;
 const auto is=[](const std::wstring& a,const wchar_t* b){return _wcsicmp(a.c_str(),b)==0;};
 if(is(parts[0],L"MCL_UserData")||is(parts[0],L"Logs")||is(parts[0],L"Update"))return true;
 if(parts.size()==1)
  return is(parts[0],L"rom_directory.txt")||is(parts[0],L"mame_executable.txt")||is(parts[0],L"flycast_executable.txt");
 if(is(parts[0],L"Emulators")&&parts.size()>=3){
  // Emulators\<emulator>\<top-level folder or file>. (MAME's plugins\hiscore is program code, so
  // only folders directly inside the emulator folder count.)
  if(parts.size()==3&&is(parts[2],L"emu.cfg"))return true;
  if(parts.size()>=4)for(const wchar_t* d:{L"cfg",L"nvram",L"hiscore",L"diff",L"memcard",L"data",L"mappings"})if(is(parts[2],d))return true;
 }
 return false;
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
 if(!s.h){err=L"NETWORK ERROR: no internet connection";return false;}
 WinHttpSetTimeouts(s.h,15000,15000,30000,60000);
 c.h=WinHttpConnect(s.h,host,uc.nPort,0);if(!c.h){err=L"NETWORK ERROR: the update server could not be reached";return false;}
 r.h=WinHttpOpenRequest(c.h,L"GET",path,nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,uc.nScheme==INTERNET_SCHEME_HTTPS?WINHTTP_FLAG_SECURE:0);
 if(!r.h){err=L"NETWORK ERROR: the update server could not be reached";return false;}
 const wchar_t* hdr=L"Accept: application/vnd.github+json, application/octet-stream\r\n";
 if(!WinHttpSendRequest(r.h,hdr,static_cast<DWORD>(-1L),WINHTTP_NO_REQUEST_DATA,0,0,0)||!WinHttpReceiveResponse(r.h,nullptr)){err=L"NETWORK ERROR: no internet connection";return false;}
 DWORD status=0,len=sizeof status;
 WinHttpQueryHeaders(r.h,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&status,&len,WINHTTP_NO_HEADER_INDEX);
 if(status==404){err=L"no release found";return false;}
 if(status!=200){err=L"the server answered "+std::to_wstring(status);return false;}
 wchar_t cl[32]{};len=sizeof cl;total=0;
 if(WinHttpQueryHeaders(r.h,WINHTTP_QUERY_CONTENT_LENGTH,WINHTTP_HEADER_NAME_BY_INDEX,cl,&len,WINHTTP_NO_HEADER_INDEX))total=std::wcstoull(cl,nullptr,10);
 std::vector<char> buf(1<<20);
 for(;;){
  DWORD got=0;
  if(!WinHttpReadData(r.h,buf.data(),static_cast<DWORD>(buf.size()),&got)){err=L"NETWORK ERROR: the connection was lost during the download";return false;}
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
  // MCL_UPDATE_HOST (testing only) points the check elsewhere, e.g. at a host that doesn't exist.
  wchar_t hostEnv[256]{};const std::wstring host=GetEnvironmentVariableW(L"MCL_UPDATE_HOST",hostEnv,256)>0?hostEnv:kUpdateHost;
  const bool ok=httpGet(std::wstring(L"https://")+host+kUpdatePath,[&](const char* d,DWORD n){body.append(d,n);return body.size()<(4u<<20);},total,err);
  if(!ok){fail(err==L"no release found"?L"no release has been published yet":err);return;}
  ReleaseInfo r;
  if(!parseLatestRelease(body,r)){fail(L"the latest release has no .zip, .rar or .7z to download");return;}
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
  const fs::path zip=work/"update.archive"; // tar finds the format (zip, rar, 7z) from the content
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
  // Windows 10/11 include tar (bsdtar/libarchive), which unpacks .zip, .rar (incl. RAR5) and .7z.
  const std::wstring cmd=L"tar.exe -xf \""+zip.wstring()+L"\" -C \""+unpacked.wstring()+L"\"";
  if(!runHidden(cmd,work,30*60*1000,code)||code!=0){fs::remove_all(work,ec);fail(L"the update could not be unpacked");return;}
  fs::remove(zip,ec);
  const fs::path root=findGameRoot(unpacked);
  if(root.empty()){fs::remove_all(work,ec);fail(L"the download does not contain the game");return;}
  source_=root;progress_=1;state_=State::Ready;
 });
}
void Updater::cancel(){cancel_=true;}
// Only the game and README.txt are visible in the game folder; everything else at the top
// (content, settings, emulators, and any folder a future update adds) gets Hidden + System.
void hideGameFolder(const fs::path& app){
 std::error_code ec;
 for(fs::directory_iterator it(app,ec),end;!ec&&it!=end;it.increment(ec)){
  const std::wstring n=it->path().filename().wstring();
  if(_wcsicmp(n.c_str(),L"MarvelCrossoverLegacy_Features.exe")==0||_wcsicmp(n.c_str(),L"README.txt")==0)continue;
  const DWORD a=GetFileAttributesW(it->path().c_str());
  if(a!=INVALID_FILE_ATTRIBUTES&&(a&(FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM))!=(FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM))
   SetFileAttributesW(it->path().c_str(),a|FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM);
 }
}
bool installUpdate(const fs::path& source,const fs::path& app,long long releaseId,const std::string& version,
 std::wstring& error,const std::function<void(double)>& progress){
 std::error_code ec;
 const fs::path backup=app/"Update"/"backup";
 fs::remove_all(backup,ec);
 // What to install
 std::vector<fs::path> files;
 for(fs::recursive_directory_iterator it(source,ec),end;!ec&&it!=end;it.increment(ec)){
  if(!it->is_regular_file(ec))continue;
  const fs::path rel=fs::relative(it->path(),source,ec);
  if(!ec&&!rel.empty()&&!keptOnUpdate(rel))files.push_back(rel);
 }
 if(files.empty()){error=L"the update has no files to install";return false;}
 struct Step{fs::path dest,saved;}; // saved = where the old file went ("" = there was none)
 std::vector<Step> done;
 const auto undo=[&]{
  for(auto it=done.rbegin();it!=done.rend();++it){
   std::error_code e;
   SetFileAttributesW(it->dest.c_str(),FILE_ATTRIBUTE_NORMAL);
   if(it->saved.empty())fs::remove(it->dest,e);
   else{DeleteFileW(it->dest.c_str());MoveFileExW(it->saved.c_str(),it->dest.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_COPY_ALLOWED);}
  }
 };
 for(size_t i=0;i<files.size();++i){
  const fs::path src=source/files[i],dest=app/files[i];
  fs::create_directories(dest.parent_path(),ec);
  Step step{dest,{}};
  const DWORD a=GetFileAttributesW(dest.c_str());
  if(a!=INVALID_FILE_ATTRIBUTES){
   // the game hides some files; clear that so the file can be moved aside
   SetFileAttributesW(dest.c_str(),FILE_ATTRIBUTE_NORMAL);
   step.saved=backup/files[i];
   fs::create_directories(step.saved.parent_path(),ec);
   if(!MoveFileExW(dest.c_str(),step.saved.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_COPY_ALLOWED)){
    SetFileAttributesW(dest.c_str(),a);
    error=L"a game file is in use and could not be replaced ("+files[i].filename().wstring()+L")";
    undo();hideGameFolder(app);return false;
   }
  }
  if(!MoveFileExW(src.c_str(),dest.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_COPY_ALLOWED)){
   if(!step.saved.empty())MoveFileExW(step.saved.c_str(),dest.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_COPY_ALLOWED);
   error=L"a new file could not be written ("+files[i].filename().wstring()+L")";
   undo();hideGameFolder(app);return false;
  }
  done.push_back(step);
  if(progress)progress(static_cast<double>(i+1)/static_cast<double>(files.size()));
 }
 hideGameFolder(app);
 fs::create_directories(app/"MCL_UserData",ec);
 std::ofstream(app/"MCL_UserData"/"installed_release.txt",std::ios::trunc)<<releaseId<<"\n";
 std::ofstream(app/"MCL_UserData"/"update_result.txt",std::ios::trunc)<<"ok\t"<<version<<"\n";
 return true;
}
bool takeUpdateResult(const fs::path& app,UpdateResult& out){
 const fs::path f=app/"MCL_UserData"/"update_result.txt";
 std::ifstream in(f);std::string word,version;
 if(!(in>>word))return false;
 std::getline(in,version);while(!version.empty()&&(version[0]=='\t'||version[0]==' '))version.erase(0,1);
 in.close();std::error_code ec;fs::remove(f,ec);
 out.ok=word=="ok";out.version=version;return true;
}
void cleanupUpdateFolder(const fs::path& app){
 const fs::path work=app/"Update";
 std::error_code ec;
 if(!fs::exists(work,ec))return;
 std::thread([work]{
  for(int i=0;i<30;++i){ // the previous exe (now in Update\backup) may take a moment to close
   std::error_code e;
   for(fs::recursive_directory_iterator it(work,e),end;!e&&it!=end;it.increment(e))SetFileAttributesW(it->path().c_str(),FILE_ATTRIBUTE_NORMAL);
   SetFileAttributesW(work.c_str(),FILE_ATTRIBUTE_NORMAL);
   fs::remove_all(work,e);
   if(!fs::exists(work,e))return;
   Sleep(1000);
  }
 }).detach();
}
void Updater::startInstall(){
 if(state_!=State::Ready)return;
 join();progress_=0;state_=State::Installing;
 worker_=std::thread([this]{
  const ReleaseInfo r=release();
  std::wstring err;
  const std::string version=r.tag.empty()?r.name:r.tag;
  if(installUpdate(source_,dir_,r.id,version,err,[this](double p){progress_=p;})){progress_=1;state_=State::Installed;}
  else{std::error_code ec;fs::remove_all(dir_/"Update"/"files",ec);fail(err);}
 });
}
bool Updater::restartGame(){
 if(state_!=State::Installed)return false;
 std::wstring cmd=L"\""+(dir_/"MarvelCrossoverLegacy_Features.exe").wstring()+L"\" --updated";
 STARTUPINFOW si{};si.cb=sizeof si;PROCESS_INFORMATION pi{};
 if(!CreateProcessW(nullptr,cmd.data(),nullptr,nullptr,FALSE,0,nullptr,dir_.c_str(),&si,&pi))return false;
 CloseHandle(pi.hThread);CloseHandle(pi.hProcess);
 return true;
}
#endif
}
