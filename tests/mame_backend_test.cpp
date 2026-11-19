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
 assert(mcl::createCps2LaunchSpec(exe,root/"ROM Files","xmcota",spec,reason));
 assert(spec.game=="xmcota");
 assert(spec.arguments.size()==3);
 assert(spec.arguments[0]==L"xmcota");
 assert(spec.arguments[1]==L"-rompath");
 assert(spec.arguments[2].find(L"ROM Files")!=std::wstring::npos);
 assert(!mcl::createCps2LaunchSpec(exe,root/"ROM Files","sfexp",spec,reason));
 assert(!mcl::createCps2LaunchSpec(exe,root/"ROM Files","mvsc2",spec,reason));
 assert(!mcl::createCps2LaunchSpec(exe,root/"ROM Files","xmcota && del *",spec,reason));
 assert(!mcl::createCps2LaunchSpec(exe,root/"missing","xmcota",spec,reason));
 {std::ofstream f(root/"mame_executable.txt");f<<"mame test.exe\n";}
 assert(mcl::readMameExecutable(root/"mame_executable.txt")==exe);
 fs::remove_all(root);
 std::cout<<"PASS: MAME CPS-2 allowlist, external ROM validation, spaces in paths, invalid games, configuration\n";
}
