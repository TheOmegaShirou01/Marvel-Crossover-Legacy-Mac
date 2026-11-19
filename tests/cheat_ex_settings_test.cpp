#include "../src/cheat_ex_settings.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
int main(){
 namespace fs=std::filesystem;
 const auto root=fs::temp_directory_path()/"mcl_ex_settings_test_036";
 fs::remove_all(root);
 mcl::ExCheatSettings s(root/"settings.dat");
 assert(!s.load());
 for(const auto* id:{"sfexp","sfex2p"})for(bool b:s.options(id).flags)assert(!b);
 s.options("sfexp").set(mcl::ExCheat::Health,true);
 s.options("sfexp").set(mcl::ExCheat::SecretCharacters,true);
 s.options("sfex2p").set(mcl::ExCheat::Power,true);
 s.options("sfex2p").set(mcl::ExCheat::Time,true);
 assert(s.save());
 mcl::ExCheatSettings again(root/"settings.dat");assert(again.load());
 assert(again.options("sfexp").get(mcl::ExCheat::Health));
 assert(!again.options("sfex2p").get(mcl::ExCheat::Health));
 assert(again.options("sfexp").get(mcl::ExCheat::SecretCharacters));
 assert(again.options("sfex2p").get(mcl::ExCheat::Time));
 assert(again.options("sfex2p").get(mcl::ExCheat::Power));
 bool threw=false;try{again.options("sfex3");}catch(const std::invalid_argument&){threw=true;}assert(threw);
 {std::ofstream o(root/"settings.dat",std::ios::trunc);o<<"bad settings\n";}
 assert(!again.load());
 for(const auto* id:{"sfexp","sfex2p"})for(bool b:again.options(id).flags)assert(!b);
 fs::remove_all(root);
 std::cout<<"PASS: EX settings persist per game, default off, invalid data rejected, EX3 excluded\n";
}
