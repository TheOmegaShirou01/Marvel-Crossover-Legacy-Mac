#include "mame_backend.hpp"
#include "mame_sets.hpp"
#include <algorithm>
#include <fstream>
#include <system_error>
#include <utility>
#ifdef _WIN32
#include <windows.h>
#endif
namespace mcl {
namespace fs=std::filesystem;
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
 output.arguments={widen(set->set),L"-rompath",roms.mameRompath()};
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
bool createLaunchSpec(const fs::path& emulator,const RomSearchPath& roms,
 const std::string& game,MameLaunchSpec& output,std::string& reason){
 if(game=="mvsc2")return createNaomiLaunchSpec(emulator,roms,output,reason);
 if(game=="sfexp"||game=="sfex2p")return createExLaunchSpec(emulator,roms,game,output,reason);
 return createCps2LaunchSpec(emulator,roms,game,output,reason);
}
#ifdef _WIN32
struct MameBackend::Private {
 fs::path executable;
 HANDLE process=nullptr;
 DWORD pid=0;
 explicit Private(fs::path e):executable(std::move(e)){}
 ~Private(){if(process)CloseHandle(process);}
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
 return findMameSet(game)&&fs::is_regular_file(p_->executable,ec)&&!running();
}
bool MameBackend::running()const{
 return p_->process&&WaitForSingleObject(p_->process,0)==WAIT_TIMEOUT;
}
bool MameBackend::start(const std::string& id,const RomSearchPath& roms){
 if(running())return false;
 if(p_->process){CloseHandle(p_->process);p_->process=nullptr;p_->pid=0;}
 MameLaunchSpec spec;std::string reason;
 if(!createLaunchSpec(p_->executable,roms,id,spec,reason))return false;
 std::wstring cmd=quote(spec.executable.wstring());
 for(const auto& arg:spec.arguments){cmd+=L" ";cmd+=quote(arg);}
 std::vector<wchar_t> mutableCommand(cmd.begin(),cmd.end());mutableCommand.push_back(L'\0');
 STARTUPINFOW startup{};startup.cb=sizeof(startup);
 PROCESS_INFORMATION proc{};
 std::wstring workingDir=spec.executable.parent_path().wstring();
 // CREATE_NO_WINDOW: console builds of MAME (mame64.exe) otherwise open an extra
 // black console window beside the game window. The game window is unaffected.
 if(!CreateProcessW(spec.executable.c_str(),mutableCommand.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,
        workingDir.c_str(),&startup,&proc))return false;
 p_->process=proc.hProcess;p_->pid=proc.dwProcessId;CloseHandle(proc.hThread);
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
#endif
}
