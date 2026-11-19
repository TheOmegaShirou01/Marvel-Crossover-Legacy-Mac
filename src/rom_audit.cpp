#include "rom_audit.hpp"
#include <algorithm>
#include <system_error>
#include <vector>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
namespace mcl {
namespace fs=std::filesystem;
bool createMameAuditSpec(const fs::path& exe,const RomSearchPath& roms,
 const std::string& game,MameLaunchSpec& out,std::string& reason){
 out={};
 MameLaunchSpec launch;
 if(!createLaunchSpec(exe,roms,game,launch,reason))return false;
 out=launch;
 out.arguments={L"-verifyroms",std::wstring(out.mameSet.begin(),out.mameSet.end()),L"-rompath",roms.mameRompath()};
 reason.clear();return true;
}
AuditResult interpretMameAuditExit(unsigned long code){
 return code==0?AuditResult{AuditStatus::Pass,code,"MAME reports ROM set verified"}:
 AuditResult{AuditStatus::Failed,code,"MAME ROM audit failed; check ROM set, key, BIOS and MAME version"};
}
#ifdef _WIN32
static std::wstring quoteArg(const std::wstring& in){
 std::wstring out=L"\"";size_t slash=0;
 for(wchar_t ch:in){
  if(ch==L'\\'){++slash;continue;}
  if(ch==L'"'){out.append(slash*2+1,L'\\');out+=ch;slash=0;continue;}
  out.append(slash,L'\\');slash=0;out+=ch;
 }
 out.append(slash*2,L'\\');out+=L'"';return out;
}
AuditResult runMameRomAudit(const MameLaunchSpec& spec,unsigned timeoutMs){
 std::error_code ec;
 if(spec.executable.empty()||!fs::is_regular_file(spec.executable,ec))
  return {AuditStatus::MissingFiles,0,"MAME executable not found"};
 if(spec.arguments.size()!=4 || spec.arguments[0]!=L"-verifyroms")
  return {AuditStatus::CannotStart,0,"Invalid audit specification"};
 std::wstring cmd=quoteArg(spec.executable.wstring());
 for(const auto& a:spec.arguments){cmd+=L" ";cmd+=quoteArg(a);}
 std::vector<wchar_t> command(cmd.begin(),cmd.end());command.push_back(L'\0');
 STARTUPINFOW si{};si.cb=sizeof si; PROCESS_INFORMATION pi{};
 auto working=spec.executable.parent_path().wstring();
 if(!CreateProcessW(spec.executable.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,
  nullptr,working.c_str(),&si,&pi))return {AuditStatus::CannotStart,GetLastError(),"Unable to start MAME audit"};
 CloseHandle(pi.hThread);
 DWORD wait=WaitForSingleObject(pi.hProcess,timeoutMs);
 if(wait==WAIT_TIMEOUT){TerminateProcess(pi.hProcess,258);WaitForSingleObject(pi.hProcess,5000);
  CloseHandle(pi.hProcess);return {AuditStatus::TimedOut,258,"MAME audit timed out"};}
 if(wait!=WAIT_OBJECT_0){CloseHandle(pi.hProcess);return {AuditStatus::CannotStart,0,"Unable to wait for audit"};}
 DWORD code=1;
 if(!GetExitCodeProcess(pi.hProcess,&code)){CloseHandle(pi.hProcess);return {AuditStatus::CannotStart,0,"Audit process status unavailable"};}
 CloseHandle(pi.hProcess);
 return interpretMameAuditExit(code);
}
#endif
}
