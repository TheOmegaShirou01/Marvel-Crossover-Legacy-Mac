#include "autoconfig.hpp"
#include "external_roms.hpp"
#include "mame_backend.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
namespace fs=std::filesystem;
static void touch(const fs::path& p){fs::create_directories(p.parent_path());std::ofstream(p)<<"x";}
int main(){
 const fs::path root=fs::temp_directory_path()/"mcl_autoconfig_test";
 fs::remove_all(root);
 // a fake player PC: the game folder, ROMs scattered in two places, emulators elsewhere
 const fs::path app=root/"Games"/"MCL";
 fs::create_directories(app);
 for(const char* z:{"xmcotau.zip","xmcota.zip","mshud.zip","msh.zip","xmvsfur2.zip","xmvsf.zip","mshvsfu1d.zip","mshvsf.zip","mvscu.zip","mvsc.zip"})touch(root/"Desktop"/"Arcade"/"roms"/z);
 for(const char* z:{"sfexp.zip","coh1000c.zip","sfex2p.zip","coh3002c.zip","mvsc2.zip","naomi.zip","qsound.zip"})touch(root/"Downloads"/"more"/z);
 touch(root/"Desktop"/"Emus"/"MAME"/"mame.exe");
 touch(root/"Desktop"/"Emus"/"Flycast"/"flycast.exe");
 touch(root/"Desktop"/"notroms"/"readme.txt");
 // names outside the ANSI code page (emoji + katakana) must not break the search
 touch(root/"Desktop"/fs::path(L"\U0001F3AE games \u30DE\u30FC\u30D9\u30EB")/"notes.txt");
 const std::vector<fs::path> roots{app,root/"Desktop",root/"Downloads"};
 assert(mcl::knownRomZips().size()>=17);
 const auto folders=mcl::findRomFolders(roots,4);
 assert(folders.size()==2&&mcl::gamesFound(folders)==8);
 auto r=mcl::autoConfigure(app,roots);
 assert(r.romsChanged&&r.mameChanged&&r.flycastChanged&&r.games==8);
 assert(mcl::readMameExecutable(app/"mame_executable.txt").filename()=="mame.exe");
 assert(mcl::readMameExecutable(app/"flycast_executable.txt").filename()=="flycast.exe");
 assert(mcl::readRomSearchPath(app/"rom_directory.txt").dirs.size()==2);
 // a working setup is left alone
 r=mcl::autoConfigure(app,roots);
 assert(!r.romsChanged&&!r.mameChanged&&!r.flycastChanged&&r.games==8);
 // a package extracted anywhere: emulator and ROMs inside the game folder are stored relative
 const fs::path pkg=root/"Extracted"/"MCL Package";
 touch(pkg/"Emulators"/"MAME 0.289"/"mame.exe");touch(pkg/"Emulators"/"Flycast 2.7"/"flycast.exe");
 for(const char* z:{"xmcotau.zip","mshud.zip","xmvsfur2.zip","mshvsfu1d.zip","mvscu.zip","sfexp.zip","coh1000c.zip","sfex2p.zip","coh3002c.zip","mvsc2.zip","naomi.zip"})touch(pkg/"ROMs"/z);
 r=mcl::autoConfigure(pkg,{pkg});
 assert(r.games==8&&r.mameChanged&&r.romsChanged);
 {std::ifstream f(pkg/"mame_executable.txt");std::string line;std::getline(f,line);assert(line.rfind("Emulators",0)==0);}
 const fs::path moved=root/"Somewhere Else"/"MCL";fs::create_directories(moved.parent_path());fs::rename(pkg,moved);
 assert(mcl::gamesFound(mcl::readRomSearchPath(moved/"rom_directory.txt").dirs)==8); // still works after moving
 assert(fs::is_regular_file(mcl::readMameExecutable(moved/"mame_executable.txt")));
 fs::remove_all(root);
 std::cout<<"PASS: finds ROM folders and emulators on a new PC, leaves working setups alone, relative inside the game folder\n";
}
