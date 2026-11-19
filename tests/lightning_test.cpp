#include "../src/lightning.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
 mcl::LightningField f(42);
 int maxLive=0;bool sawFront=false,sawFlash=false,sawBehind=false;
 for(double t=0;t<12000;t+=16.0){
  f.update(t);
  maxLive=std::max<int>(maxLive,static_cast<int>(f.bolts().size()));
  if(f.flash()>0.5f)sawFlash=true;
  for(const auto& b:f.bolts()){
   (b.front?sawFront:sawBehind)=true;
   assert(b.main.size()>=9);
   for(const auto& p:b.main){assert(std::isfinite(p.x)&&std::isfinite(p.y));assert(p.x>-600&&p.x<2200&&p.y>-500&&p.y<1300);}
   assert(t-b.born<=b.life);
   const float a=f.boltAlpha(b,t);assert(a>=0.f&&a<=1.f);
  }
  assert(f.flash()>=0.f&&f.flash()<=1.f);
 }
 assert(sawFront&&sawBehind&&sawFlash);
 assert(maxLive>=4&&maxLive<60);
 // Always busy: after warm-up there are at least minArcs live bolts every frame.
 {mcl::LightningField g(7);for(double t=0;t<3000;t+=16.0){g.update(t);if(t>100)assert(g.bolts().size()>=4);}}
 // Quiet config (menu highlight): no strikes, no front bolts, never flashes.
 mcl::LightningConfig quiet;quiet.strikes=false;quiet.frontBolts=false;quiet.minArcs=1;quiet.maxArcs=3;quiet.arcChance=0.25f;
 mcl::LightningField q(3,{100,50,80,20},quiet);
 for(double t=0;t<8000;t+=16.0){q.update(t);assert(q.flash()==0.f);for(const auto& b:q.bolts())assert(!b.front);}
 const auto c=q.crackle(500);assert(c.size()>24);
 std::cout<<"PASS: lightning bounds, lifetimes, front/behind bolts, flashes, quiet config, crackle\n";
}
