#include "lightning.hpp"
#include <algorithm>
#include <cmath>
namespace mcl {
static constexpr float kPi=3.14159265358979f;
LightningField::LightningField(std::uint32_t seed,LightningRing ring,LightningConfig cfg)
 :rng_(seed),ring_(ring),cfg_(cfg){}
float LightningField::rnd(float a,float b){std::uniform_real_distribution<float> d(a,b);return d(rng_);}
LPoint LightningField::onRing(float t,float w) const {
 return {ring_.cx+(ring_.rx+w)*std::cos(t),ring_.cy+(ring_.ry+w*0.45f)*std::sin(t)};
}
std::size_t LightningField::pick(std::size_t lo,std::size_t size,std::size_t hiMargin){
 if(size<=lo+hiMargin+1)return size/2;
 std::uniform_int_distribution<std::size_t> d(lo,size-hiMargin-1);return d(rng_);
}
// Midpoint displacement: each pass splits every segment and pushes the midpoint sideways.
std::vector<LPoint> LightningField::jag(LPoint a,LPoint b,float rough,int depth){
 std::vector<LPoint> pts{a,b};
 for(int d=0;d<depth;++d){
  std::vector<LPoint> out;out.reserve(pts.size()*2);out.push_back(pts[0]);
  for(std::size_t i=1;i<pts.size();++i){
   const LPoint p=pts[i-1],q=pts[i];
   const float dx=q.x-p.x,dy=q.y-p.y;float len=std::hypot(dx,dy);if(len<1e-3f)len=1e-3f;
   const float off=rnd(-1.f,1.f)*rough*len;
   out.push_back({(p.x+q.x)*0.5f-dy/len*off,(p.y+q.y)*0.5f+dx/len*off});
   out.push_back(q);
  }
  pts.swap(out);
 }
 return pts;
}
static void appendTail(std::vector<LPoint>& dst,const std::vector<LPoint>& src){
 if(src.size()>1)dst.insert(dst.end(),src.begin()+1,src.end());
}
void LightningField::spawnArc(double now){
 const float t=rnd(0,2*kPi),span=rnd(0.45f,1.1f)*(rnd(0,1)<0.5f?-1.f:1.f);
 const int n=7;std::vector<LPoint> anchors;
 for(int i=0;i<=n;++i)anchors.push_back(onRing(t+span*i/n,rnd(-28,28)));
 Bolt b;b.main.push_back(anchors[0]);
 for(int i=1;i<=n;++i)appendTail(b.main,jag(anchors[i-1],anchors[i],0.32f,4));
 const int nb=rnd(0,1)<0.7f?static_cast<int>(rnd(1,4)):0;
 for(int k=0;k<nb;++k){
  const LPoint s=b.main[pick(2,b.main.size(),2)];
  const float ang=std::atan2((s.y-ring_.cy)/ring_.ry,(s.x-ring_.cx)/ring_.rx)+rnd(-0.6f,0.6f);
  const float L=rnd(40,140);
  b.branches.push_back(jag(s,{s.x+std::cos(ang)*L*1.4f,s.y+std::sin(ang)*L},0.35f,3));
 }
 b.born=now;b.life=rnd(120,380);b.width=rnd(1.1f,1.7f);b.front=false;
 bolts_.push_back(std::move(b));
}
void LightningField::spawnStrike(double now){
 const float t=rnd(0,2*kPi);
 const LPoint a=onRing(t,rnd(30,90));
 const LPoint e{ring_.cx+std::cos(t)*ring_.rx*rnd(0.49f,0.68f),ring_.cy+std::sin(t)*ring_.ry*rnd(0.32f,0.5f)};
 Bolt b;b.main=jag(a,e,0.28f,6);
 for(int k=0;k<4;++k){
  const LPoint s=b.main[pick(4,b.main.size(),4)];
  b.branches.push_back(jag(s,{s.x+ring_.rx*rnd(-0.21f,0.21f),s.y+ring_.ry*rnd(-0.32f,0.32f)},0.35f,4));
 }
 b.born=now;b.life=rnd(160,320);b.width=1.7f;b.front=rnd(0,1)<0.4f;
 bolts_.push_back(std::move(b));
 flash_=1.f;reflash_=now+rnd(70,140);
}
void LightningField::spawnFront(double now){
 const bool left=rnd(0,1)<0.5f;
 const float t1=(left?kPi:0.f)+rnd(-0.55f,0.55f),t2=(left?0.f:kPi)+rnd(-0.55f,0.55f);
 const LPoint a=onRing(t1,rnd(-20,30)),e=onRing(t2,rnd(-20,30));
 const LPoint mid{ring_.cx+ring_.rx*rnd(-0.23f,0.23f),ring_.cy+ring_.ry*rnd(-0.5f,0.5f)};
 Bolt b;b.main=jag(a,mid,0.2f,5);appendTail(b.main,jag(mid,e,0.2f,5));
 const int n=static_cast<int>(rnd(2,5));
 for(int k=0;k<n;++k){
  const LPoint s=b.main[pick(3,b.main.size(),3)];
  b.branches.push_back(jag(s,{s.x+ring_.rx*rnd(-0.18f,0.18f),s.y+ring_.ry*rnd(-0.35f,0.35f)},0.35f,3));
 }
 b.born=now;b.life=rnd(90,220);b.width=1.35f;b.front=true;
 bolts_.push_back(std::move(b));
 flash_=std::max(flash_,0.55f);
}
void LightningField::update(double now){
 if(!started_){
  started_=true;last_=now;
  nextStrike_=now+cfg_.strikeMinMs;nextFront_=now+cfg_.frontMinMs*0.85;
 }
 const double dt=std::max(0.0,now-last_);last_=now;
 flash_=std::max(0.f,flash_-static_cast<float>(dt*0.006));
 bolts_.erase(std::remove_if(bolts_.begin(),bolts_.end(),[&](const Bolt& b){return now-b.born>b.life;}),bolts_.end());
 const int live=static_cast<int>(bolts_.size());
 if(live<cfg_.minArcs||rnd(0,1)<cfg_.arcChance)spawnArc(now);
 if(live<cfg_.maxArcs&&rnd(0,1)<cfg_.arcChance*0.78f)spawnArc(now);
 if(cfg_.strikes&&now>=nextStrike_){spawnStrike(now);nextStrike_=now+rnd(static_cast<float>(cfg_.strikeMinMs),static_cast<float>(cfg_.strikeMaxMs));}
 if(cfg_.frontBolts&&now>=nextFront_){spawnFront(now);nextFront_=now+rnd(static_cast<float>(cfg_.frontMinMs),static_cast<float>(cfg_.frontMaxMs));}
 if(reflash_>0&&now>=reflash_){flash_=std::max(flash_,0.8f);reflash_=0;}
}
float LightningField::boltAlpha(const Bolt& b,double now){
 const double age=(now-b.born)/b.life;
 if(age>1.0||age<0.0)return 0.f;
 const float flick=rnd(0,1)<0.18f?0.25f:1.f;
 return flick*static_cast<float>(1.0-age*0.6);
}
std::vector<LPoint> LightningField::crackle(double now,float wobble){
 const int segs=24;const float base=static_cast<float>(now/900.0);
 std::vector<LPoint> ringPts;
 for(int i=0;i<=segs;++i)ringPts.push_back(onRing(base+i*2*kPi/segs,rnd(-wobble,wobble)));
 std::vector<LPoint> out{ringPts[0]};
 for(int i=1;i<=segs;++i)appendTail(out,jag(ringPts[i-1],ringPts[i],0.22f,2));
 return out;
}
}
