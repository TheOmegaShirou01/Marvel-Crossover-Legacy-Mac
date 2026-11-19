#include "../src/mame_backend.hpp"
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
 assert(spec.arguments.size()==3);
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
 {std::ofstream f(root/"mame_executable.txt");f<<"mame test.exe\n";}
 assert(mcl::readMameExecutable(root/"mame_executable.txt")==exe);
 fs::remove_all(root);
 std::cout<<"PASS: MAME CPS-2 allowlist, external ROM validation, spaces in paths, invalid games, configuration\n";
}
