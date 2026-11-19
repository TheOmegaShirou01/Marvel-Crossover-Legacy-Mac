#include "../src/rom_audit.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
namespace fs=std::filesystem;
int main(){
 auto root=fs::temp_directory_path()/"mcl_audit_0312";
 fs::remove_all(root);fs::create_directories(root/"ROM archive");
 auto rom=root/"ROM archive", exe=root/"mame.exe";
 {std::ofstream f(exe);f<<"fake executable for spec testing";}
 for(auto name:{"xmcota.zip","msh.zip","xmvsf.zip","mshvsf.zip","mvsc.zip","mvsc2.zip","naomi.zip","sfexp.zip","coh1000c.zip","sfex2p.zip","coh3002c.zip"}){
  std::ofstream f(rom/name);f<<"fake file for presence testing";
 }
 for(auto game:{"xmcota","msh","xmvsf","mshvsf","mvsc","mvsc2","sfexp","sfex2p"}){
  mcl::MameLaunchSpec spec;std::string why;
  assert(mcl::createMameAuditSpec(exe,rom,game,spec,why));
  assert(spec.game==game && spec.arguments.size()==4);
  assert(spec.arguments[0]==L"-verifyroms" && spec.arguments[2]==L"-rompath");
  assert(spec.arguments[3].find(L"ROM archive")!=std::wstring::npos);
 }
 for(auto game:{"sfex3","invalid","mvsc; del *",""}){
  mcl::MameLaunchSpec spec;std::string why;
  assert(!mcl::createMameAuditSpec(exe,rom,game,spec,why));
  assert(spec.game.empty());
 }
 fs::remove(rom/"naomi.zip");{
  mcl::MameLaunchSpec spec;std::string why;
  assert(!mcl::createMameAuditSpec(exe,rom,"mvsc2",spec,why));
 }
 assert(mcl::interpretMameAuditExit(0).status==mcl::AuditStatus::Pass);
 assert(mcl::interpretMameAuditExit(2).status==mcl::AuditStatus::Failed);
 fs::remove_all(root);std::cout<<"PASS: eight-game audit specs, BIOS gate, unsupported IDs, exit interpretation\n";
}
