#include "../src/pause_menu_lua.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
namespace fs=std::filesystem;
int main(int argc,char** argv){
 const std::string lua=mcl::buildPauseMenuLua(R"(C:\Games\MCL\bin\MCL_UserData\game_signal.txt)");
 assert(lua.find("MCL_SIGNAL_FILE = [[C:\\Games\\MCL\\bin\\MCL_UserData\\game_signal.txt]]")!=std::string::npos);
 for(const char* must:{"emu.pause()","emu.unpause()","machine:exit()","START1","COIN1","ARE YOU SURE?","CONTINUE","EXIT GAME","MCL_TEST_PRESSED"})
  assert(lua.find(must)!=std::string::npos);
 // A path containing "]]" must not break out of the Lua string.
 const std::string tricky=mcl::buildPauseMenuLua("C:\\odd]]name\\s.txt");
 assert(tricky.find("[=[C:\\odd]]name\\s.txt]=]")!=std::string::npos);
 if(argc>1&&argv[1][0]!='\0'){
  const fs::path out=argv[1];fs::create_directories(out);
  std::ofstream(out/"pause_menu.lua")<<mcl::buildPauseMenuLua((out/"game_signal.txt").u8string());
 }
 std::cout<<"PASS: pause menu Lua generation, signal path quoting\n";
}
