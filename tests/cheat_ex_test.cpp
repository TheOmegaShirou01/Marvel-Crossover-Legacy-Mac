#include "../src/cheat_ex_code.hpp"
#include <cassert>
#include <iostream>
int main(){
 mcl::CheatExCode c;
 for(int k:{40,39,40,37,88,67}){assert(!c.feed(k));assert(!c.unlocked());}
 assert(c.feed(13));assert(c.unlocked());assert(!c.feed(13));
 c.reset();assert(!c.unlocked());
 for(int k:{40,39,40,37,88,65,13})assert(!c.feed(k));
 assert(!c.unlocked());
 for(int k:{40,39,40,37,88,67,13})c.feed(k);
 assert(c.unlocked());std::cout<<"PASS CHEAT EX unlock and reset\n";
}
