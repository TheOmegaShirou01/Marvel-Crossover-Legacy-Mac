// macOS / Linux build of gamepads.hpp: every controller SDL sees, as raw joystick buttons.
// MAME on these systems reads controllers through SDL too, so device N here is JOYCODE_N and
// button k is BUTTONk+1 in MAME, exactly what CONTROLS SETUP writes.
#ifndef _WIN32
#include "gamepads.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cstdlib>
namespace mcl {
struct Gamepads::Impl {
 std::vector<SDL_Joystick*> sticks;
 std::vector<PadInfo> infos;
 std::vector<std::uint64_t> capPrev;
 std::vector<int> capHatPrev,capAxisPrev;
 void close(){for(auto* j:sticks)if(j)SDL_CloseJoystick(j);sticks.clear();infos.clear();}
};
static std::wstring wide(const char* s){std::wstring w;if(s)for(;*s;++s)w+=static_cast<wchar_t>(static_cast<unsigned char>(*s));return w;}
Gamepads::Gamepads():p_(std::make_unique<Impl>()){}
Gamepads::~Gamepads(){p_->close();}
bool Gamepads::init(void*){
 if(!SDL_WasInit(SDL_INIT_GAMEPAD)&&!SDL_InitSubSystem(SDL_INIT_GAMEPAD))return false; // also starts joysticks
 rescan();
 return true;
}
void Gamepads::rescan(){
 p_->close();
 int n=0;
 SDL_JoystickID* ids=SDL_GetJoysticks(&n);
 for(int i=0;i<n&&ids;++i){
  SDL_Joystick* j=SDL_OpenJoystick(ids[i]);
  if(!j)continue;
  p_->sticks.push_back(j);
  PadInfo info;info.name=wide(SDL_GetJoystickName(j));
  info.vid=SDL_GetJoystickVendor(j);info.pid=SDL_GetJoystickProduct(j);
  p_->infos.push_back(info);
 }
 SDL_free(ids);
 resetCapture();
}
int Gamepads::count() const{return static_cast<int>(p_->sticks.size());}
const PadInfo& Gamepads::info(int i) const{static PadInfo none;return i>=0&&i<count()?p_->infos[static_cast<size_t>(i)]:none;}
bool Gamepads::poll(int i,PadSnapshot& out){
 if(i<0||i>=count())return false;
 SDL_Joystick* j=p_->sticks[static_cast<size_t>(i)];
 SDL_UpdateJoysticks();
 out={};
 const int nb=std::min(64,SDL_GetNumJoystickButtons(j));
 for(int b=0;b<nb;++b)if(SDL_GetJoystickButton(j,b))out.buttons|=std::uint64_t(1)<<b;
 auto axis=[&](int a){return SDL_GetNumJoystickAxes(j)>a?static_cast<long>(SDL_GetJoystickAxis(j,a))*1000/32767:0L;};
 out.lx=axis(0);out.ly=axis(1);out.lz=axis(2);out.lrz=axis(5);
 const Uint8 hat=SDL_GetNumJoystickHats(j)>0?SDL_GetJoystickHat(j,0):0;
 out.up=(hat&SDL_HAT_UP)||out.ly<-500;out.down=(hat&SDL_HAT_DOWN)||out.ly>500;
 out.left=(hat&SDL_HAT_LEFT)||out.lx<-500;out.right=(hat&SDL_HAT_RIGHT)||out.lx>500;
 return true;
}
void Gamepads::resetCapture(){
 SDL_UpdateJoysticks();
 const size_t n=p_->sticks.size();
 p_->capPrev.assign(n,0);p_->capHatPrev.assign(n,0);p_->capAxisPrev.assign(n,0);
 for(size_t i=0;i<n;++i){PadSnapshot s;if(poll(static_cast<int>(i),s))p_->capPrev[i]=s.buttons;} // held buttons don't count
}
std::string Gamepads::capturePress(){
 for(size_t i=0;i<p_->sticks.size();++i){
  PadSnapshot s;if(!poll(static_cast<int>(i),s))continue;
  const std::uint64_t pressed=s.buttons&~p_->capPrev[i];p_->capPrev[i]=s.buttons;
  const std::string pad="JOYCODE_"+std::to_string(i+1)+"_";
  for(int b=0;b<64;++b)if(pressed&(std::uint64_t(1)<<b))return pad+"BUTTON"+std::to_string(b+1);
  SDL_Joystick* j=p_->sticks[i];
  const int hat=SDL_GetNumJoystickHats(j)>0?SDL_GetJoystickHat(j,0):0;
  const int newHat=hat&~p_->capHatPrev[i];p_->capHatPrev[i]=hat;
  if(newHat&SDL_HAT_UP)return pad+"HAT1UP";if(newHat&SDL_HAT_DOWN)return pad+"HAT1DOWN";
  if(newHat&SDL_HAT_LEFT)return pad+"HAT1LEFT";if(newHat&SDL_HAT_RIGHT)return pad+"HAT1RIGHT";
  // analog triggers (axes 4/5 on most pads) as switches
  int ax=0;if(s.lz>600)ax=1;else if(s.lrz>600)ax=2;
  const int was=p_->capAxisPrev[i];p_->capAxisPrev[i]=ax;
  if(ax&&ax!=was)return pad+(ax==1?"ZAXIS_POS_SWITCH":"RZAXIS_POS_SWITCH");
 }
 return {};
}
// Raw SDL joystick numbering (used only when the player picked menu buttons themselves).
MenuButtons defaultMenuButtons(unsigned vid,unsigned pid){(void)vid;(void)pid;return {0,1,8,9};}
}
#endif
