#include "../src/mame_backend.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
namespace fs=std::filesystem;
int main(){
 auto root=fs::temp_directory_path()/"mcl_ex_backend_0311";fs::remove_all(root);fs::create_directories(root/"ROM space");
 auto rom=root/"ROM space",exe=root/"mame file.exe";
 {std::ofstream f(exe);f<<"test dummy";}
 mcl::MameLaunchSpec spec;std::string why;
 assert(!mcl::createExLaunchSpec(exe,rom,"sfexp",spec,why));
 {std::ofstream f(rom/"sfexp.zip");f<<"test dummy";}
 assert(!mcl::createExLaunchSpec(exe,rom,"sfexp",spec,why));
 assert(why=="EX hardware BIOS ZIP missing");
 {std::ofstream f(rom/"coh1000c.zip");f<<"test dummy";}
 assert(mcl::createExLaunchSpec(exe,rom,"sfexp",spec,why));
 assert(spec.game=="sfexp"&&spec.arguments[0]==L"sfexp"&&spec.arguments[1]==L"-rompath"&&spec.arguments.size()==4&&spec.arguments[3]==L"-skip_gameinfo");
 assert(spec.arguments[2].find(L"ROM space")!=std::wstring::npos);
 assert(!mcl::createExLaunchSpec(exe,rom,"sfex2p",spec,why));
 {std::ofstream f(rom/"sfex2p.zip");f<<"test dummy";}
 assert(!mcl::createExLaunchSpec(exe,rom,"sfex2p",spec,why));
 {std::ofstream f(rom/"coh3002c.zip");f<<"test dummy";}
 assert(mcl::createExLaunchSpec(exe,rom,"sfex2p",spec,why));
 assert(spec.game=="sfex2p"&&spec.arguments[0]==L"sfex2p");
 for(auto id:{"sfex3","mvsc2","sfexp; del *",""})assert(!mcl::createExLaunchSpec(exe,rom,id,spec,why));
 assert(!mcl::createExLaunchSpec(root/"missing.exe",rom,"sfexp",spec,why));
 assert(!mcl::createExLaunchSpec(exe,root/"missing", "sfexp",spec,why));
 fs::remove_all(root);std::cout<<"PASS: ZN-1/ZN-2 ROM and BIOS, game allowlist, paths, failure cases\n";
}
