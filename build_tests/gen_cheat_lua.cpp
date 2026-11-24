// Test helper: prints the cheat Lua the launcher would write for one choice cheat.
// usage: gen_cheat_lua <game> <cheat id> <choice number (1-based)>
#include "../src/cheats.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
int main(int argc,char** argv){
 if(argc<4)return 2;
 const int pick=std::atoi(argv[3]);
 for(const auto& c:mcl::cheatsFor(argv[1])){
  if(std::string(c.id)!=argv[2])continue;
  const auto& pokes=c.isChoice()?c.choices.at(static_cast<size_t>(pick-1)).pokes:c.pokes;
  std::cout<<mcl::buildCheatLua(argv[1],pokes);
  return 0;
 }
 return 1;
}
