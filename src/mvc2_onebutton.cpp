#include "mvc2_onebutton.hpp"
namespace mcl {
namespace {
constexpr std::uint32_t kSlots=0x2D7088,kSlotSize=0x5A4,kTimer=0x2F8378;
constexpr double kStepMs=34; // two frames per motion step
// Windows set-1 scancodes / SDL scancodes of keys no ordinary keyboard has. F13-F20 for player 1,
// F21-F24 and international keys for player 2 (tested: F13 and INTERNATIONAL1 reach Flycast).
const InjectKey kKeys[2][8]={
 {{0x7C,0x64,104},{0x7D,0x65,105},{0x7E,0x66,106},{0x7F,0x67,107},{0x80,0x68,108},{0x81,0x69,109},{0x82,0x6A,110},{0x83,0x6B,111}},
 {{0x84,0x6C,112},{0x85,0x6D,113},{0x86,0x6E,114},{0x87,0x76,115},{0xC1,0x73,135},{0xE9,0x70,136},{0xEA,0x7D,137},{0xEB,0x79,138}},
};
enum { Up=1, Down=2, Left=4, Right=8, LP=16, HP=32, LK=64, HK=128 };
}
const InjectKey& mvc2InjectKey(int player,int function){return kKeys[player&1][function&7];}
const char* mvc2InjectOption(int function){
 static const char* const n[8]={"btn_dpad1_up","btn_dpad1_down","btn_dpad1_left","btn_dpad1_right","btn_a","btn_b","btn_x","btn_y"};
 return n[function&7];
}
unsigned mvc2Takeover(bool specials,bool supers){return (specials?(kLP|kLK):0u)|(supers?(kHP|kHK):0u);}
void Mvc2OneButton::reset(){for(auto& p:p_)p=Player{};lastTimer_=-1;timerMoved_=-1e9;}
void Mvc2OneButton::apply(int pl,unsigned keys,const PostKey& post){
 Player& P=p_[pl];
 const unsigned change=keys^P.down;
 for(int f=0;f<8;++f)if(change&(1u<<f))post(kKeys[pl][f],(keys>>f)&1);
 P.down=keys;
}
void Mvc2OneButton::releaseAll(const PostKey& post){
 for(int pl=0;pl<2;++pl){apply(pl,0,post);p_[pl].queue.clear();p_[pl].at=0;}
}
bool Mvc2OneButton::inFight(double now,const ReadRam& read){
 std::uint8_t t=0;if(!read(kTimer,&t,1))return false;
 if(t!=lastTimer_){lastTimer_=t;timerMoved_=now;}
 bool alive[2]={false,false};
 for(int s=0;s<6;++s){
  std::uint8_t hp=0;if(!read(kSlots+s*kSlotSize+0x420,&hp,1))return false;
  if(hp>0&&hp<=144)alive[s&1]=true;
 }
 return alive[0]&&alive[1]&&now-timerMoved_<3000;
}
bool Mvc2OneButton::facingRight(int pl,const ReadRam& read){
 std::uint32_t pick=kSlots+pl*kSlotSize;
 for(int k=0;k<3;++k){
  const std::uint32_t slot=kSlots+(pl+2*k)*kSlotSize;
  std::uint8_t on=0;if(read(slot,&on,1)&&(on&1)){pick=slot;break;}
 }
 std::uint8_t f=0;read(pick+0x110,&f,1);
 return (f&1)!=0;
}
void Mvc2OneButton::tick(double now,const unsigned held[2],const ReadRam& read,const PostKey& post){
 if(!takeover_)return;
 const bool fighting=inFight(now,read);
 for(int pl=0;pl<2;++pl){
  Player& P=p_[pl];
  const unsigned h=held[pl]&takeover_;
  const unsigned pressed=h&~P.prev;
  P.prev=h;
  if(P.at<P.queue.size()){ // a move is being typed
   while(P.at<P.queue.size()&&now>=P.queue[P.at].until)++P.at;
   apply(pl,P.at<P.queue.size()?P.queue[P.at].keys:0u,post);
   continue;
  }
  P.queue.clear();P.at=0;
  if(fighting&&pressed){
   const bool right=facingRight(pl,read);
   const unsigned F=right?Right:Left,B=right?Left:Right;
   std::vector<unsigned> seq;unsigned btn=0;
   if(pressed&kLP){seq={Down,Down|F,F};btn=LP;}
   else if(pressed&kLK){seq={Down,Down|B,B};btn=LK;}
   else if(pressed&kHP){seq={Down,Down|F,F};btn=LP|HP;}
   else if(pressed&kHK){seq={Down,Down|F,F};btn=LK|HK;}
   double t=now;
   for(std::size_t i=0;i<seq.size();++i){
    const bool last=i+1==seq.size();
    t+=last?kStepMs*2:kStepMs;
    P.queue.push_back({seq[i]|(last?btn:0u),t});
   }
   P.queue.push_back({0u,t+kStepMs}); // let go
   P.at=0;
   apply(pl,P.queue[0].keys,post);
   continue;
  }
  // Outside a fight (menus, select screens) the taken-over buttons pass straight through;
  // in a fight they only ever start moves.
  unsigned keys=0;
  if(!fighting){if(h&kLP)keys|=LP;if(h&kHP)keys|=HP;if(h&kLK)keys|=LK;if(h&kHK)keys|=HK;}
  apply(pl,keys,post);
 }
}
}
