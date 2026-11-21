#ifdef _WIN32
#define DIRECTINPUT_VERSION 0x0800
#include "gamepads.hpp"
#include <dinput.h>
#include <wrl/client.h>
#include <algorithm>
#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")
namespace mcl {
using Microsoft::WRL::ComPtr;
struct Gamepads::Impl {
 HWND window=nullptr;
 ComPtr<IDirectInput8W> di;
 struct Dev { ComPtr<IDirectInputDevice8W> dev; PadInfo info; GUID guid{}; PadSnapshot last; bool haveLast=false; };
 std::vector<Dev> devs;
 static BOOL CALLBACK enumCb(const DIDEVICEINSTANCEW* inst,void* ctx){
  auto* self=static_cast<Impl*>(ctx);
  Dev d;d.guid=inst->guidInstance;d.info.name=inst->tszProductName;
  if(FAILED(self->di->CreateDevice(inst->guidInstance,&d.dev,nullptr)))return DIENUM_CONTINUE;
  if(FAILED(d.dev->SetDataFormat(&c_dfDIJoystick2)))return DIENUM_CONTINUE;
  d.dev->SetCooperativeLevel(self->window,DISCL_BACKGROUND|DISCL_NONEXCLUSIVE);
  DIPROPDWORD vp{};vp.diph.dwSize=sizeof vp;vp.diph.dwHeaderSize=sizeof(DIPROPHEADER);vp.diph.dwHow=DIPH_DEVICE;
  if(SUCCEEDED(d.dev->GetProperty(DIPROP_VIDPID,&vp.diph))){d.info.vid=LOWORD(vp.dwData);d.info.pid=HIWORD(vp.dwData);}
  DIPROPGUIDANDPATH gp{};gp.diph.dwSize=sizeof gp;gp.diph.dwHeaderSize=sizeof(DIPROPHEADER);gp.diph.dwHow=DIPH_DEVICE;
  if(SUCCEEDED(d.dev->GetProperty(DIPROP_GUIDANDPATH,&gp.diph))){
   std::wstring path=gp.wszPath;std::transform(path.begin(),path.end(),path.begin(),::towupper);
   d.info.isXInput=path.find(L"IG_")!=std::wstring::npos; // XInput-compatible devices carry "IG_" in their path
  }
  // axes to -1000..1000
  DIPROPRANGE r{};r.diph.dwSize=sizeof r;r.diph.dwHeaderSize=sizeof(DIPROPHEADER);r.diph.dwHow=DIPH_DEVICE;r.lMin=-1000;r.lMax=1000;
  d.dev->SetProperty(DIPROP_RANGE,&r.diph);
  d.dev->Acquire();
  self->devs.push_back(std::move(d));
  return DIENUM_CONTINUE;
 }
 void enumerate(){
  devs.clear();
  if(di)di->EnumDevices(DI8DEVCLASS_GAMECTRL,enumCb,this,DIEDFL_ATTACHEDONLY);
 }
 bool read(Dev& d,PadSnapshot& s){
  DIJOYSTATE2 js{};
  if(FAILED(d.dev->Poll())){d.dev->Acquire();d.dev->Poll();}
  if(FAILED(d.dev->GetDeviceState(sizeof js,&js))){if(FAILED(d.dev->Acquire()))return false;if(FAILED(d.dev->GetDeviceState(sizeof js,&js)))return false;}
  s=PadSnapshot{};
  s.lx=js.lX;s.ly=js.lY;s.lz=js.lZ;s.lrz=js.lRz;s.pov=js.rgdwPOV[0];
  for(int k=0;k<64&&k<128;++k)if(js.rgbButtons[k]&0x80)s.buttons|=(std::uint64_t(1)<<k);
  const bool centered=LOWORD(s.pov)==0xFFFF;
  const DWORD a=s.pov;
  if(!centered){
   s.up=(a>=31500||a<=4500);s.right=(a>=4500&&a<=13500);s.down=(a>=13500&&a<=22500);s.left=(a>=22500&&a<=31500);
  }
  s.up=s.up||s.ly<-600;s.down=s.down||s.ly>600;s.left=s.left||s.lx<-600;s.right=s.right||s.lx>600;
  return true;
 }
};
Gamepads::Gamepads():p_(new Impl){}
Gamepads::~Gamepads()=default;
bool Gamepads::init(HWND window){
 p_->window=window;
 if(FAILED(DirectInput8Create(GetModuleHandleW(nullptr),DIRECTINPUT_VERSION,IID_IDirectInput8W,reinterpret_cast<void**>(p_->di.GetAddressOf()),nullptr)))return false;
 p_->enumerate();
 return true;
}
void Gamepads::rescan(){p_->enumerate();}
int Gamepads::count() const{return static_cast<int>(p_->devs.size());}
const PadInfo& Gamepads::info(int i) const{return p_->devs[static_cast<size_t>(i)].info;}
bool Gamepads::poll(int i,PadSnapshot& out){
 if(i<0||i>=count())return false;
 return p_->read(p_->devs[static_cast<size_t>(i)],out);
}
void Gamepads::resetCapture(){for(auto& d:p_->devs){PadSnapshot s;d.haveLast=p_->read(d,s);d.last=s;}}
std::string Gamepads::capturePress(){
 for(size_t i=0;i<p_->devs.size();++i){
  auto& d=p_->devs[i];PadSnapshot s;
  if(!p_->read(d,s))continue;
  if(!d.haveLast){d.last=s;d.haveLast=true;continue;}
  const std::string pre="JOYCODE_"+std::to_string(i+1)+"_";
  std::string hit;
  const std::uint64_t pressed=s.buttons&~d.last.buttons;
  for(int k=0;k<64&&hit.empty();++k)if(pressed&(std::uint64_t(1)<<k))hit=pre+"BUTTON"+std::to_string(k+1);
  const bool povNow=LOWORD(s.pov)!=0xFFFF,povWas=LOWORD(d.last.pov)!=0xFFFF;
  if(hit.empty()&&povNow&&!povWas){
   const DWORD a=s.pov;
   hit=pre+(a>=31500||a<4500?"HAT1UP":a<13500?"HAT1RIGHT":a<22500?"HAT1DOWN":"HAT1LEFT");
  }
  const auto axis=[&](long now,long was,const char* neg,const char* pos){
   if(!hit.empty())return;
   if(now<-700&&was>=-700)hit=pre+neg;else if(now>700&&was<=700)hit=pre+pos;
  };
  axis(s.ly,d.last.ly,"YAXIS_UP_SWITCH","YAXIS_DOWN_SWITCH");
  axis(s.lx,d.last.lx,"XAXIS_LEFT_SWITCH","XAXIS_RIGHT_SWITCH");
  axis(s.lz,d.last.lz,"ZAXIS_NEG_SWITCH","ZAXIS_POS_SWITCH");
  axis(s.lrz,d.last.lrz,"RZAXIS_NEG_SWITCH","RZAXIS_POS_SWITCH");
  d.last=s;
  if(!hit.empty())return hit;
 }
 return {};
}
MenuButtons defaultMenuButtons(unsigned vid,unsigned pid){
 (void)pid;
 // DirectInput button numbers (0-based).
 if(vid==0x054C)return {1,2,8,9};   // Sony DualShock 3/4, DualSense: Cross, Circle, Share/Create, Options
 if(vid==0x057E)return {1,0,8,9};   // Nintendo Switch Pro / Joy-Con: A, B, Minus, Plus
 if(vid==0x045E)return {0,1,6,7};   // Microsoft (XInput pads seen through DirectInput): A, B, View, Menu
 return {0,1,8,9};                  // generic pads: first two face buttons, then select/start
}
}
#endif
