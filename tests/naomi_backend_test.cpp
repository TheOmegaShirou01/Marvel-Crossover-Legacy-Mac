#include "../src/mame_backend.hpp"
#include "../src/external_roms.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
namespace fs=std::filesystem;
int main(){
 auto base=fs::temp_directory_path()/"mcl_naomi_test_0310";fs::remove_all(base);
 fs::create_directories(base/"Runtime"/"ROMs");
 auto rom=base/"Runtime"/"ROMs";auto exe=base/"mame.exe";
 {std::ofstream f(exe);f<<"placeholder";}
 {std::ofstream f(rom/"mvsc2.zip");f<<"PK\003\004data";}
 mcl::MameLaunchSpec spec;std::string why;
 assert(!mcl::createNaomiLaunchSpec(exe,rom,spec,why));
 assert(why=="NAOMI BIOS ZIP missing");
 {std::ofstream f(rom/"naomi.zip");f<<"PK\003\004data";}
 assert(mcl::createNaomiLaunchSpec(exe,rom,spec,why));
 assert(spec.game=="mvsc2" && spec.arguments.size()==4&&spec.arguments[3]==L"-skip_gameinfo");
 assert(spec.arguments[0]==L"mvsc2" && spec.arguments[1]==L"-rompath");
 assert(mcl::resolveRuntimeRomDirectory(base)==rom);
 {std::ofstream f(base/"rom_directory.txt");f<<"custom-roms\n";}
 assert(mcl::resolveRuntimeRomDirectory(base)==base/"custom-roms");
 fs::remove_all(base);
 std::cout<<"PASS: NAOMI BIOS/ROM requirements and runtime ROM path resolution\n";
}
