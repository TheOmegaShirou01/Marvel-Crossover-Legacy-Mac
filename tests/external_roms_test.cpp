#include "../src/external_roms.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace fs=std::filesystem;
int main(){
 const auto root=fs::temp_directory_path()/"mcl_external_rom_test_v037";
 fs::remove_all(root);fs::create_directories(root/"rom test");
 {std::ofstream f(root/"rom_directory.txt");f<<"rom test\n";}
 const auto dir=mcl::readRomDirectory(root/"rom_directory.txt");
 assert(dir==root/"rom test");
 auto state=mcl::checkExternalRomFiles(dir,"sfexp");
 assert(!state.readyForFileLevelCheck()&&state.files.size()==2);
 {std::ofstream f(dir/"sfexp.zip",std::ios::binary);f<<"not a zip";}
 state=mcl::checkExternalRomFiles(dir,"sfexp");assert(state.files[0].present&&!state.files[0].looksLikeZip);
 for(const auto* n:{"sfexp.zip","coh1000c.zip","sfex2p.zip","coh3002c.zip"}){
  std::ofstream f(dir/n,std::ios::binary|std::ios::trunc);f.put('P');f.put('K');f.put(3);f.put(4);f<<"mock data";
 }
 assert(mcl::checkExternalRomFiles(dir,"sfexp").readyForFileLevelCheck());
 assert(mcl::checkExternalRomFiles(dir,"sfex2p").readyForFileLevelCheck());
 bool rejected=false;try{mcl::checkExternalRomFiles(dir,"sfex3");}catch(const std::invalid_argument&){rejected=true;}
 assert(rejected);
 assert(mcl::readRomDirectory(root/"not_here.txt").empty());
 fs::remove_all(root);
 std::cout<<"PASS: external ROM configuration, ZIP header, missing files, separate EX dependencies, EX3 rejected\n";
}
