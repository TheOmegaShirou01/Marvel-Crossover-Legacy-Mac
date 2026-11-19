#ifdef _WIN32
#include "title_screen.hpp"
#include "earth_video.hpp"
#include "lightning.hpp"
#include "menu_model.hpp"
#include "cheats.hpp"
#include <memory>
#include <d2d1_1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <Xinput.h>
#include <mmsystem.h>
#include <wrl/client.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>
#include <vector>
#pragma comment(lib,"d2d1.lib")
#pragma comment(lib,"dwrite.lib")
#pragma comment(lib,"windowscodecs.lib")
#pragma comment(lib,"xinput.lib")
#pragma comment(lib,"winmm.lib")
#pragma comment(lib,"ole32.lib")
using Microsoft::WRL::ComPtr;
namespace mcl_title {
namespace fs=std::filesystem;
using mcl::Pad;
using mcl::MenuItem;
using MK=mcl::MainMenuModel::Kind;

constexpr float kLogoW=1600.f,kLogoH=760.f;
enum class Screen { Title, Menu, Credits, Cheats };

const wchar_t* const kCredits[]={
 L"#MARVEL: CROSSOVER LEGACY",
 L"An unofficial, non-commercial fan collection",
 L"",
 L"#THE GAMES",
 L"X-Men: Children of the Atom  -  Capcom, 1994",
 L"Marvel Super Heroes  -  Capcom, 1995",
 L"X-Men vs. Street Fighter  -  Capcom, 1996",
 L"Marvel Super Heroes vs. Street Fighter  -  Capcom, 1997",
 L"Marvel vs. Capcom: Clash of Super Heroes  -  Capcom, 1998",
 L"Marvel vs. Capcom 2: New Age of Heroes  -  Capcom, 2000",
 L"Street Fighter EX Plus  -  Arika / Capcom, 1997",
 L"Street Fighter EX2 Plus  -  Arika / Capcom, 1999",
 L"",
 L"#EMULATION",
 L"MAME  -  the Multiple Arcade Machine Emulator",
 L"Every game runs unmodified from your own original arcade ROMs",
 L"",
 L"#LEGAL",
 L"Marvel characters are trademarks of Marvel.",
 L"Capcom characters and games are trademarks of Capcom.",
 L"This collection is not affiliated with or endorsed by either company.",
 L"",
 L"",
 L"#THE LEGENDS LIVE ON!",
};

struct App {
 HWND hwnd=nullptr;HINSTANCE inst=nullptr;fs::path dir;Callbacks cb;
 ComPtr<ID2D1Factory1> factory;ComPtr<ID2D1HwndRenderTarget> rt;ComPtr<ID2D1DeviceContext> dc;
 ComPtr<IDWriteFactory> dw;ComPtr<IWICImagingFactory> wic;
 ComPtr<IDWriteTextFormat> fPress,fMenu,fSmall,fBody,fHead,fBanner;
 ComPtr<ID2D1SolidColorBrush> brush;ComPtr<ID2D1StrokeStyle> round;
 ComPtr<ID2D1RadialGradientBrush> fallbackBg;
 ComPtr<ID2D1Bitmap> logo,videoBmp;
 mcl::EarthVideo video;std::vector<std::uint32_t> videoPixels;std::uint32_t vw=0,vh=0;std::uint64_t videoSerial=0;
 mcl::LightningField logoBolts{20261119u};
 mcl::LightningField itemBolts{77u,{0,0,100,30},[]{mcl::LightningConfig c;c.strikes=false;c.frontBolts=false;c.minArcs=1;c.maxArcs=3;c.arcChance=0.22f;return c;}()};
 mcl::MainMenuModel menu;
 std::unique_ptr<mcl::CheatSettings> cheats; // MCL_UserData\marvel_cheats.txt
 int cheatGame=0,cheatRow=0;
 Screen screen=Screen::Title;double screenSince=0;
 double layoutFrom=0,layoutTo=0,layoutStart=-1e9; // 0 = title layout, 1 = menu layout
 std::wstring banner;double bannerUntil=0;float bannerFlash=0;
 bool fullscreen=true;
 WORD padPrev=0;int stickPrev=0;
 std::chrono::steady_clock::time_point t0=std::chrono::steady_clock::now();
 double now() const {return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t0).count();}
 float W() const {return rt?rt->GetSize().width:1280.f;}
 float H() const {return rt?rt->GetSize().height:720.f;}
};
App* g=nullptr;

D2D1_COLOR_F rgba(float r,float gg,float b,float a=1.f){return D2D1::ColorF(r,gg,b,a);}
float ease(float t){t=std::clamp(t,0.f,1.f);return 1-(1-t)*(1-t)*(1-t);}

// ---------- resources ----------
void makeFormats(){
 const float h=g->H();
 auto mk=[&](const wchar_t* family,float size,DWRITE_FONT_WEIGHT w,DWRITE_FONT_STYLE s,ComPtr<IDWriteTextFormat>& out){
  out.Reset();
  g->dw->CreateTextFormat(family,nullptr,w,s,DWRITE_FONT_STRETCH_NORMAL,size,L"en-us",&out);
  if(out){out->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);out->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);out->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);}
 };
 mk(L"Impact",h*0.072f,DWRITE_FONT_WEIGHT_NORMAL,DWRITE_FONT_STYLE_ITALIC,g->fPress);
 mk(L"Impact",h*0.060f,DWRITE_FONT_WEIGHT_NORMAL,DWRITE_FONT_STYLE_ITALIC,g->fMenu);
 mk(L"Arial",h*0.020f,DWRITE_FONT_WEIGHT_BOLD,DWRITE_FONT_STYLE_NORMAL,g->fSmall);
 mk(L"Arial",h*0.030f,DWRITE_FONT_WEIGHT_BOLD,DWRITE_FONT_STYLE_NORMAL,g->fBody);
 mk(L"Impact",h*0.050f,DWRITE_FONT_WEIGHT_NORMAL,DWRITE_FONT_STYLE_ITALIC,g->fHead);
 mk(L"Impact",h*0.095f,DWRITE_FONT_WEIGHT_NORMAL,DWRITE_FONT_STYLE_ITALIC,g->fBanner);
}
ComPtr<ID2D1Bitmap> loadPng(const fs::path& p){
 ComPtr<ID2D1Bitmap> out;std::error_code ec;
 if(!g->wic||!fs::is_regular_file(p,ec))return out;
 ComPtr<IWICBitmapDecoder> dec;ComPtr<IWICBitmapFrameDecode> frame;ComPtr<IWICFormatConverter> conv;
 if(FAILED(g->wic->CreateDecoderFromFilename(p.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnDemand,&dec)))return out;
 if(FAILED(dec->GetFrame(0,&frame))||FAILED(g->wic->CreateFormatConverter(&conv)))return out;
 if(FAILED(conv->Initialize(frame.Get(),GUID_WICPixelFormat32bppPBGRA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeMedianCut)))return out;
 g->rt->CreateBitmapFromWicBitmap(conv.Get(),nullptr,&out);
 return out;
}
void makeSizeDependent(){
 makeFormats();
 g->fallbackBg.Reset();
 ComPtr<ID2D1GradientStopCollection> stops;
 const D2D1_GRADIENT_STOP s[]={{0.f,rgba(0.07f,0.16f,0.42f)},{0.55f,rgba(0.03f,0.05f,0.14f)},{1.f,rgba(0.01f,0.01f,0.03f)}};
 g->rt->CreateGradientStopCollection(s,3,&stops);
 g->rt->CreateRadialGradientBrush(D2D1::RadialGradientBrushProperties(D2D1::Point2F(g->W()/2,g->H()*0.42f),{0,0},g->W()*0.7f,g->H()*0.8f),stops.Get(),&g->fallbackBg);
}
bool ensureTarget(){
 if(g->rt)return true;
 RECT rc;GetClientRect(g->hwnd,&rc);
 const auto size=D2D1::SizeU(std::max<LONG>(1,rc.right),std::max<LONG>(1,rc.bottom));
 if(FAILED(g->factory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),D2D1::HwndRenderTargetProperties(g->hwnd,size),&g->rt)))return false;
 g->rt.As(&g->dc);
 g->rt->CreateSolidColorBrush(rgba(1,1,1),&g->brush);
 g->logo=loadPng(g->dir/"Logos"/"mcl_logo.png");
 g->videoBmp.Reset();g->videoSerial=0;
 makeSizeDependent();
 return true;
}

// ---------- drawing helpers ----------
void text(const std::wstring& s,IDWriteTextFormat* f,D2D1_RECT_F r,D2D1_COLOR_F fill,float outline,float alpha=1.f){
 if(!f)return;
 g->brush->SetColor(rgba(0,0,0));g->brush->SetOpacity(alpha);
 if(outline>0){
  for(int i=0;i<16;++i){const float a=i*3.14159265f/8;const float dx=std::cos(a)*outline,dy=std::sin(a)*outline;
   g->rt->DrawText(s.c_str(),static_cast<UINT32>(s.size()),f,D2D1::RectF(r.left+dx,r.top+dy,r.right+dx,r.bottom+dy),g->brush.Get());}
 }
 g->brush->SetColor(fill);g->brush->SetOpacity(alpha);
 g->rt->DrawText(s.c_str(),static_cast<UINT32>(s.size()),f,r,g->brush.Get());
 g->brush->SetOpacity(1);
}
float textWidth(const std::wstring& s,IDWriteTextFormat* f){
 ComPtr<IDWriteTextLayout> l;DWRITE_TEXT_METRICS m{};
 if(f&&SUCCEEDED(g->dw->CreateTextLayout(s.c_str(),static_cast<UINT32>(s.size()),f,10000,1000,&l))&&SUCCEEDED(l->GetMetrics(&m)))return m.width;
 return 200;
}
void polyline(const std::vector<mcl::LPoint>& p,float width,D2D1_COLOR_F c,float alpha,float sx,float ox,float oy){
 if(p.size()<2||alpha<=0.004f)return;
 g->brush->SetColor(c);g->brush->SetOpacity(std::min(1.f,alpha));
 const float w=std::max(0.6f,width*sx);
 for(size_t i=1;i<p.size();++i)
  g->rt->DrawLine(D2D1::Point2F(ox+p[i-1].x*sx,oy+p[i-1].y*sx),D2D1::Point2F(ox+p[i].x*sx,oy+p[i].y*sx),g->brush.Get(),w,g->round.Get());
 g->brush->SetOpacity(1);
}
void additive(bool on){if(g->dc)g->dc->SetPrimitiveBlend(on?D2D1_PRIMITIVE_BLEND_ADD:D2D1_PRIMITIVE_BLEND_SOURCE_OVER);}
void drawBolts(mcl::LightningField& f,double now,bool front,float sx,float ox,float oy,float wscale){
 additive(true);
 for(const auto& b:f.bolts()){
  if(b.front!=front)continue;
  const float a=f.boltAlpha(b,now);if(a<=0)continue;
  auto pass=[&](const std::vector<mcl::LPoint>& p,float m){
   const float w=b.width*m*wscale;
   polyline(p,34*w,rgba(0.12f,0.42f,1.f),0.12f*a,sx,ox,oy);
   polyline(p,16*w,rgba(0.21f,0.72f,1.f),0.32f*a,sx,ox,oy);
   polyline(p,7*w,rgba(0.62f,0.91f,1.f),0.8f*a,sx,ox,oy);
   polyline(p,2.8f*w,rgba(1,1,1),a,sx,ox,oy);
  };
  pass(b.main,1.f);for(const auto& br:b.branches)pass(br,0.55f);
 }
 additive(false);
}
void drawCrackle(mcl::LightningField& f,double now,float sx,float ox,float oy,float wscale,float wobble){
 const auto c=f.crackle(now,wobble);
 additive(true);
 polyline(c,18*wscale,rgba(0.12f,0.42f,1.f),0.10f,sx,ox,oy);
 polyline(c,6*wscale,rgba(0.3f,0.77f,1.f),0.22f,sx,ox,oy);
 polyline(c,1.6f*wscale,rgba(0.85f,0.96f,1.f),0.45f,sx,ox,oy);
 additive(false);
}

// ---------- layout ----------
float layoutT(double now){
 const float t=ease(static_cast<float>((now-g->layoutStart)/450.0));
 return static_cast<float>(g->layoutFrom+(g->layoutTo-g->layoutFrom)*t);
}
void goLayout(double to,double now){g->layoutFrom=layoutT(now);g->layoutTo=to;g->layoutStart=now;}
D2D1_RECT_F logoRect(double now){
 const float W=g->W(),H=g->H(),t=layoutT(now);
 const float wTitle=std::min(W*0.80f,H*0.66f*kLogoW/kLogoH),cyTitle=H*0.40f;
 const float wMenu=std::min(W*0.50f,H*0.40f*kLogoW/kLogoH),cyMenu=H*0.22f;
 const float w=wTitle+(wMenu-wTitle)*t,cy=cyTitle+(cyMenu-cyTitle)*t,h=w*kLogoH/kLogoW;
 return D2D1::RectF(W/2-w/2,cy-h/2,W/2+w/2,cy+h/2);
}
float menuTop(){return g->H()*0.46f;}
float menuStep(){return g->H()*0.078f;}

// ---------- audio ----------
void playVoice(){
 std::error_code ec;
 const fs::path mp3=g->dir/"Audio"/"title_voice.mp3",wav=g->dir/"Audio"/"title_voice.wav";
 if(fs::is_regular_file(mp3,ec)){
  mciSendStringW(L"close mcl_voice",nullptr,0,nullptr);
  const std::wstring open=L"open \""+mp3.wstring()+L"\" type mpegvideo alias mcl_voice";
  if(mciSendStringW(open.c_str(),nullptr,0,nullptr)==0)mciSendStringW(L"play mcl_voice from 0",nullptr,0,nullptr);
 }else if(fs::is_regular_file(wav,ec)){
  PlaySoundW(wav.c_str(),nullptr,SND_FILENAME|SND_ASYNC|SND_NODEFAULT);
 }
}

// ---------- screens ----------
void setScreen(Screen s){g->screen=s;g->screenSince=g->now();}
void showBanner(const std::wstring& s){g->banner=s;g->bannerUntil=g->now()+2200;g->bannerFlash=1.f;g->itemBolts.clear();}
// Background video is paused while another window (Arcade Select, Options, MAME) is in front.
struct VideoPause{VideoPause(){g->video.setPaused(true);}~VideoPause(){g->video.setPaused(false);}};
void activate(MenuItem item){
 if(item==MenuItem::Arcade||item==MenuItem::Options||item==MenuItem::CheatEx){
  VideoPause pause;
  if(item==MenuItem::Options){if(g->cb.options)g->cb.options(g->hwnd,g->menu.cheatExUnlocked());}
  else if(g->cb.arcade)g->cb.arcade(g->hwnd,item==MenuItem::CheatEx||g->menu.cheatExUnlocked(),item==MenuItem::CheatEx,g->menu.cheatsUnlocked());
  return;
 }
 switch(item){
  case MenuItem::Arcade: break; // handled above
  case MenuItem::Options: break; // handled above
  case MenuItem::Cheats:
   if(!g->cheats)g->cheats=std::make_unique<mcl::CheatSettings>(g->dir/"MCL_UserData"/"marvel_cheats.txt");
   g->cheats->load();g->cheatRow=0;setScreen(Screen::Cheats);break;
  case MenuItem::CheatEx: break; // handled above
  case MenuItem::Credits: setScreen(Screen::Credits);break;
  case MenuItem::Exit: DestroyWindow(g->hwnd);break;
 }
}
void onPad(Pad p){
 // Ignore input while another window (Arcade Select, Options, a game) is in front:
 // their modal loops still dispatch messages to this window.
 if(!IsWindowEnabled(g->hwnd))return;
 const double now=g->now();
 switch(g->screen){
  case Screen::Title:{
   // Cheat codes work on the title screen too (classic arcade style).
   const auto r=g->menu.pressOnTitle(p);
   if(r.kind==MK::CheatExUnlocked)showBanner(L"CHEAT EX UNLOCKED!");
   else if(r.kind==MK::CheatsUnlocked)showBanner(L"CHEATS UNLOCKED!");
   else if(r.kind==MK::Activate){
    playVoice();g->bannerFlash=1.f;g->menu.resetCodes();
    setScreen(Screen::Menu);goLayout(1,now);g->itemBolts.clear();
   }
   break;}
  case Screen::Menu:{
   const auto r=g->menu.press(p);
   switch(r.kind){
    case MK::Moved: g->itemBolts.clear();break;
    case MK::Activate: activate(r.item);break;
    case MK::Back: setScreen(Screen::Title);goLayout(0,now);break;
    case MK::CheatExUnlocked: showBanner(L"CHEAT EX UNLOCKED!");break;
    case MK::CheatsUnlocked: showBanner(L"CHEATS UNLOCKED!");break;
    default: break;
   }
   break;}
  case Screen::Credits:
   if(p==Pad::Circle||p==Pad::Cross||p==Pad::Start)setScreen(Screen::Menu);
   break;
  case Screen::Cheats:{
   const auto& games=mcl::marvelCheatGames();
   const int ng=static_cast<int>(games.size());
   const auto& list=mcl::cheatsFor(games[static_cast<size_t>(g->cheatGame)]);
   const int nr=static_cast<int>(list.size());
   switch(p){
    case Pad::Left: g->cheatGame=(g->cheatGame+ng-1)%ng;g->cheatRow=0;break;
    case Pad::Right: g->cheatGame=(g->cheatGame+1)%ng;g->cheatRow=0;break;
    case Pad::Up: if(nr)g->cheatRow=(g->cheatRow+nr-1)%nr;break;
    case Pad::Down: if(nr)g->cheatRow=(g->cheatRow+1)%nr;break;
    case Pad::Cross: case Pad::Start:
     if(nr&&g->cheats){
      const auto& game=games[static_cast<size_t>(g->cheatGame)];
      const auto& c=list[static_cast<size_t>(g->cheatRow)];
      g->cheats->set(game,c.id,!g->cheats->enabled(game,c.id));
      g->cheats->save();
     }
     break;
    case Pad::Circle: setScreen(Screen::Menu);break;
    default: break;
   }
   break;}
 }
}

void drawBackground(){
 const float W=g->W(),H=g->H();
 if(g->video.latest(g->videoPixels,g->vw,g->vh,g->videoSerial)){
  if(!g->videoBmp||g->videoBmp->GetPixelSize().width!=g->vw||g->videoBmp->GetPixelSize().height!=g->vh){
   g->videoBmp.Reset();
   g->rt->CreateBitmap(D2D1::SizeU(g->vw,g->vh),D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,D2D1_ALPHA_MODE_IGNORE)),&g->videoBmp);
  }
  if(g->videoBmp)g->videoBmp->CopyFromMemory(nullptr,g->videoPixels.data(),g->vw*4);
 }
 if(g->videoBmp){
  const auto s=g->videoBmp->GetSize();
  const float k=std::max(W/s.width,H/s.height),w=s.width*k,h=s.height*k;
  g->rt->DrawBitmap(g->videoBmp.Get(),D2D1::RectF((W-w)/2,(H-h)/2,(W+w)/2,(H+h)/2),1.f,D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
  g->brush->SetColor(rgba(0.0f,0.01f,0.04f));g->brush->SetOpacity(0.45f);
  g->rt->FillRectangle(D2D1::RectF(0,0,W,H),g->brush.Get());g->brush->SetOpacity(1);
 }else if(g->fallbackBg){
  g->rt->FillRectangle(D2D1::RectF(0,0,W,H),g->fallbackBg.Get());
 }
}
void drawLogo(double now){
 const auto r=logoRect(now);
 const float sx=(r.right-r.left)/kLogoW;
 g->logoBolts.update(now);
 drawCrackle(g->logoBolts,now,sx,r.left,r.top,1.f,14.f);
 drawBolts(g->logoBolts,now,false,sx,r.left,r.top,1.f);
 if(g->logo){
  if(g->dc)g->dc->DrawBitmap(g->logo.Get(),&r,1.f,D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC,nullptr);
  else g->rt->DrawBitmap(g->logo.Get(),r);
 }else{
  text(L"MARVEL: CROSSOVER LEGACY",g->fBanner.Get(),r,rgba(1,0.8f,0.2f),5);
 }
 drawBolts(g->logoBolts,now,true,sx,r.left,r.top,1.f);
}
void drawTitle(double now){
 const float W=g->W(),H=g->H();
 const bool on=std::fmod(now,900.0)<600.0;
 if(on)text(L"PRESS START",g->fPress.Get(),D2D1::RectF(0,H*0.74f,W,H*0.84f),rgba(1.f,0.84f,0.25f),H*0.005f);
 text(L"ENTER  /  START BUTTON",g->fSmall.Get(),D2D1::RectF(0,H*0.84f,W,H*0.88f),rgba(0.75f,0.82f,0.95f),2,0.85f);
 text(L"© 2026 YAMANATSUCHI INITIATIVE      © 1997 MARVEL, CAPCOM, ARIKA",g->fSmall.Get(),D2D1::RectF(0,H*0.94f,W,H*0.98f),rgba(0.78f,0.82f,0.92f),2,0.9f);
}
void drawMenu(double now){
 const float W=g->W(),H=g->H();
 const float appear=ease(static_cast<float>((now-g->screenSince)/350.0));
 const auto items=g->menu.items();
 for(size_t i=0;i<items.size();++i){
  const std::wstring label=mcl::menuLabel(items[i]);
  const float cy=menuTop()+menuStep()*static_cast<float>(i)+(1-appear)*H*0.05f;
  const D2D1_RECT_F r=D2D1::RectF(0,cy-menuStep()/2,W,cy+menuStep()/2);
  const bool sel=static_cast<int>(i)==g->menu.cursor();
  if(sel){
   const float tw=textWidth(label,g->fMenu.Get());
   g->itemBolts.setRing({W/2,cy,tw*0.5f+H*0.05f,menuStep()*0.42f});
   g->itemBolts.update(now);
   drawCrackle(g->itemBolts,now,1,0,0,0.35f,4.f);
   drawBolts(g->itemBolts,now,false,1,0,0,0.32f);
   const bool hot=items[i]==MenuItem::Cheats||items[i]==MenuItem::CheatEx;
   text(label,g->fMenu.Get(),r,hot?rgba(1.f,0.45f,0.25f):rgba(1.f,0.84f,0.25f),H*0.0045f,appear);
  }else{
   text(label,g->fMenu.Get(),r,rgba(0.86f,0.9f,1.f),H*0.0035f,appear*0.85f);
  }
 }
 text(L"↑↓ CHOOSE      ENTER / ✕ SELECT      ESC / ○ BACK      ALT+ENTER  WINDOW / FULLSCREEN",g->fSmall.Get(),D2D1::RectF(0,H*0.94f,W,H*0.98f),rgba(0.6f,0.66f,0.8f),2,0.85f);
}
void drawCredits(double now){
 const float W=g->W(),H=g->H();
 g->brush->SetColor(rgba(0,0,0.02f));g->brush->SetOpacity(0.55f);g->rt->FillRectangle(D2D1::RectF(0,0,W,H),g->brush.Get());g->brush->SetOpacity(1);
 const float line=H*0.055f,speed=H*0.06f/1000.f;
 const size_t n=sizeof(kCredits)/sizeof(kCredits[0]);
 const float total=line*static_cast<float>(n)+H;
 const float y0=H-std::fmod(static_cast<float>(now-g->screenSince)*speed,total);
 for(size_t i=0;i<n;++i){
  const float y=y0+line*static_cast<float>(i);if(y<-line||y>H+line)continue;
  std::wstring s=kCredits[i];const bool head=!s.empty()&&s[0]==L'#';if(head)s.erase(0,1);
  text(s,head?g->fHead.Get():g->fBody.Get(),D2D1::RectF(0,y-line/2,W,y+line/2),head?rgba(1.f,0.84f,0.25f):rgba(0.92f,0.95f,1.f),head?H*0.004f:2);
 }
 text(L"ESC / ○ BACK",g->fSmall.Get(),D2D1::RectF(0,H*0.94f,W,H*0.98f),rgba(0.6f,0.66f,0.8f),2);
}
const wchar_t* cheatGameTitle(const std::string& id){
 if(id=="xmcota")return L"X-MEN: CHILDREN OF THE ATOM";
 if(id=="msh")return L"MARVEL SUPER HEROES";
 if(id=="xmvsf")return L"X-MEN VS. STREET FIGHTER";
 if(id=="mshvsf")return L"MARVEL SUPER HEROES VS. STREET FIGHTER";
 if(id=="mvsc")return L"MARVEL VS. CAPCOM";
 return L"";
}
void drawCheats(double now){
 const float W=g->W(),H=g->H();
 g->brush->SetColor(rgba(0,0,0.02f));g->brush->SetOpacity(0.72f);
 g->rt->FillRectangle(D2D1::RectF(W*0.12f,H*0.38f,W*0.88f,H*0.93f),g->brush.Get());g->brush->SetOpacity(1);
 text(L"MARVEL CHEATS",g->fHead.Get(),D2D1::RectF(0,H*0.39f,W,H*0.45f),rgba(1.f,0.45f,0.25f),H*0.004f);
 const auto& games=mcl::marvelCheatGames();
 const std::string& game=games[static_cast<size_t>(g->cheatGame)];
 const bool blink=std::fmod(now,1000.0)<650.0;
 text(std::wstring(blink?L"◀   ":L"    ")+cheatGameTitle(game)+(blink?L"   ▶":L"    "),g->fBody.Get(),
      D2D1::RectF(0,H*0.455f,W,H*0.505f),rgba(1.f,0.84f,0.25f),2);
 const auto& list=mcl::cheatsFor(game);
 const float top=H*0.525f,step=H*0.036f;
 for(size_t i=0;i<list.size();++i){
  const bool sel=static_cast<int>(i)==g->cheatRow;
  const bool on=g->cheats&&g->cheats->enabled(game,list[i].id);
  const float y=top+step*static_cast<float>(i);
  std::wstring label;for(const char* c=list[i].label;*c;++c)label+=static_cast<wchar_t>(*c);
  const D2D1_RECT_F left=D2D1::RectF(W*0.18f,y,W*0.70f,y+step),right=D2D1::RectF(W*0.70f,y,W*0.82f,y+step);
  if(sel){g->brush->SetColor(rgba(0.12f,0.42f,1.f));g->brush->SetOpacity(0.35f);g->rt->FillRectangle(D2D1::RectF(W*0.16f,y,W*0.84f,y+step),g->brush.Get());g->brush->SetOpacity(1);}
  ComPtr<IDWriteTextLayout> l;
  g->dw->CreateTextLayout(label.c_str(),static_cast<UINT32>(label.size()),g->fBody.Get(),left.right-left.left,step,&l);
  if(l){l->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
   g->brush->SetColor(sel?rgba(1,1,1):rgba(0.82f,0.87f,0.97f));
   g->rt->DrawTextLayout(D2D1::Point2F(left.left,left.top),l.Get(),g->brush.Get());}
  text(on?L"ON":L"OFF",g->fBody.Get(),right,on?rgba(0.35f,1.f,0.45f):rgba(0.55f,0.6f,0.7f),2);
 }
 text(L"Applies when you launch the game.  Achievements are disabled while cheats are on.  MvC2 (Flycast): no cheats yet.",
      g->fSmall.Get(),D2D1::RectF(0,H*0.895f,W,H*0.93f),rgba(0.65f,0.72f,0.85f),2);
 text(L"←→ GAME      ↑↓ CHEAT      ENTER / ✕ ON-OFF      ESC / ○ BACK",g->fSmall.Get(),D2D1::RectF(0,H*0.94f,W,H*0.98f),rgba(0.6f,0.66f,0.8f),2);
}
void render(){
 if(!ensureTarget())return;
 const double now=g->now();
 const float W=g->W(),H=g->H();
 g->rt->BeginDraw();
 g->rt->Clear(rgba(0,0,0));
 drawBackground();
 drawLogo(now);
 switch(g->screen){
  case Screen::Title: drawTitle(now);break;
  case Screen::Menu: drawMenu(now);break;
  case Screen::Credits: drawCredits(now);break;
  case Screen::Cheats: drawCheats(now);break;
 }
 if(now<g->bannerUntil){
  const float k=static_cast<float>((g->bannerUntil-now)/2200.0);
  text(g->banner,g->fBanner.Get(),D2D1::RectF(0,H*0.35f,W,H*0.5f),rgba(1.f,0.84f,0.25f),H*0.006f,std::min(1.f,k*3));
 }
 const float flash=std::max(g->logoBolts.flash(),g->bannerFlash);
 if(flash>0.01f){g->brush->SetColor(rgba(0.75f,0.92f,1.f));g->brush->SetOpacity(flash*0.35f);g->rt->FillRectangle(D2D1::RectF(0,0,W,H),g->brush.Get());g->brush->SetOpacity(1);}
 g->bannerFlash=std::max(0.f,g->bannerFlash-0.06f);
 if(g->rt->EndDraw()==D2DERR_RECREATE_TARGET){g->rt.Reset();g->dc.Reset();g->brush.Reset();g->logo.Reset();g->videoBmp.Reset();}
}

// ---------- input ----------
void pollPad(){
 XINPUT_STATE st{};DWORD found=ERROR_DEVICE_NOT_CONNECTED;
 for(DWORD i=0;i<XUSER_MAX_COUNT&&found!=ERROR_SUCCESS;++i){ZeroMemory(&st,sizeof st);found=XInputGetState(i,&st);}
 if(found!=ERROR_SUCCESS){g->padPrev=0;g->stickPrev=0;return;}
 const WORD b=st.Gamepad.wButtons,pressed=static_cast<WORD>(b&~g->padPrev);g->padPrev=b;
 if(pressed&XINPUT_GAMEPAD_DPAD_UP)onPad(Pad::Up);
 if(pressed&XINPUT_GAMEPAD_DPAD_DOWN)onPad(Pad::Down);
 if(pressed&XINPUT_GAMEPAD_DPAD_LEFT)onPad(Pad::Left);
 if(pressed&XINPUT_GAMEPAD_DPAD_RIGHT)onPad(Pad::Right);
 if(pressed&XINPUT_GAMEPAD_A)onPad(Pad::Cross);
 if(pressed&XINPUT_GAMEPAD_B)onPad(Pad::Circle);
 if(pressed&XINPUT_GAMEPAD_BACK)onPad(Pad::Select);
 if(pressed&XINPUT_GAMEPAD_START)onPad(Pad::Start);
 const SHORT y=st.Gamepad.sThumbLY;const int dir=y>20000?1:(y<-20000?-1:0);
 if(dir!=g->stickPrev){if(dir==1)onPad(Pad::Up);else if(dir==-1)onPad(Pad::Down);g->stickPrev=dir;}
}
void setFullscreen(bool on){
 g->fullscreen=on;
 MONITORINFO mi{sizeof mi};GetMonitorInfoW(MonitorFromWindow(g->hwnd,MONITOR_DEFAULTTONEAREST),&mi);
 if(on){
  SetWindowLongPtrW(g->hwnd,GWL_STYLE,WS_POPUP|WS_VISIBLE);
  const RECT m=mi.rcMonitor;
  SetWindowPos(g->hwnd,HWND_TOP,m.left,m.top,m.right-m.left,m.bottom-m.top,SWP_FRAMECHANGED|SWP_SHOWWINDOW);
 }else{
  SetWindowLongPtrW(g->hwnd,GWL_STYLE,WS_OVERLAPPEDWINDOW|WS_VISIBLE);
  RECT r{0,0,1280,720};AdjustWindowRect(&r,WS_OVERLAPPEDWINDOW,FALSE);
  const RECT wa=mi.rcWork;const int w=r.right-r.left,h=r.bottom-r.top;
  SetWindowPos(g->hwnd,HWND_TOP,wa.left+(wa.right-wa.left-w)/2,wa.top+(wa.bottom-wa.top-h)/2,w,h,SWP_FRAMECHANGED|SWP_SHOWWINDOW);
 }
}
LRESULT CALLBACK proc(HWND h,UINT m,WPARAM w,LPARAM l){
 switch(m){
  case WM_SIZE:
   if(g&&g->rt){g->rt->Resize(D2D1::SizeU(LOWORD(l),HIWORD(l)));makeSizeDependent();}
   return 0;
  case WM_SETCURSOR:
   if(g&&g->fullscreen&&LOWORD(l)==HTCLIENT){SetCursor(nullptr);return TRUE;}
   break;
  case WM_SYSKEYDOWN:
   if(w==VK_RETURN&&(HIWORD(l)&KF_ALTDOWN)){setFullscreen(!g->fullscreen);return 0;}
   break;
  case WM_KEYDOWN:{
   const bool repeat=(l&(1<<30))!=0;
   switch(w){
    case VK_UP: onPad(Pad::Up);return 0;
    case VK_DOWN: onPad(Pad::Down);return 0;
    case VK_LEFT: if(!repeat)onPad(Pad::Left);return 0;
    case VK_RIGHT: if(!repeat)onPad(Pad::Right);return 0;
    case 'X': case VK_SPACE: if(!repeat)onPad(Pad::Cross);return 0;
    case 'C': case VK_ESCAPE: if(!repeat)onPad(Pad::Circle);return 0;
    case VK_BACK: if(!repeat)onPad(Pad::Select);return 0;
    case VK_RETURN: if(!repeat)onPad(Pad::Start);return 0;
   }
   break;}
  case WM_LBUTTONDOWN:
   if(g&&g->screen==Screen::Title)onPad(Pad::Start);
   return 0;
  case WM_ERASEBKGND: return 1;
  case WM_DESTROY: PostQuitMessage(0);return 0;
 }
 return DefWindowProcW(h,m,w,l);
}
}

int mcl_title::run(HINSTANCE inst,const fs::path& appDir,const Callbacks& cb){
 const HRESULT co=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
 App app;g=&app;app.inst=inst;app.dir=appDir;app.cb=cb;
 D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,__uuidof(ID2D1Factory1),nullptr,reinterpret_cast<void**>(app.factory.GetAddressOf()));
 DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,__uuidof(IDWriteFactory),reinterpret_cast<IUnknown**>(app.dw.GetAddressOf()));
 CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&app.wic));
 if(!app.factory||!app.dw){g=nullptr;if(SUCCEEDED(co))CoUninitialize();return 3;}
 const D2D1_STROKE_STYLE_PROPERTIES sp=D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_ROUND,D2D1_CAP_STYLE_ROUND,D2D1_CAP_STYLE_ROUND,D2D1_LINE_JOIN_ROUND);
 app.factory->CreateStrokeStyle(sp,nullptr,0,&app.round);

 WNDCLASSEXW wc{sizeof wc};wc.lpfnWndProc=proc;wc.hInstance=inst;wc.hCursor=LoadCursor(nullptr,IDC_ARROW);
 wc.hbrBackground=static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));wc.lpszClassName=L"MCLTitleScreen";
 wc.hIcon=LoadIcon(nullptr,IDI_APPLICATION);
 RegisterClassExW(&wc);
 app.hwnd=CreateWindowExW(0,wc.lpszClassName,L"Marvel: Crossover Legacy",WS_POPUP,0,0,1280,720,nullptr,nullptr,inst,nullptr);
 if(!app.hwnd){g=nullptr;if(SUCCEEDED(co))CoUninitialize();return 2;}
 setFullscreen(true);
 app.video.start(appDir/"earth_background.mp4",1280,720);
 MSG msg{};bool quit=false;
 while(!quit){
  while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){
   if(msg.message==WM_QUIT){quit=true;break;}
   TranslateMessage(&msg);DispatchMessageW(&msg);
  }
  if(quit||!IsWindow(app.hwnd))break;
  if(IsIconic(app.hwnd)||!IsWindowEnabled(app.hwnd)){app.video.setPaused(true);WaitMessage();continue;}
  app.video.setPaused(false);
  pollPad();
  if(IsWindow(app.hwnd))render();
 }
 app.video.stop();
 mciSendStringW(L"close mcl_voice",nullptr,0,nullptr);
 app.rt.Reset();app.dc.Reset();
 g=nullptr;
 if(SUCCEEDED(co))CoUninitialize();
 return static_cast<int>(msg.wParam);
}
#endif
