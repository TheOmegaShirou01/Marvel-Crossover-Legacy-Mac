#include "../src/mame_backend.hpp"
#include "../src/mame_sets.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
namespace fs=std::filesystem;
int main(){
 const auto root=fs::temp_directory_path()/"mcl_039_launch_validation";
 fs::remove_all(root);fs::create_directories(root/"ROM Files");
 const auto exe=root/"mame test.exe";
 {std::ofstream f(exe);f<<"placeholder for path validation only";}
 {std::ofstream f(root/"ROM Files"/"xmcota.zip");f<<"not real ROM";}
 mcl::MameLaunchSpec spec;std::string reason;
 // Collection "xmcota" launches the USA clone xmcotau, which needs its own zip too.
 assert(!mcl::createCps2LaunchSpec(exe,root/"ROM Files","xmcota",spec,reason));
 {std::ofstream f(root/"ROM Files"/"xmcotau.zip");f<<"not real ROM";}
 assert(mcl::createCps2LaunchSpec(exe,root/"ROM Files","xmcota",spec,reason));
 assert(spec.game=="xmcota" && spec.mameSet=="xmcotau");
 assert(spec.arguments.size()==4&&spec.arguments[3]==L"-skip_gameinfo");
 assert(spec.arguments[0]==L"xmcotau");
 assert(spec.arguments[1]==L"-rompath");
 assert(spec.arguments[2].find(L"ROM Files")!=std::wstring::npos);
 assert(!mcl::createCps2LaunchSpec(exe,root/"ROM Files","sfexp",spec,reason));
 assert(!mcl::createCps2LaunchSpec(exe,root/"ROM Files","mvsc2",spec,reason));
 assert(!mcl::createCps2LaunchSpec(exe,root/"ROM Files","xmcota && del *",spec,reason));
 assert(!mcl::createCps2LaunchSpec(exe,root/"missing","xmcota",spec,reason));
 // Clone in one folder, parent in another: both folders go into -rompath.
 fs::create_directories(root/"Extra");
 fs::remove(root/"ROM Files"/"xmcotau.zip");
 {std::ofstream f(root/"Extra"/"xmcotau.zip");f<<"not real ROM";}
 mcl::RomSearchPath two;two.dirs={root/"ROM Files",root/"Extra"};
 assert(mcl::createCps2LaunchSpec(exe,two,"xmcota",spec,reason));
 assert(spec.arguments[2].find(L"ROM Files;")!=std::wstring::npos);
 assert(spec.arguments[2].find(L"Extra")!=std::wstring::npos);
 // A set supplied as a folder (MAME accepts <set>/ directories) also counts.
 fs::remove(root/"Extra"/"xmcotau.zip");fs::create_directories(root/"Extra"/"xmcotau");
 assert(mcl::createCps2LaunchSpec(exe,two,"xmcota",spec,reason));
 // Parent missing everywhere -> rejected.
 fs::remove(root/"ROM Files"/"xmcota.zip");
 assert(!mcl::createCps2LaunchSpec(exe,two,"xmcota",spec,reason));
 // Failed-start explanation picks the relevant MAME lines only.
 {
  const std::string log=
   "mpr-23084.ic32s NOT FOUND (tried in mvsc2 naomi)\r\n"
   "315-6146.bin NOT FOUND (tried in mie mvsc2 naomi)\r\n"
   "WARNING: the machine might not run correctly.\n"
   "Required files are missing, the machine cannot be run.\n"
   "Average speed: 100%\n";
  const auto lines=mcl::summarizeMameLog(log,10);
  assert(lines.size()==3);
  assert(lines[0]=="mpr-23084.ic32s NOT FOUND (tried in mvsc2 naomi)");
  assert(lines[2].find("Required files are missing")==0);
  assert(mcl::summarizeMameLog(log,1).size()==1);
  assert(mcl::summarizeMameLog("Average speed: 99.8%\n").empty());
 }
 // Flycast: MvC2 only; last folder holding mvsc2.zip wins; BIOS must sit beside it.
 {
  const auto orig=root/"orig",extra=root/"extra",fly=root/"flycast.exe";
  fs::create_directories(orig);fs::create_directories(extra);
  {std::ofstream f(fly);f<<"x";}
  mcl::RomSearchPath search;search.dirs={orig,extra};
  assert(mcl::usesFlycast("mvsc2")&&!mcl::usesFlycast("xmcota"));
  assert(!mcl::createFlycastLaunchSpec(fly,search,"mvsc2",spec,reason)&&reason.find("mvsc2.zip")!=std::string::npos);
  {std::ofstream f(orig/"mvsc2.zip");f<<"x";}
  assert(!mcl::createFlycastLaunchSpec(fly,search,"mvsc2",spec,reason)&&reason.find("naomi.zip")!=std::string::npos);
  {std::ofstream f(orig/"naomi.zip");f<<"x";}
  assert(mcl::createFlycastLaunchSpec(fly,search,"mvsc2",spec,reason));
  assert(spec.arguments.size()==7&&spec.arguments[1]==L"window:fullscreen=yes"&&spec.arguments[5]==L"input:maple_sdl_keyboard=4");
  assert(spec.arguments[6].find(L"orig")!=std::wstring::npos);
  // A repaired copy in the later folder overrides, but only with its BIOS beside it.
  {std::ofstream f(extra/"mvsc2.zip");f<<"x";}
  assert(!mcl::createFlycastLaunchSpec(fly,search,"mvsc2",spec,reason));
  {std::ofstream f(extra/"naomi.zip");f<<"x";}
  assert(mcl::createFlycastLaunchSpec(fly,search,"mvsc2",spec,reason)&&spec.arguments[6].find(L"extra")!=std::wstring::npos);
  assert(!mcl::createFlycastLaunchSpec(fly,search,"xmcota",spec,reason));
  assert(!mcl::createFlycastLaunchSpec(root/"nope.exe",search,"mvsc2",spec,reason));
  // Flycast exits 0 on failure; the log decides.
  const std::string flog="00:01:118 ui/gui.cpp:1318 E[BOOT]: Cannot find mpr-23084.ic32s\n"
                         "00:01:118 ui/gui.cpp:484 E[COMMON]: Flycast has stopped: Cannot find mpr-23084.ic32s\n";
  assert(mcl::logShowsFailedStart(flog));
  assert(mcl::summarizeMameLog(flog).size()==2);
  assert(!mcl::logShowsFailedStart("N[BOOT]: Game ID is [MARVEL VS CAPCOM2  JAPAN]\n"));
  assert(mcl::logShowsFailedStart("Fatal error: Required files are missing, the machine cannot be run.\n"));
 }
 {std::ofstream f(root/"mame_executable.txt");f<<"mame test.exe\n";}
 assert(mcl::readMameExecutable(root/"mame_executable.txt")==exe);
 fs::remove_all(root);
 std::cout<<"PASS: MAME CPS-2 allowlist, external ROM validation, spaces in paths, invalid games, configuration\n";
}
