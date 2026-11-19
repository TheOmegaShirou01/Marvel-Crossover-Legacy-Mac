#include "mame_backend.hpp"
#include <algorithm>
#include <fstream>
#include <system_error>
#include <utility>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
namespace mcl {
namespace fs=std::filesystem;
static bool cps2Game(const std::string& id){
 return id=="xmcota"||id=="msh"||id=="xmvsf"||id=="mshvsf"||id=="mvsc";
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
bool createCps2LaunchSpec(const fs::path& emulator,const fs::path& romDirectory,
 const std::string& game,MameLaunchSpec& output,std::string& reason){
 output={};
 if(!cps2Game(game)){reason="Not a supported CPS-2 title";return false;}
 std::error_code ec;
 if(emulator.empty()||!fs::is_regular_file(emulator,ec)){reason="MAME executable not found";return false;}
 if(romDirectory.empty()||!fs::is_directory(romDirectory,ec)){reason="ROM directory not found";return false;}
 // This verifies presence only, not a correct revision, BIOS, key or successful boot.
 if(!fs::is_regular_file(romDirectory/(game+".zip"),ec)){
  reason="Selected ROM ZIP is missing";return false;
 }
 output.executable=fs::absolute(emulator);
 output.romDirectory=fs::absolute(romDirectory);
 output.game=game;
 output.arguments={std::wstring(game.begin(),game.end()),L"-rompath",output.romDirectory.wstring()};
 reason.clear();return true;
}
bool createNaomiLaunchSpec(const fs::path& emulator,const fs::path& romDirectory,
 MameLaunchSpec& output,std::string& reason){
 output={};std::error_code ec;
 if(emulator.empty()||!fs::is_regular_file(emulator,ec)){reason="MAME executable not found";return false;}
 if(romDirectory.empty()||!fs::is_directory(romDirectory,ec)){reason="ROM directory not found";return false;}
 if(!fs::is_regular_file(romDirectory/"mvsc2.zip",ec)){reason="MVC2 ROM ZIP missing";return false;}
 if(!fs::is_regular_file(romDirectory/"naomi.zip",ec)){reason="NAOMI BIOS ZIP missing";return false;}
 output.executable=fs::absolute(emulator);output.romDirectory=fs::absolute(romDirectory);
 output.game="mvsc2";output.arguments={L"mvsc2",L"-rompath",output.romDirectory.wstring()};
 reason.clear();return true;
}
bool createExLaunchSpec(const fs::path& emulator,const fs::path& romDirectory,
 const std::string& game,MameLaunchSpec& output,std::string& reason){
 output={};std::error_code ec;
 if(game!="sfexp"&&game!="sfex2p"){reason="Not a supported CHEAT EX arcade title";return false;}
 if(emulator.empty()||!fs::is_regular_file(emulator,ec)){reason="MAME executable not found";return false;}
 if(romDirectory.empty()||!fs::is_directory(romDirectory,ec)){reason="ROM directory not found";return false;}
 const auto bios=game=="sfexp"?"coh1000c.zip":"coh3002c.zip";
 if(!fs::is_regular_file(romDirectory/(game+".zip"),ec)){reason="Selected EX ROM ZIP missing";return false;}
 if(!fs::is_regular_file(romDirectory/bios,ec)){reason="EX hardware BIOS ZIP missing";return false;}
 output.executable=fs::absolute(emulator);output.romDirectory=fs::absolute(romDirectory);
 output.game=game;output.arguments={std::wstring(game.begin(),game.end()),L"-rompath",output.romDirectory.wstring()};
 reason.clear();return true;
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
 return (cps2Game(game)||game=="mvsc2"||game=="sfexp"||game=="sfex2p")&&fs::is_regular_file(p_->executable,ec)&&!running();
}
bool MameBackend::running()const{
 return p_->process&&WaitForSingleObject(p_->process,0)==WAIT_TIMEOUT;
}
bool MameBackend::start(const std::string& id,const fs::path& roms){
 if(running())return false;
 if(p_->process){CloseHandle(p_->process);p_->process=nullptr;p_->pid=0;}
 MameLaunchSpec spec;std::string reason;
 if(id=="mvsc2") {if(!createNaomiLaunchSpec(p_->executable,roms,spec,reason))return false;}
 else if(id=="sfexp"||id=="sfex2p"){if(!createExLaunchSpec(p_->executable,roms,id,spec,reason))return false;}
 else if(!createCps2LaunchSpec(p_->executable,roms,id,spec,reason))return false;
 std::wstring cmd=quote(spec.executable.wstring());
 for(const auto& arg:spec.arguments){cmd+=L" ";cmd+=quote(arg);}
 std::vector<wchar_t> mutableCommand(cmd.begin(),cmd.end());mutableCommand.push_back(L'\0');
 STARTUPINFOW startup{};startup.cb=sizeof(startup);
 PROCESS_INFORMATION proc{};
 std::wstring workingDir=spec.executable.parent_path().wstring();
 if(!CreateProcessW(spec.executable.c_str(),mutableCommand.data(),nullptr,nullptr,FALSE,0,nullptr,
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
