// Live check of the MvC2 cheat path: starts MvC2 through MameBackend (as the app does), writes the
// chosen cheats' pokes with writeNaomi every frame (like GameLauncher::applyFlycastCheats), presses
// START twice to reach the select screen, waits, then stops Flycast.
// usage: mvc2_cheat_live <bin dir> <seconds> <cheat id> [cheat id ...]
#include "../src/mame_backend.hpp"
#include "../src/external_roms.hpp"
#include "../src/cheats.hpp"
#include <windows.h>
#include <cstdlib>
#include <iostream>
#include <string>
int main(int argc,char** argv){
 if(argc<4)return 2;
 const std::filesystem::path bin=argv[1];const int secs=std::atoi(argv[2]);
 std::vector<mcl::Poke> pokes;
 for(int i=3;i<argc;++i)for(const auto& c:mcl::cheatsFor("mvsc2"))if(std::string(c.id)==argv[i])pokes.insert(pokes.end(),c.pokes.begin(),c.pokes.end());
 mcl::MameBackend b(mcl::readMameExecutable(bin/"mame_executable.txt"));
 b.setFlycastExecutable(mcl::readMameExecutable(bin/"flycast_executable.txt"));
 b.setWindowed(true);
 if(!b.start("mvsc2",mcl::resolveRuntimeRomSearchPath(bin))){std::cerr<<"start failed\n";return 3;}
 const DWORD t0=GetTickCount();int writes=0,fails=0;bool k1=false,k2=false;
 while(b.running()&&GetTickCount()-t0<static_cast<DWORD>(secs)*1000){
  for(const auto& p:pokes){const std::uint32_t v=p.value;if(b.writeNaomi(p.addr,&v,p.width==4?4:1))++writes;else ++fails;}
  const DWORD el=GetTickCount()-t0;
  if(!k1&&el>40000){b.postKey('1',0x02,true);Sleep(100);b.postKey('1',0x02,false);k1=true;}
  if(!k2&&el>42500){b.postKey('1',0x02,true);Sleep(100);b.postKey('1',0x02,false);k2=true;}
  Sleep(16);
 }
 std::uint32_t m[4]{};b.readNaomi(0x2D6FB8,m,sizeof m);
 std::printf("writes %d fails %d masks %08X %08X %08X %08X\n",writes,fails,m[0],m[1],m[2],m[3]);
 std::cout.flush();
 return 0; // Flycast is left running for the screenshot; the caller kills it
}
