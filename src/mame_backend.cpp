#include "mame_backend.hpp"
#include "mame_sets.hpp"
#include "autosave.hpp"
#include <atomic>
#include <thread>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <system_error>
#include <utility>
#ifdef _WIN32
#include <windows.h>
#endif
namespace mcl {
namespace fs=std::filesystem;
std::vector<std::string> summarizeMameLog(const std::string& log,std::size_t maxLines){
 std::vector<std::string> out;
 std::size_t pos=0;
 while(pos<log.size()&&out.size()<maxLines){
  auto end=log.find('\n',pos);if(end==std::string::npos)end=log.size();
  std::string line=log.substr(pos,end-pos);pos=end+1;
  while(!line.empty()&&(line.back()=='\r'||line.back()==' '))line.pop_back();
  std::string low=line;for(auto& c:low)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  if(low.find("not found")!=std::string::npos||low.find("incorrect")!=std::string::npos||
     low.find("required files are missing")!=std::string::npos||low.find("fatal")!=std::string::npos||
     low.find("error")!=std::string::npos||low.find("unknown system")!=std::string::npos||
     low.find("cannot")!=std::string::npos||low.find("has stopped")!=std::string::npos)
   out.push_back(line);
 }
 return out;
}
bool logShowsFailedStart(const std::string& log){
 std::string low=log;for(auto& c:low)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
 return low.find("has stopped")!=std::string::npos||low.find("cannot be run")!=std::string::npos||
        low.find("required files are missing")!=std::string::npos;
}
fs::path readMameExecutable(const fs::path& configFile){
 std::ifstream f(configFile,std::ios::binary);
 std::string s;
 if(!std::getline(f,s))return {};
 if(!s.empty()&&s.back()=='\r')s.pop_back();
 if(s.empty()||s.size()>4096||s.find('\0')!=std::string::npos)return {};
 auto p=fs::u8path(s);
 if(p.is_relative())p=configFile.parent_path()/p;
 return p.lexically_normal();
}
static std::wstring widen(const std::string& ascii){return std::wstring(ascii.begin(),ascii.end());}
// Shared checks + argument construction. Presence only: not a correct revision,
// key or successful boot (MAME -verifyroms is the real check).
static bool buildSpec(const fs::path& emulator,const RomSearchPath& roms,const std::string& game,
 MameLaunchSpec& output,std::string& reason,const char* missingSet,const char* missingBios){
 output={};std::error_code ec;
 const auto* set=findMameSet(game);
 if(!set){reason="Unknown game";return false;}
 if(emulator.empty()||!fs::is_regular_file(emulator,ec)){reason="MAME executable not found";return false;}
 if(roms.empty()||!roms.anyDirectoryExists()){reason="ROM directory not found";return false;}
 if(!roms.hasSet(set->set)){reason=missingSet;return false;}
 if(*set->parent&&!roms.hasSet(set->parent)){reason=std::string("Parent ROM ZIP missing: ")+set->parent+".zip";return false;}
 if(*set->bios&&!roms.hasSet(set->bios)){reason=missingBios;return false;}
 output.executable=fs::absolute(emulator);
 output.roms=roms;
 output.game=game;
 output.mameSet=set->set;
 // -skip_gameinfo: no MAME system-information screen before the game starts.
 output.arguments={widen(set->set),L"-rompath",roms.mameRompath(),L"-skip_gameinfo"};
 reason.clear();return true;
}
bool createCps2LaunchSpec(const fs::path& emulator,const RomSearchPath& roms,
 const std::string& game,MameLaunchSpec& output,std::string& reason){
 if(!isCps2Game(game)){output={};reason="Not a supported CPS-2 title";return false;}
 return buildSpec(emulator,roms,game,output,reason,"Selected ROM ZIP is missing","");
}
bool createNaomiLaunchSpec(const fs::path& emulator,const RomSearchPath& roms,
 MameLaunchSpec& output,std::string& reason){
 return buildSpec(emulator,roms,"mvsc2",output,reason,"MVC2 ROM ZIP missing","NAOMI BIOS ZIP missing");
}
bool createExLaunchSpec(const fs::path& emulator,const RomSearchPath& roms,
 const std::string& game,MameLaunchSpec& output,std::string& reason){
 if(game!="sfexp"&&game!="sfex2p"){output={};reason="Not a supported CHEAT EX arcade title";return false;}
 return buildSpec(emulator,roms,game,output,reason,"Selected EX ROM ZIP missing","EX hardware BIOS ZIP missing");
}
bool createFlycastLaunchSpec(const fs::path& flycast,const RomSearchPath& roms,
 const std::string& game,MameLaunchSpec& output,std::string& reason){
 output={};std::error_code ec;
 const auto* set=findMameSet(game);
 if(!set||set->emulator!=Emulator::Flycast){reason="Not a Flycast game";return false;}
 if(flycast.empty()||!fs::is_regular_file(flycast,ec)){reason="Flycast not found (set its path in flycast_executable.txt)";return false;}
 fs::path zip;
 for(auto it=roms.dirs.rbegin();it!=roms.dirs.rend();++it){
  const auto p=*it/(std::string(set->set)+".zip");
  if(fs::is_regular_file(p,ec)){zip=p;break;}
 }
 if(zip.empty()){reason=std::string("ROM set missing: ")+set->set+".zip";return false;}
 if(*set->bios&&!fs::is_regular_file(zip.parent_path()/(std::string(set->bios)+".zip"),ec)){
  reason=std::string("Flycast needs ")+set->bios+".zip in the same folder as "+set->set+".zip";return false;
 }
 output.executable=fs::absolute(flycast);output.roms=roms;output.game=game;output.mameSet=set->set;
 // input:maple_sdl_keyboard=4: the keyboard drives both players (MvC2 player 2 keys); a second
 // controller is player 2 on its own (Flycast gives the 2nd pad port B).
 output.arguments={L"-config",L"window:fullscreen=yes",L"-config",L"log:LogToFile=yes",L"-config",L"input:maple_sdl_keyboard=4",fs::absolute(zip).wstring()};
 reason.clear();return true;
}
bool createLaunchSpec(const fs::path& emulator,const RomSearchPath& roms,
 const std::string& game,MameLaunchSpec& output,std::string& reason){
 if(game=="mvsc2")return createNaomiLaunchSpec(emulator,roms,output,reason);
 if(game=="sfexp"||game=="sfex2p")return createExLaunchSpec(emulator,roms,game,output,reason);
 return createCps2LaunchSpec(emulator,roms,game,output,reason);
}
#ifdef _WIN32
struct MameBackend::Private {
 std::thread guard; std::atomic<bool> guardOn{false}; std::atomic<bool> wasTopmost{false};
 fs::path executable;
 HANDLE process=nullptr;
 DWORD pid=0;
 fs::path log;          // MAME console capture
 fs::path flycast;      // Flycast executable
 fs::path activeLog;    // log of the current/last launch (MAME capture or flycast.log)
 fs::path cheatScript;  // -autoboot_script for the next MAME launch
 bool windowed=false;   // OPTIONS: games in a window
 std::wstring ctrlr;     // -ctrlr file name (CONTROLS SETUP)
 std::string lastError;
 ULONGLONG startedAt=0;
 explicit Private(fs::path e):executable(std::move(e)){}
 ~Private(){guardOn=false;if(guard.joinable())guard.join();if(process)CloseHandle(process);}
};
static std::wstring quote(const std::wstring& str){
 // Windows CreateProcess argument quoting, including embedded quotes and backslashes.
 std::wstring out=L"\"";unsigned slashes=0;
 for(auto ch:str){
  if(ch==L'\\'){++slashes;continue;}
  if(ch==L'"'){out.append(slashes*2+1,L'\\');out+=ch;slashes=0;continue;}
  out.append(slashes,L'\\');slashes=0;out+=ch;
 }
 out.append(slashes*2,L'\\');out+=L'"';return out;
}
MameBackend::MameBackend(fs::path e):p_(new Private(std::move(e))){}
MameBackend::~MameBackend(){delete p_;}
bool MameBackend::available(const std::string& game)const{
 std::error_code ec;
 if(!findMameSet(game)||running())return false;
 return fs::is_regular_file(usesFlycast(game)?p_->flycast:p_->executable,ec);
}
bool MameBackend::running()const{
 return p_->process&&WaitForSingleObject(p_->process,0)==WAIT_TIMEOUT;
}
bool MameBackend::start(const std::string& id,const RomSearchPath& roms){
 if(running())return false;
 if(p_->process){CloseHandle(p_->process);p_->process=nullptr;p_->pid=0;}
 MameLaunchSpec spec;std::string reason;
 const bool flycast=usesFlycast(id);
 const bool built=flycast?createFlycastLaunchSpec(p_->flycast,roms,id,spec,reason)
                         :createLaunchSpec(p_->executable,roms,id,spec,reason);
 p_->lastError=reason;
 if(!built)return false;
 if(p_->windowed){
  if(flycast){for(auto& a:spec.arguments)if(a==L"window:fullscreen=yes")a=L"window:fullscreen=no";}
  else spec.arguments.push_back(L"-window");
 }
 if(flycast){
  // Flycast writes its own flycast.log next to the exe; clear the previous one first.
  p_->activeLog=spec.executable.parent_path()/"flycast.log";
  std::error_code rm;fs::remove(p_->activeLog,rm);
 }else{
  p_->activeLog=p_->log;
  std::error_code ce;
  // High scores: MAME's hiscore plugin saves each game's table to MAME\hiscore\<set>.hi.
  ensureHiscoreDat(spec.executable);
  if(!p_->ctrlr.empty()){
   spec.arguments.push_back(L"-ctrlr");spec.arguments.push_back(p_->ctrlr);
   spec.arguments.push_back(L"-joystickprovider");spec.arguments.push_back(L"dinput");
  }
  spec.arguments.push_back(L"-plugin");spec.arguments.push_back(L"hiscore");
  if(!p_->cheatScript.empty()&&fs::is_regular_file(p_->cheatScript,ce)){
   spec.arguments.push_back(L"-autoboot_script");
   spec.arguments.push_back(fs::absolute(p_->cheatScript).wstring());
  }
 }
 std::wstring cmd=quote(spec.executable.wstring());
 for(const auto& arg:spec.arguments){cmd+=L" ";cmd+=quote(arg);}
 std::vector<wchar_t> mutableCommand(cmd.begin(),cmd.end());mutableCommand.push_back(L'\0');
 STARTUPINFOW startup{};startup.cb=sizeof(startup);
 PROCESS_INFORMATION proc{};
 std::wstring workingDir=spec.executable.parent_path().wstring();
 // Capture MAME's console output so a failed start can be explained to the player.
 HANDLE logHandle=INVALID_HANDLE_VALUE;
 if(!flycast&&!p_->log.empty()){
  std::error_code ec;fs::create_directories(p_->log.parent_path(),ec);
  SECURITY_ATTRIBUTES sa{sizeof sa,nullptr,TRUE};
  logHandle=CreateFileW(p_->log.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
  if(logHandle!=INVALID_HANDLE_VALUE){
   startup.dwFlags|=STARTF_USESTDHANDLES;
   startup.hStdOutput=logHandle;startup.hStdError=logHandle;startup.hStdInput=nullptr;
  }
 }
 // CREATE_NO_WINDOW: console builds of MAME (mame64.exe) otherwise open an extra
 // black console window beside the game window. The game window is unaffected.
 const BOOL ok=CreateProcessW(spec.executable.c_str(),mutableCommand.data(),nullptr,nullptr,
        logHandle!=INVALID_HANDLE_VALUE,CREATE_NO_WINDOW,nullptr,workingDir.c_str(),&startup,&proc);
 if(logHandle!=INVALID_HANDLE_VALUE)CloseHandle(logHandle);
 if(!ok){p_->lastError=flycast?"Windows could not start Flycast":"Windows could not start MAME";return false;}
 p_->process=proc.hProcess;p_->pid=proc.dwProcessId;CloseHandle(proc.hThread);
 AllowSetForegroundWindow(proc.dwProcessId);
 p_->startedAt=GetTickCount64();
 return true;
}
void MameBackend::setLogFile(fs::path file){p_->log=std::move(file);}
fs::path MameBackend::logFile() const {return p_->activeLog.empty()?p_->log:p_->activeLog;}
void MameBackend::setFlycastExecutable(fs::path file){p_->flycast=std::move(file);}
void MameBackend::setControllerFile(const std::wstring& name){p_->ctrlr=name;}
void MameBackend::setCheatScript(fs::path file){p_->cheatScript=std::move(file);}
void MameBackend::setWindowed(bool w){p_->windowed=w;}
std::string MameBackend::lastStartError() const {return p_->lastError;}
bool MameBackend::lastExit(unsigned long& code,double& seconds) const{
 if(!p_->process||WaitForSingleObject(p_->process,0)!=WAIT_OBJECT_0)return false;
 DWORD c=0;if(!GetExitCodeProcess(p_->process,&c))return false;
 code=c;seconds=static_cast<double>(GetTickCount64()-p_->startedAt)/1000.0;
 return true;
}
struct CloseTarget {DWORD pid;bool sent;};
static BOOL CALLBACK closeWindow(HWND h,LPARAM user){
 auto& target=*reinterpret_cast<CloseTarget*>(user);
 DWORD pid=0;GetWindowThreadProcessId(h,&pid);
 if(pid==target.pid&&GetWindow(h,GW_OWNER)==nullptr){
  PostMessageW(h,WM_CLOSE,0,0);target.sent=true;
 }
 return TRUE;
}
bool MameBackend::requestClose(){
 if(!running())return false;
 CloseTarget target{p_->pid,false};EnumWindows(closeWindow,reinterpret_cast<LPARAM>(&target));
 return target.sent;
}
bool MameBackend::stop(){
 if(!p_->process)return false;
 if(running()){
  CloseTarget target{p_->pid,false};EnumWindows(closeWindow,reinterpret_cast<LPARAM>(&target));
  if(!target.sent)return false;
  if(WaitForSingleObject(p_->process,5000)!=WAIT_OBJECT_0)return false;
 }
 CloseHandle(p_->process);p_->process=nullptr;p_->pid=0;
 return true;
}
// Only act on a verified, foreground, top-level window owned by our MAME process.
// SendInput can be blocked by Windows integrity levels and MAME input settings.
struct PauseWindowLookup {DWORD pid; HWND window;};
static BOOL CALLBACK findPauseWindow(HWND h,LPARAM arg){
 auto& t=*reinterpret_cast<PauseWindowLookup*>(arg);
 DWORD pid=0;GetWindowThreadProcessId(h,&pid);
 if(pid==t.pid && IsWindowVisible(h) && GetWindow(h,GW_OWNER)==nullptr){
  t.window=h;return FALSE;
 }
 return TRUE;
}
void MameBackend::startCoverGuard(HWND cover){
 stopCoverGuard();
 if(!running())return;
 p_->guardOn=true;p_->wasTopmost=false;
 const DWORD pid=p_->pid;
 (void)cover;
 p_->guard=std::thread([this,pid]{
  // 1 ms waits without changing the system timer: high-resolution waitable timer (Win10+),
  // falling back to Sleep(1).
  HANDLE timer=CreateWaitableTimerExW(nullptr,nullptr,0x00000002/*CREATE_WAITABLE_TIMER_HIGH_RESOLUTION*/,TIMER_ALL_ACCESS);
  while(p_->guardOn){
   PauseWindowLookup lookup{pid,nullptr};
   EnumWindows(findPauseWindow,reinterpret_cast<LPARAM>(&lookup));
   // Only the emulator window is touched (synchronously): out of "always on top" it can
   // never cover the front end's topmost loading screen. The cover itself is left alone,
   // so nothing is queued that could re-raise it after loading.
   if(lookup.window&&(GetWindowLongW(lookup.window,GWL_EXSTYLE)&WS_EX_TOPMOST)){
    p_->wasTopmost=true;
    SetWindowPos(lookup.window,HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOOWNERZORDER);
   }
   if(timer){LARGE_INTEGER due;due.QuadPart=-10000;SetWaitableTimer(timer,&due,0,nullptr,nullptr,FALSE);WaitForSingleObject(timer,5);}
   else Sleep(1);
  }
  if(timer)CloseHandle(timer);
 });
}
void MameBackend::stopCoverGuard(){
 p_->guardOn=false;
 if(p_->guard.joinable())p_->guard.join();
}
bool MameBackend::bringToFront(){
 if(!running())return false;
 PauseWindowLookup lookup{p_->pid,nullptr};
 EnumWindows(findPauseWindow,reinterpret_cast<LPARAM>(&lookup));
 if(!lookup.window)return false;
 if(p_->wasTopmost){SetWindowPos(lookup.window,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);p_->wasTopmost=false;}
 ShowWindow(lookup.window,SW_SHOW);
 SetForegroundWindow(lookup.window);
 return true;
}
bool MameBackend::requestPauseHotkey(){
 if(!running())return false;
 PauseWindowLookup lookup{p_->pid,nullptr};
 EnumWindows(findPauseWindow,reinterpret_cast<LPARAM>(&lookup));
 if(!lookup.window)return false;
 if(GetForegroundWindow()!=lookup.window){
  if(!SetForegroundWindow(lookup.window))return false;
  if(GetForegroundWindow()!=lookup.window)return false;
 }
 // Current MAME documentation: default UI Pause is F5. Do not use P (legacy).
 INPUT key[2]{};
 key[0].type=INPUT_KEYBOARD;key[0].ki.wVk=VK_F5;
 key[1].type=INPUT_KEYBOARD;key[1].ki.wVk=VK_F5;key[1].ki.dwFlags=KEYEVENTF_KEYUP;
 return SendInput(2,key,sizeof(INPUT))==2;
}
bool MameBackend::pause(bool){return false; /* No verified pause state/acknowledgement protocol yet. */}
HWND MameBackend::window() const{
 if(!running())return nullptr;
 PauseWindowLookup lookup{p_->pid,nullptr};
 EnumWindows(findPauseWindow,reinterpret_cast<LPARAM>(&lookup));
 return lookup.window;
}
bool MameBackend::freeze(bool frozen){
 if(!p_->process)return false;
 using NtProc=LONG(NTAPI*)(HANDLE);
 static const HMODULE nt=GetModuleHandleW(L"ntdll.dll");
 if(!nt)return false;
 const auto fn=reinterpret_cast<NtProc>(reinterpret_cast<void*>(GetProcAddress(nt,frozen?"NtSuspendProcess":"NtResumeProcess")));
 return fn&&fn(p_->process)>=0;
}
int MameBackend::naomiAdvancing(){
 if(!running())return -1;
 // NAOMI main RAM is Flycast's one 32 MB file-mapped region; MvC2's variables sit in its first 4 MB.
 std::uintptr_t base=0;MEMORY_BASIC_INFORMATION mbi{};
 for(std::uintptr_t a=0;VirtualQueryEx(p_->process,reinterpret_cast<void*>(a),&mbi,sizeof mbi)==sizeof mbi;a=reinterpret_cast<std::uintptr_t>(mbi.BaseAddress)+mbi.RegionSize){
  if(mbi.State==MEM_COMMIT&&mbi.Type==MEM_MAPPED&&mbi.RegionSize==0x2000000&&!(mbi.Protect&PAGE_GUARD)){base=reinterpret_cast<std::uintptr_t>(mbi.BaseAddress);break;}
  if(mbi.RegionSize==0)break;
 }
 if(!base)return -1;
 const size_t size=0x400000;std::vector<char> a(size),b(size);SIZE_T got=0;
 if(!ReadProcessMemory(p_->process,reinterpret_cast<void*>(base),a.data(),size,&got)||got!=size)return -1;
 Sleep(150);
 if(!ReadProcessMemory(p_->process,reinterpret_cast<void*>(base),b.data(),size,&got)||got!=size)return -1;
 return a==b?0:1;
}
bool MameBackend::tapKey(unsigned vk){
 const HWND h=window();if(!h)return false;
 const UINT sc=MapVirtualKeyW(vk,MAPVK_VK_TO_VSC);
 PostMessageW(h,WM_KEYDOWN,vk,static_cast<LPARAM>(1|(sc<<16)));
 Sleep(80);
 PostMessageW(h,WM_KEYUP,vk,static_cast<LPARAM>(1|(sc<<16)|(3u<<30)));
 return true;
}
#endif
}
