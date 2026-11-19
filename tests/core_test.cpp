#include "../src/core.hpp"
#include <cassert>
#include <iostream>
#include <filesystem>
#include <fstream>
int main(){
 namespace fs=std::filesystem;
 fs::path root=fs::temp_directory_path()/"mcl_core_test_sandbox";fs::remove_all(root);
 {mcl::Collection c(root);assert(c.games().size()==8);assert(c.achievements().size()==108);assert(!c.recordEvent("bad", "win"));assert(!c.recordEvent("mvsc", "win",-1));assert(c.recordEvent("mvsc","win",3));assert(c.stats("mvsc").wins==3);
 assert(c.unlockAchievement("mvsc_01"));assert(!c.unlockAchievement("mvsc_01"));assert(!c.unlockAchievement("unknown"));
 assert(!c.saveState("mvsc",0,{1,2}));assert(!c.saveState("../bad",1,{1,2}));assert(c.saveState("mvsc",1,{1,2,3}));
 std::vector<unsigned char> bytes;assert(c.loadState("mvsc",1,bytes));assert((bytes==std::vector<unsigned char>{1,2,3}));
 fs::create_directories(root/"media"/"mvsc"/"music");std::ofstream(root/"media"/"mvsc"/"music"/"sample.wav")<<"dummy";std::ofstream(root/"media"/"mvsc"/"music"/"unsafe.exe")<<"dummy";assert(c.media("mvsc","music").size()==1);}
 {mcl::Collection c(root);assert(c.stats("mvsc").wins==3);assert(c.progress("mvsc_01").unlocked);assert(c.media("mvsc","music").size()==1);}
 fs::remove_all(root);std::cout<<"PASS: 8 games, 108 achievement definitions, persistence, validation, state slots, media scan\n";
}
