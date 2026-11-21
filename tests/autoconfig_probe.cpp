// Debug helper (not a unit test): run autoConfigure on a real folder with the default roots.
#include "autoconfig.hpp"
#include <exception>
#include <iostream>
int main(int argc,char** argv){
 if(argc<2){std::cout<<"PASS: probe needs a folder argument (skipped)\n";return 0;}
 try{
  const auto r=mcl::autoConfigure(argv[1]);
  std::cout<<"games "<<r.games<<" roms "<<r.romsChanged<<" mame "<<r.mameChanged<<" fly "<<r.flycastChanged<<"\n";
 }catch(const std::exception& e){std::cout<<"EXCEPTION: "<<e.what()<<"\n";return 1;}
 return 0;
}
