#pragma once
#include <cstdint>
#include <random>
#include <vector>
namespace mcl {
// Procedural lightning (portable, no graphics API). Port of Design/Logo/lightning_preview.html.
// Coordinates are in whatever space the ring is given in (logo artwork pixels for the title,
// screen pixels for menu highlights); the renderer maps and draws the polylines.
struct LPoint {float x=0,y=0;};
struct Bolt {
 std::vector<LPoint> main;
 std::vector<std::vector<LPoint>> branches;
 double born=0, life=0;
 float width=1;
 bool front=false; // drawn over the logo instead of behind it
};
struct LightningRing {float cx=800,cy=392,rx=770,ry=340;};
struct LightningConfig {
 bool strikes=true;      // big bolts toward the centre + double screen flash
 bool frontBolts=true;   // bolts slashing across the front
 int minArcs=4, maxArcs=7;
 float arcChance=0.45f;  // per update when between min and max
 double strikeMinMs=1200, strikeMaxMs=2400;
 double frontMinMs=700, frontMaxMs=1600;
};
class LightningField {
public:
 explicit LightningField(std::uint32_t seed=1,LightningRing ring={},LightningConfig cfg={});
 void setRing(const LightningRing& r){ring_=r;}
 const LightningRing& ring() const {return ring_;}
 void clear(){bolts_.clear();}
 // Advance to absolute time `nowMs` (monotonic). Spawns and expires bolts, decays flash.
 void update(double nowMs);
 const std::vector<Bolt>& bolts() const {return bolts_;}
 // 0..1 full-screen flash intensity (renderer draws a pale blue overlay at ~35% of this).
 float flash() const {return flash_;}
 // Brightness for a bolt this frame (flicker x fade). 0 when expired.
 float boltAlpha(const Bolt& b,double nowMs);
 // Thin crackle that hugs the whole ring, re-jagged every call.
 std::vector<LPoint> crackle(double nowMs,float wobble=14.f);
private:
 float rnd(float a,float b);
 LPoint onRing(float t,float wobble) const;
 std::vector<LPoint> jag(LPoint a,LPoint b,float rough,int depth);
 std::size_t pick(std::size_t lo,std::size_t size,std::size_t hiMargin);
 void spawnArc(double now);
 void spawnStrike(double now);
 void spawnFront(double now);
 std::mt19937 rng_;
 LightningRing ring_;
 LightningConfig cfg_;
 std::vector<Bolt> bolts_;
 float flash_=0;
 double last_=0, nextStrike_=0, nextFront_=0, reflash_=0;
 bool started_=false;
};
}
