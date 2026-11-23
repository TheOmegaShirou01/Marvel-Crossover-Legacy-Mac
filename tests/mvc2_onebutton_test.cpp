#include "../src/mvc2_onebutton.hpp"
#include "../src/controls.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <vector>
using namespace mcl;
// Fake NAOMI RAM: timer, slot "on screen" flags, facing bits and health.
struct Ram {
 std::map<std::uint32_t,std::uint8_t> b;
 bool read(std::uint32_t off,void* out,std::size_t n){for(std::size_t i=0;i<n;++i){auto it=b.find(off+static_cast<std::uint32_t>(i));static_cast<std::uint8_t*>(out)[i]=it==b.end()?0:it->second;}return true;}
};
constexpr std::uint32_t kSlots=0x2D7088,kSize=0x5A4;
static void fighters(Ram& r,bool p1Right){
 for(int s=0;s<6;++s){r.b[kSlots+s*kSize+0x420]=144;r.b[kSlots+s*kSize]=s<2?1:0;}
 r.b[kSlots+0x110]=p1Right?1:0;              // P1 point
 r.b[kSlots+kSize+0x110]=p1Right?0:1;        // P2 point
}
int main(){
 Ram ram;std::vector<std::pair<int,bool>> events; // (sdl scancode, down)
 const auto post=[&](const InjectKey& k,bool down){events.push_back({k.sdl,down});};
 const auto read=[&](std::uint32_t o,void* p,std::size_t n){return ram.read(o,p,n);};
 const int up=mvc2InjectKey(0,0).sdl,down=mvc2InjectKey(0,1).sdl,left=mvc2InjectKey(0,2).sdl,right=mvc2InjectKey(0,3).sdl,lp=mvc2InjectKey(0,4).sdl,hp=mvc2InjectKey(0,5).sdl;
 (void)up;
 assert(mvc2Takeover(true,false)==(kLP|kLK)&&mvc2Takeover(false,true)==(kHP|kHK)&&mvc2Takeover(false,false)==0);
 Mvc2OneButton e;e.configure(true,true);
 // 1) not in a fight (timer never moves): LP passes straight through
 fighters(ram,true);ram.b[0x2F8378]=0x99;
 unsigned none[2]={0,0},lpOnly[2]={kLP,0};
 e.tick(0,none,read,post);
 e.tick(5000,lpOnly,read,post);
 assert(events.size()==1&&events[0]==std::make_pair(lp,true));
 e.tick(5010,none,read,post);
 assert(events.size()==2&&events[1]==std::make_pair(lp,false));
 // 2) in a fight (timer moves), facing right: LP types down, down+right, right+LP, then lets go
 events.clear();
 ram.b[0x2F8378]=0x98;e.tick(6000,none,read,post);
 double t=6100;e.tick(t,lpOnly,read,post);
 for(int i=0;i<20;++i){t+=10;e.tick(t,lpOnly,read,post);}
 std::vector<std::pair<int,bool>> want={{down,true},{right,true},{down,false},{lp,true},{right,false},{lp,false}};
 assert(events==want);
 // holding LP after the move does not leak a plain LP
 events.clear();for(int i=0;i<10;++i){t+=10;e.tick(t,lpOnly,read,post);}assert(events.empty());
 e.tick(t+=10,none,read,post);
 // 3) facing left: LP's forward is left
 events.clear();fighters(ram,false);ram.b[0x2F8378]=0x97;
 e.tick(t+=10,lpOnly,read,post);for(int i=0;i<20;++i)e.tick(t+=10,lpOnly,read,post);
 assert(events.size()==6&&events[1]==std::make_pair(left,true));
 // 4) HP = hyper: QCF + LP + HP together
 events.clear();e.tick(t+=10,none,read,post);fighters(ram,true);ram.b[0x2F8378]=0x96;
 unsigned hpOnly[2]={kHP,0};
 e.tick(t+=10,hpOnly,read,post);for(int i=0;i<20;++i)e.tick(t+=10,hpOnly,read,post);
 bool sawLp=false,sawHp=false;for(const auto& ev:events){if(ev.first==lp&&ev.second)sawLp=true;if(ev.first==hp&&ev.second)sawHp=true;}
 assert(sawLp&&sawHp);
 // 5) Flycast files: taken-over buttons are left out, spare keys are bound
 ControlsConfig m(ControlsConfig::Layout::Mvc2);
 const std::string kb=flycastKeyboardCfg(m,mvc2Takeover(true,true));
 assert(kb.find("224:btn_a\n")==std::string::npos&&kb.find("104:btn_dpad1_up\n")!=std::string::npos&&kb.find("108:btn_a\n")!=std::string::npos);
 assert(kb.find("135:btn_a1\n")!=std::string::npos&&kb.find("44:btn_c\n")!=std::string::npos); // P2 spare LP; assist 1 still on Space
 const std::string pad=flycastPadCfg(m,'r',mvc2Takeover(true,false));
 assert(pad.find("2:btn_a\n")==std::string::npos&&pad.find("0:btn_x\n")==std::string::npos&&pad.find("1:btn_y\n")!=std::string::npos);
 std::cout<<"PASS: MvC2 1-button moves (pass-through, QCF/QCB by facing, hyper, Flycast files)\n";
}
