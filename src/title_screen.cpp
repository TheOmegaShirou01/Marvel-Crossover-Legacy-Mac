#ifdef _WIN32
#include "title_screen.hpp"
#include "earth_video.hpp"
#include "lightning.hpp"
#include "menu_model.hpp"
#include "cheats.hpp"
#include "game_launcher.hpp"
#include "music.hpp"
#include "music_player.hpp"
#include "app_settings.hpp"
#include <map>
#include <fstream>
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
enum class Screen { Title, Menu, Credits, Cheats, Select, Options, Controls };
enum OptRow { OptMusic, OptVoice, OptDisplay, OptGames, OptVideo, OptCaption, OptControls, OptDevTools, OptBack, OptCount };

// Arcade Select cards: collection id, full title, credit line.
struct CardInfo { const char* id; const wchar_t* title; const wchar_t* sub; };
const CardInfo kMarvelCards[]={
 {"xmcota",L"X-MEN: CHILDREN OF THE ATOM",L"CAPCOM   •   1994   •   CPS-2"},
 {"msh",L"MARVEL SUPER HEROES",L"CAPCOM   •   1995   •   CPS-2"},
 {"xmvsf",L"X-MEN VS. STREET FIGHTER",L"CAPCOM   •   1996   •   CPS-2"},
 {"mshvsf",L"MARVEL SUPER HEROES VS. STREET FIGHTER",L"CAPCOM   •   1997   •   CPS-2"},
 {"mvsc",L"MARVEL VS. CAPCOM: CLASH OF SUPER HEROES",L"CAPCOM   •   1998   •   CPS-2"},
 {"mvsc2",L"MARVEL VS. CAPCOM 2: NEW AGE OF HEROES",L"CAPCOM   •   2000   •   NAOMI"},
};
const CardInfo kExCards[]={
 {"sfexp",L"STREET FIGHTER EX PLUS",L"ARIKA / CAPCOM   •   1997   •   ZN-1"},
 {"sfex2p",L"STREET FIGHTER EX2 PLUS",L"ARIKA / CAPCOM   •   1999   •   ZN-2"},
};

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
 std::unique_ptr<mcl::CheatSettings> cheats; // MCL_UserData\marvel_cheats.txt (Marvel + EX)
 int cheatGame=0,cheatRow=0;
 std::vector<std::string> cheatGames;Screen cheatsReturn=Screen::Menu;const wchar_t* cheatsTitle=L"MARVEL CHEATS";
 // Arcade Select
 bool exMode=false;int selCard=0;double selSince=0;
 std::map<std::string,ComPtr<ID2D1Bitmap>> cards;
 mcl::EarthVideo preview;std::string previewId;std::string previewReady;
 ComPtr<ID2D1Bitmap> previewBmp;std::vector<std::uint32_t> previewPixels;std::uint32_t pw=0,ph=0;std::uint64_t previewSerial=0;
 mcl::LightningField cardBolts{4242u,{0,0,100,60},[]{mcl::LightningConfig c;c.strikes=false;c.frontBolts=false;c.minArcs=2;c.maxArcs=4;c.arcChance=0.3f;return c;}()};
 bool gameRunning=false;
 bool gameLoading=false;double loadingSince=0; // our NOW LOADING cover is on top of MAME
 mcl::AppSettings settings;int optRow=0; // OPTIONS screen (MCL_UserData\settings.txt)
 std::wstring overlay; // modal message panel (failed launch etc.)
 // Menu music
 fs::path musicDir;std::vector<mcl::Track> library;
 std::unique_ptr<mcl::MusicRotation> marvelMusic,exMusic;
 mcl::MusicPlayer music;const mcl::Track* track=nullptr;std::string musicPool;
 double fadeStart=-1;std::string pendingPick;bool characterSelectPlayed=false;
 std::wstring nowPlaying;double nowPlayingSince=-1e9;
 double whiteFlashStart=-1e9;          // EXIT GAME > YES: white flash on return
 double quickExitSince=-1;bool quickExit=false; // L1+R1+R2+SELECT+START held 3 s
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
 g->cards.clear();
 for(const auto& c:kMarvelCards)g->cards[c.id]=loadPng(g->dir/"Logos"/"Cards"/(std::string(c.id)+".png"));
 for(const auto& c:kExCards)g->cards[c.id]=loadPng(g->dir/"Logos"/"Cards"/(std::string(c.id)+".png"));
 g->previewBmp.Reset();g->previewSerial=0;g->previewReady.clear();
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
// Layout 0 = title (big logo), 1 = menu, 2 = Arcade Select (small logo at the top).
D2D1_RECT_F logoRect(double now){
 const float W=g->W(),H=g->H(),t=layoutT(now);
 struct L{float w,cy;};
 const L title{std::min(W*0.80f,H*0.66f*kLogoW/kLogoH),H*0.40f};
 const L menu{std::min(W*0.50f,H*0.40f*kLogoW/kLogoH),H*0.22f};
 const L sel{std::min(W*0.34f,H*0.30f*kLogoW/kLogoH),H*0.165f};
 const L& a=t<=1.f?title:menu;const L& b=t<=1.f?menu:sel;const float k=t<=1.f?t:t-1.f;
 const float w=a.w+(b.w-a.w)*k,cy=a.cy+(b.cy-a.cy)*k,h=w*kLogoH/kLogoW;
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
  if(g->settings.voiceVolume<=0)return;
  if(mciSendStringW(open.c_str(),nullptr,0,nullptr)==0){
   mciSendStringW((L"setaudio mcl_voice volume to "+std::to_wstring(g->settings.voiceVolume*100)).c_str(),nullptr,0,nullptr);
   mciSendStringW(L"play mcl_voice from 0",nullptr,0,nullptr);
  }
 }else if(fs::is_regular_file(wav,ec)){
  PlaySoundW(wav.c_str(),nullptr,SND_FILENAME|SND_ASYNC|SND_NODEFAULT);
 }
}

// ---------- screens ----------
void goLayout(double to,double now);
float layoutT(double now);
void setScreen(Screen s){
 g->screen=s;g->screenSince=g->now();
 // OPTIONS / CONTROLS use the small top logo like Arcade Select; the menu uses the medium one.
 if((s==Screen::Options||s==Screen::Controls)&&g->layoutTo!=2)goLayout(2,g->now());
 else if(s==Screen::Menu&&g->layoutTo==2)goLayout(1,g->now());
}
void showBanner(const std::wstring& s){g->banner=s;g->bannerUntil=g->now()+2200;g->bannerFlash=1.f;g->itemBolts.clear();}
// Background video is paused while another window (Arcade Select, Options, MAME) is in front.
struct VideoPause{VideoPause(){g->video.setPaused(true);}~VideoPause(){g->video.setPaused(false);}};
// OPTIONS screen handlers (defined further down).
void optionsPad(Pad p);
void changeOption(int dir);
// ---------- Arcade Select helpers ----------
const CardInfo* selCards(int& n){
 if(g->exMode){n=static_cast<int>(sizeof(kExCards)/sizeof(kExCards[0]));return kExCards;}
 n=static_cast<int>(sizeof(kMarvelCards)/sizeof(kMarvelCards[0]));return kMarvelCards;
}
const CardInfo& selected(){int n=0;const CardInfo* c=selCards(n);return c[std::clamp(g->selCard,0,n-1)];}
void enterSelect(bool ex){
 if(!ex&&!g->characterSelectPlayed){
  // First visit this session: the original MvC1 Character Select theme, then the rotation.
  g->characterSelectPlayed=true;g->pendingPick="mvsc/MVC1_Character_Select_PS1.mp3";
  if(g->track&&g->fadeStart<0)g->fadeStart=g->now()-1000; // quick fade into it
 }
 g->exMode=ex;g->selCard=0;g->selSince=g->now();g->cardBolts.clear();
 g->overlay.clear();setScreen(Screen::Select);goLayout(2,g->now());
}
void openCheats(const std::vector<std::string>& games,int start,Screen back,const wchar_t* title){
 if(!g->cheats)g->cheats=std::make_unique<mcl::CheatSettings>(g->dir/"MCL_UserData"/"marvel_cheats.txt");
 g->cheats->load();
 g->cheatGames=games;g->cheatGame=std::clamp(start,0,static_cast<int>(games.size())-1);g->cheatRow=0;
 g->cheatsReturn=back;g->cheatsTitle=title;setScreen(Screen::Cheats);
}
void selectCheats(){
 const std::string id=selected().id;
 const bool ex=g->exMode;
 if(ex?!g->menu.cheatExUnlocked()&&!g->exMode:!g->menu.cheatsUnlocked()){showBanner(L"ENTER THE CHEAT CODE FIRST");return;}
 const auto& list=ex?mcl::exCheatGames():mcl::marvelCheatGames();
 const auto it=std::find(list.begin(),list.end(),id);
 if(it==list.end()){showBanner(L"NO CHEATS FOR THIS GAME YET");return;}
 openCheats(list,static_cast<int>(it-list.begin()),Screen::Select,ex?L"CHEAT EX":L"MARVEL CHEATS");
}
void launchSelected(){
 if(!g->cb.launcher){g->overlay=L"COULD NOT START THE GAME\n\nNo launcher is connected.";return;}
 g->preview.stop();g->previewId.clear();g->previewReady.clear();
 g->cb.launcher->setGamesWindowed(!g->settings.gamesFullscreen);
 const auto r=g->cb.launcher->launch(selected().id,g->menu.cheatsUnlocked(),g->menu.cheatExUnlocked()||g->exMode);
 if(!r.ok){g->overlay=L"COULD NOT START THE GAME\n\n"+r.message;return;}
 g->gameRunning=true;g->music.stop();g->track=nullptr;g->fadeStart=-1;
 // Stay on top with our own loading screen until MAME has finished loading its ROMs,
 // so its plain "Loading Machine (xx%)" box is never seen.
 g->gameLoading=true;g->loadingSince=g->now();
 SetWindowPos(g->hwnd,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
}
void moveCard(Pad p){
 int n=0;selCards(n);
 const int cols=g->exMode?2:3;
 int s=g->selCard;
 if(p==Pad::Left)s=(s+n-1)%n;
 else if(p==Pad::Right)s=(s+1)%n;
 else if(p==Pad::Up||p==Pad::Down){if(n>cols)s=(s+cols)%n;}
 if(s!=g->selCard){g->selCard=s;g->selSince=g->now();g->cardBolts.clear();}
}
void activate(MenuItem item){
 if(item==MenuItem::Options){g->optRow=0;setScreen(Screen::Options);return;}
 if(item==MenuItem::Arcade||item==MenuItem::CheatEx){enterSelect(item==MenuItem::CheatEx);return;}
 switch(item){
  case MenuItem::Arcade: break; // handled above
  case MenuItem::Options: break; // handled above
  case MenuItem::Cheats:{
   std::vector<std::string> list=mcl::marvelCheatGames();
   if(g->menu.cheatExUnlocked())for(const auto& e:mcl::exCheatGames())list.push_back(e);
   openCheats(list,0,Screen::Menu,L"CHEATS");break;}
  case MenuItem::CheatEx: break; // handled above
  case MenuItem::Credits: setScreen(Screen::Credits);break;
  case MenuItem::Exit: DestroyWindow(g->hwnd);break;
 }
}
void onPad(Pad p){
 // Ignore input while another window (Arcade Select, Options, a game) is in front:
 // their modal loops still dispatch messages to this window.
 if(!IsWindowEnabled(g->hwnd)||g->gameRunning)return;
 if(!g->overlay.empty()){if(p==Pad::Cross||p==Pad::Circle||p==Pad::Start)g->overlay.clear();return;}
 const double now=g->now();
 switch(g->screen){
  case Screen::Options: optionsPad(p);break;
  case Screen::Controls: if(p==Pad::Circle||p==Pad::Cross||p==Pad::Start)setScreen(Screen::Options);break;
  case Screen::Select:
   switch(p){
    case Pad::Left: case Pad::Right: case Pad::Up: case Pad::Down: moveCard(p);break;
    case Pad::Cross: case Pad::Start: launchSelected();break;
    case Pad::Circle: g->preview.stop();g->previewId.clear();g->previewReady.clear();setScreen(Screen::Menu);goLayout(1,now);break;
    case Pad::Select: selectCheats();break;
   }
   break;
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
   const auto& games=g->cheatGames;
   if(games.empty()){setScreen(g->cheatsReturn);break;}
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
    case Pad::Circle: setScreen(g->cheatsReturn);if(g->cheatsReturn==Screen::Select)g->selSince=g->now();break;
    default: break;
   }
   break;}
 }
}

// ---------- keyboard-only actions ----------
void startMenu(){
 playVoice();g->bannerFlash=1.f;g->menu.resetCodes();
 setScreen(Screen::Menu);goLayout(1,g->now());g->itemBolts.clear();
}
// Enter: confirm on every screen; does not touch cheat-code progress on title/menu.
void onConfirm(){
 if(!IsWindowEnabled(g->hwnd)||g->gameRunning)return;
 if(!g->overlay.empty()){g->overlay.clear();return;}
 switch(g->screen){
  case Screen::Select: launchSelected();break;
  case Screen::Options: changeOption(0);break;
  case Screen::Controls: setScreen(Screen::Options);break;
  case Screen::Title: startMenu();break;
  case Screen::Menu: g->menu.resetCodes();activate(g->menu.selected());break;
  case Screen::Cheats: onPad(Pad::Cross);break;   // toggle the highlighted cheat
  case Screen::Credits: setScreen(Screen::Menu);break;
 }
}
// Esc: back on every screen.
void onBack(){
 if(!IsWindowEnabled(g->hwnd)||g->gameRunning)return;
 if(!g->overlay.empty()){g->overlay.clear();return;}
 switch(g->screen){
  case Screen::Title: break;
  case Screen::Menu: g->menu.resetCodes();setScreen(Screen::Title);goLayout(0,g->now());break;
  case Screen::Cheats: onPad(Pad::Circle);break;
  case Screen::Credits: setScreen(Screen::Menu);break;
  case Screen::Select: onPad(Pad::Circle);break;
  case Screen::Options: setScreen(Screen::Menu);break;
  case Screen::Controls: setScreen(Screen::Options);break;
 }
}
// X / C / Backspace / Space: cheat-code buttons. On the title screen and main menu they only
// feed the codes; elsewhere X/Space act like confirm and C like back.
void onCodeKey(Pad p){
 if(!IsWindowEnabled(g->hwnd))return;
 if(g->screen==Screen::Title||g->screen==Screen::Menu){
  const auto c=g->menu.feedCodes(p);
  if(c.unlocked==MK::CheatExUnlocked)showBanner(L"CHEAT EX UNLOCKED!");
  else if(c.unlocked==MK::CheatsUnlocked)showBanner(L"CHEATS UNLOCKED!");
  return;
 }
 if(p==Pad::Cross||p==Pad::Start)onConfirm();
 else if(p==Pad::Circle)onBack();
 else if(p==Pad::Select&&g->screen==Screen::Select)onPad(Pad::Select);
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
 text(L"↑↓ CHOOSE      ENTER  SELECT      ESC  BACK      ALT+ENTER  WINDOW / FULLSCREEN",g->fSmall.Get(),D2D1::RectF(0,H*0.94f,W,H*0.98f),rgba(0.6f,0.66f,0.8f),2,0.85f);
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
 if(id=="sfexp")return L"STREET FIGHTER EX PLUS";
 if(id=="sfex2p")return L"STREET FIGHTER EX2 PLUS";
 return L"";
}
void drawCheats(double now){
 const float W=g->W(),H=g->H();
 g->brush->SetColor(rgba(0,0,0.02f));g->brush->SetOpacity(0.72f);
 g->rt->FillRectangle(D2D1::RectF(W*0.12f,H*0.38f,W*0.88f,H*0.93f),g->brush.Get());g->brush->SetOpacity(1);
 text(g->cheatsTitle,g->fHead.Get(),D2D1::RectF(0,H*0.39f,W,H*0.45f),rgba(1.f,0.45f,0.25f),H*0.004f);
 if(g->cheatGames.empty())return;
 const std::string& game=g->cheatGames[static_cast<size_t>(g->cheatGame)];
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
// Draws `bmp` covering `r` (cropping the source to r's aspect), squeezed horizontally by sx.
// zoom > 1 crops further into the centre (preview clips have built-in side bars).
void drawCover(ID2D1Bitmap* bmp,D2D1_RECT_F r,float sx,float opacity,float zoom=1.f){
 if(!bmp)return;
 const auto s=bmp->GetSize();
 const float rw=r.right-r.left,rh=r.bottom-r.top;if(rw<=1||rh<=1)return;
 float cw=s.width,ch=s.width*rh/rw;if(ch>s.height){ch=s.height;cw=s.height*rw/rh;}
 cw/=zoom;ch/=zoom;
 const D2D1_RECT_F src=D2D1::RectF((s.width-cw)/2,(s.height-ch)/2,(s.width+cw)/2,(s.height+ch)/2);
 const float cx=(r.left+r.right)/2,hw=rw/2*sx;
 const D2D1_RECT_F dst=D2D1::RectF(cx-hw,r.top,cx+hw,r.bottom);
 if(g->dc)g->dc->DrawBitmap(bmp,&dst,opacity,D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC,&src);
 else g->rt->DrawBitmap(bmp,dst,opacity,D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,src);
}
void updatePreview(double now){
 const std::string id=selected().id;
 // Start decoding the selected card's clip once the selection has rested briefly.
 if(g->previewId!=id&&now-g->selSince>250){
  g->preview.stop();g->previewReady.clear();g->previewSerial=0;
  g->preview.start(g->dir/"Previews"/(id+".mp4"),640,360);
  g->previewId=id;
 }
 if(g->previewId==id&&g->preview.latest(g->previewPixels,g->pw,g->ph,g->previewSerial)){
  if(!g->previewBmp||g->previewBmp->GetPixelSize().width!=g->pw||g->previewBmp->GetPixelSize().height!=g->ph){
   g->previewBmp.Reset();
   g->rt->CreateBitmap(D2D1::SizeU(g->pw,g->ph),D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,D2D1_ALPHA_MODE_IGNORE)),&g->previewBmp);
  }
  if(g->previewBmp){g->previewBmp->CopyFromMemory(nullptr,g->previewPixels.data(),g->pw*4);g->previewReady=id;}
 }
}
void drawSelect(double now){
 const float W=g->W(),H=g->H();
 int n=0;const CardInfo* cards=selCards(n);
 const int cols=g->exMode?2:3,rows=(n+cols-1)/cols;
 updatePreview(now);
 text(g->exMode?L"CHEAT EX  •  SECRET FIGHTERS":L"SELECT YOUR GAME",g->fHead.Get(),D2D1::RectF(0,H*0.325f,W,H*0.385f),
      g->exMode?rgba(1.f,0.45f,0.25f):rgba(1.f,0.84f,0.25f),H*0.004f);
 const float gap=H*0.035f;
 float ch=H*0.19f,cw=ch*320.f/175.f;
 const float maxW=(W*0.86f-gap*(cols-1))/cols;if(cw>maxW){cw=maxW;ch=cw*175.f/320.f;}
 const float gridW=cw*cols+gap*(cols-1),gridH=ch*rows+gap*(rows-1);
 const float x0=(W-gridW)/2,y0=H*0.40f+(H*0.43f-gridH)/2;
 const std::string selId=selected().id;
 // Flip: 700 ms after the selection settles (and once the clip has a frame), 300 ms animation.
 const double rest=now-g->selSince;
 const bool clip=g->previewReady==selId&&g->previewBmp;
 float flip=0;if(clip&&rest>700)flip=static_cast<float>(std::min(1.0,(rest-700)/300.0));
 D2D1_RECT_F selRect{};
 for(int i=0;i<n;++i){
  const int r=i/cols,c=i%cols;
  const int inRow=std::min(cols,n-r*cols);
  const float rowX=x0+(gridW-(cw*inRow+gap*(inRow-1)))/2;
  D2D1_RECT_F rc=D2D1::RectF(rowX+c*(cw+gap),y0+r*(ch+gap),rowX+c*(cw+gap)+cw,y0+r*(ch+gap)+ch);
  const bool sel=i==g->selCard;
  if(sel){const float gx=cw*0.04f,gy=ch*0.04f;rc=D2D1::RectF(rc.left-gx,rc.top-gy,rc.right+gx,rc.bottom+gy);selRect=rc;continue;}
  g->brush->SetColor(rgba(0.03f,0.05f,0.12f));g->rt->FillRectangle(rc,g->brush.Get());
  drawCover(g->cards[cards[i].id].Get(),rc,1.f,0.55f);
  g->brush->SetColor(rgba(0.75f,0.82f,0.95f));g->brush->SetOpacity(0.5f);g->rt->DrawRectangle(rc,g->brush.Get(),2.f);g->brush->SetOpacity(1);
 }
 // Selected card last: lightning ring, flip between logo and live preview, gold frame.
 if(selRect.right>selRect.left){
  const float cx=(selRect.left+selRect.right)/2,cy=(selRect.top+selRect.bottom)/2;
  const float sw=selRect.right-selRect.left,sh=selRect.bottom-selRect.top;
  g->cardBolts.setRing({cx,cy,sw*0.60f,sh*0.68f});
  g->cardBolts.update(now);
  drawCrackle(g->cardBolts,now,1,0,0,0.45f,6.f);
  drawBolts(g->cardBolts,now,false,1,0,0,0.42f);
  g->brush->SetColor(rgba(0.02f,0.03f,0.08f));g->rt->FillRectangle(selRect,g->brush.Get());
  if(flip<0.5f)drawCover(g->cards[selId].Get(),selRect,1.f-flip*2.f,1.f);
  else drawCover(g->previewBmp.Get(),selRect,flip*2.f-1.f,1.f,1.16f);
  g->brush->SetColor(rgba(1.f,0.84f,0.25f));g->rt->DrawRectangle(selRect,g->brush.Get(),std::max(3.f,H*0.004f));
 }
 const auto& info=selected();
 text(info.title,g->fHead.Get(),D2D1::RectF(0,H*0.845f,W,H*0.895f),rgba(1,1,1),H*0.0035f);
 text(info.sub,g->fSmall.Get(),D2D1::RectF(0,H*0.895f,W,H*0.925f),rgba(0.75f,0.82f,0.95f),2);
 text(L"←↑→↓ CHOOSE      ENTER  PLAY      BACKSPACE  CHEATS      ESC  BACK",g->fSmall.Get(),D2D1::RectF(0,H*0.94f,W,H*0.98f),rgba(0.6f,0.66f,0.8f),2,0.85f);
}
void drawLoading(double now){
 const float W=g->W(),H=g->H();
 g->brush->SetColor(rgba(0,0,0.02f));g->brush->SetOpacity(0.86f);g->rt->FillRectangle(D2D1::RectF(0,0,W,H),g->brush.Get());g->brush->SetOpacity(1);
 const auto& info=selected();
 const float cw=std::min(W*0.42f,H*0.42f*320.f/175.f),ch=cw*175.f/320.f;
 const D2D1_RECT_F r=D2D1::RectF(W/2-cw/2,H*0.44f-ch/2,W/2+cw/2,H*0.44f+ch/2);
 g->cardBolts.setRing({W/2,H*0.44f,cw*0.62f,ch*0.70f});
 g->cardBolts.update(now);
 drawCrackle(g->cardBolts,now,1,0,0,0.6f,8.f);
 drawBolts(g->cardBolts,now,false,1,0,0,0.55f);
 g->brush->SetColor(rgba(0.02f,0.03f,0.08f));g->rt->FillRectangle(r,g->brush.Get());
 drawCover(g->cards[info.id].Get(),r,1.f,1.f);
 g->brush->SetColor(rgba(1.f,0.84f,0.25f));g->rt->DrawRectangle(r,g->brush.Get(),std::max(3.f,H*0.004f));
 text(info.title,g->fHead.Get(),D2D1::RectF(0,H*0.70f,W,H*0.76f),rgba(1,1,1),H*0.0035f);
 const int dots=static_cast<int>(now/350.0)%4;
 std::wstring t=L"NOW LOADING";for(int i=0;i<dots;++i)t+=L'.';
 text(t,g->fPress.Get(),D2D1::RectF(0,H*0.78f,W,H*0.88f),rgba(1.f,0.84f,0.25f),H*0.005f);
}
void drawOverlay(){
 const float W=g->W(),H=g->H();
 g->brush->SetColor(rgba(0,0,0));g->brush->SetOpacity(0.6f);g->rt->FillRectangle(D2D1::RectF(0,0,W,H),g->brush.Get());
 const D2D1_RECT_F panel=D2D1::RectF(W*0.14f,H*0.26f,W*0.86f,H*0.80f);
 g->brush->SetColor(rgba(0.03f,0.05f,0.13f));g->brush->SetOpacity(0.96f);g->rt->FillRectangle(panel,g->brush.Get());g->brush->SetOpacity(1);
 g->brush->SetColor(rgba(1.f,0.45f,0.25f));g->rt->DrawRectangle(panel,g->brush.Get(),3.f);
 ComPtr<IDWriteTextLayout> l;
 const float pad=W*0.03f;
 g->dw->CreateTextLayout(g->overlay.c_str(),static_cast<UINT32>(g->overlay.size()),g->fSmall.Get(),panel.right-panel.left-2*pad,panel.bottom-panel.top-2*pad-H*0.05f,&l);
 if(l){
  l->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);l->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);l->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
  g->brush->SetColor(rgba(0.93f,0.95f,1.f));g->rt->DrawTextLayout(D2D1::Point2F(panel.left+pad,panel.top+pad),l.Get(),g->brush.Get());
 }
 text(L"ENTER / ESC  OK",g->fSmall.Get(),D2D1::RectF(panel.left,panel.bottom-H*0.05f,panel.right,panel.bottom-H*0.01f),rgba(1.f,0.84f,0.25f),2);
}
// ---------- menu music ----------
int musicVolume(){return g->settings.musicVolume*100;} // 0..1000
constexpr double kFadeMs=1500,kMaxPieceMs=180000;
const wchar_t* musicGameTitle(const std::string& id){
 if(id=="xmcota")return L"X-MEN: CHILDREN OF THE ATOM";if(id=="msh")return L"MARVEL SUPER HEROES";
 if(id=="xmvsf")return L"X-MEN VS. STREET FIGHTER";if(id=="mshvsf")return L"MARVEL SUPER HEROES VS. STREET FIGHTER";
 if(id=="mvsc")return L"MARVEL VS. CAPCOM";if(id=="mvsc2")return L"MARVEL VS. CAPCOM 2";
 if(id=="sfexp")return L"STREET FIGHTER EX PLUS";if(id=="sfex2p")return L"STREET FIGHTER EX2 PLUS";
 return L"";
}
void loadMusic(){
 for(const auto& d:{g->dir/"Music",g->dir.parent_path()/"Music"}){
  std::error_code ec;
  if(fs::is_regular_file(d/"catalog.tsv",ec)){g->musicDir=d;g->library=mcl::loadMusicCatalog(d/"catalog.tsv");break;}
 }
 const auto seed=static_cast<std::uint32_t>(GetTickCount64());
 g->marvelMusic=std::make_unique<mcl::MusicRotation>(g->library,mcl::marvelMusicGames(),seed);
 g->exMusic=std::make_unique<mcl::MusicRotation>(g->library,mcl::exMusicGames(),seed^0x9E3779B9u);
}
std::string wantedPool(){
 const bool ex=g->exMode&&(g->screen==Screen::Select||(g->screen==Screen::Cheats&&g->cheatsReturn==Screen::Select));
 return ex?"ex":"marvel";
}
void startNextTrack(const std::string& pool,double now){
 auto& rot=pool=="ex"?g->exMusic:g->marvelMusic;
 g->track=nullptr;g->fadeStart=-1;g->musicPool=pool;
 if(!rot||rot->empty())return;
 for(int attempt=0;attempt<4&&!g->track;++attempt){
  const mcl::Track* t=nullptr;
  if(pool=="marvel"&&!g->pendingPick.empty()){t=rot->pick(g->pendingPick);g->pendingPick.clear();}
  if(!t)t=rot->next();
  if(t&&g->music.play(g->musicDir/fs::u8path(t->path),t->startMs,t->endMs,musicVolume()))g->track=t;
 }
 if(g->track){
  const std::string cap=mcl::trackCaption(*g->track);
  g->nowPlaying=L"♪  "+std::wstring(cap.begin(),cap.end())+L"   —   "+musicGameTitle(g->track->game);
  for(auto& c:g->nowPlaying)c=static_cast<wchar_t>(towupper(c));
  g->nowPlayingSince=now;
 }
 // Status for troubleshooting: bin\Logs\music_status.txt (overwritten each track).
 std::error_code ec;fs::create_directories(g->dir/"Logs",ec);
 std::ofstream log(g->dir/"Logs"/"music_status.txt",std::ios::trunc);
 log<<"music folder: "<<g->musicDir.u8string()<<"\npieces in catalog: "<<g->library.size()
    <<"\npool: "<<pool<<" ("<<(rot?rot->size():0)<<" menu pieces)\n";
 if(g->track)log<<"playing: "<<g->track->path<<" from "<<g->track->startMs<<" to "<<g->track->endMs<<" ms\n";
 else log<<"playing: NOTHING (no piece could be opened by Windows MCI)\n";
}
void updateMusic(double now){
 if(g->library.empty())return;
 // Silent from the moment a game is chosen until it has closed (covers controller launches,
 // which happen inside the same frame as this update).
 if(g->gameRunning||g->settings.musicVolume<=0){if(g->track){g->music.stop();g->track=nullptr;g->fadeStart=-1;}return;}
 const std::string want=wantedPool();
 if(!g->track){startNextTrack(want,now);return;}
 if(g->fadeStart<0){
  const int pos=g->music.positionMs();
  const int end=std::min(g->track->endMs,g->track->startMs+static_cast<int>(kMaxPieceMs));
  // Fade out at the end of the piece (or 3-minute cap), when the pool changes, or if it stopped.
  if(want!=g->musicPool||!g->pendingPick.empty()||pos>=end-static_cast<int>(kFadeMs)||!g->music.playing())g->fadeStart=now;
 }
 if(g->fadeStart>=0){
  const double k=(now-g->fadeStart)/kFadeMs;
  if(k>=1.0){startNextTrack(want,now);return;}
  g->music.setVolume(static_cast<int>(musicVolume()*(1.0-k)));
 }
}
void drawNowPlaying(double now){
 const double age=now-g->nowPlayingSince;
 if(!g->settings.nowPlaying||g->nowPlaying.empty()||age>5000)return;
 const float a=static_cast<float>(std::min({1.0,age/400.0,(5000-age)/600.0}));
 const float W=g->W(),H=g->H();
 ComPtr<IDWriteTextLayout> l;
 g->dw->CreateTextLayout(g->nowPlaying.c_str(),static_cast<UINT32>(g->nowPlaying.size()),g->fSmall.Get(),W*0.9f,H*0.04f,&l);
 if(!l)return;
 l->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
 g->brush->SetColor(rgba(0,0,0));g->brush->SetOpacity(a);
 for(int i=0;i<8;++i){const float ang=i*0.785398f;g->rt->DrawTextLayout(D2D1::Point2F(W*0.02f+std::cos(ang)*2,H*0.02f+std::sin(ang)*2),l.Get(),g->brush.Get());}
 g->brush->SetColor(rgba(1.f,0.84f,0.25f));g->rt->DrawTextLayout(D2D1::Point2F(W*0.02f,H*0.02f),l.Get(),g->brush.Get());
 g->brush->SetOpacity(1);
}
// ---------- OPTIONS ----------
void saveSettings(){g->settings.save(g->dir/"MCL_UserData"/"settings.txt");}
void applyVideoSetting(){
 if(g->settings.backgroundVideo)g->video.start(g->dir/"earth_background.mp4",1280,720);
 else{g->video.stop();g->videoBmp.Reset();g->videoSerial=0;}
}
void setFullscreen(bool on);
// dir: -1 = left, +1 = right, 0 = Enter
void changeOption(int dir){
 auto& s=g->settings;
 switch(g->optRow){
  case OptMusic: if(dir){s.musicVolume=std::clamp(s.musicVolume+dir,0,10);if(g->track&&g->fadeStart<0)g->music.setVolume(musicVolume());}break;
  case OptVoice: if(dir){s.voiceVolume=std::clamp(s.voiceVolume+dir,0,10);playVoice();}break; // preview at the new volume
  case OptDisplay: s.fullscreen=!s.fullscreen;setFullscreen(s.fullscreen);break;
  case OptGames: s.gamesFullscreen=!s.gamesFullscreen;break;
  case OptVideo: s.backgroundVideo=!s.backgroundVideo;applyVideoSetting();break;
  case OptCaption: s.nowPlaying=!s.nowPlaying;if(s.nowPlaying)g->nowPlayingSince=g->now();break;
  case OptControls: if(!dir)setScreen(Screen::Controls);return;
  case OptDevTools: if(!dir){VideoPause pause;if(g->cb.options)g->cb.options(g->hwnd,g->menu.cheatExUnlocked());}return;
  case OptBack: if(!dir)setScreen(Screen::Menu);return;
 }
 saveSettings();
}
void optionsPad(Pad p){
 switch(p){
  case Pad::Up: g->optRow=(g->optRow+OptCount-1)%OptCount;break;
  case Pad::Down: g->optRow=(g->optRow+1)%OptCount;break;
  case Pad::Left: changeOption(-1);break;
  case Pad::Right: changeOption(+1);break;
  case Pad::Cross: case Pad::Start: changeOption(0);break;
  case Pad::Circle: setScreen(Screen::Menu);break;
  default: break;
 }
}
std::wstring volumeBar(int v){std::wstring s;for(int i=0;i<10;++i)s+=i<v?L'■':L'□';return s+L"  "+std::to_wstring(v);}
void drawOptions(double now){
 const float W=g->W(),H=g->H();
 const auto& s=g->settings;
 g->brush->SetColor(rgba(0,0,0.02f));g->brush->SetOpacity(0.74f);
 g->rt->FillRectangle(D2D1::RectF(W*0.14f,H*0.37f,W*0.86f,H*0.93f),g->brush.Get());g->brush->SetOpacity(1);
 text(L"OPTIONS",g->fHead.Get(),D2D1::RectF(0,H*0.38f,W,H*0.44f),rgba(1.f,0.84f,0.25f),H*0.004f);
 struct Row{const wchar_t* label;std::wstring value;};
 const Row rows[OptCount]={
  {L"MUSIC VOLUME",s.musicVolume?volumeBar(s.musicVolume):L"OFF"},
  {L"VOICE VOLUME",s.voiceVolume?volumeBar(s.voiceVolume):L"OFF"},
  {L"DISPLAY",s.fullscreen?L"FULLSCREEN":L"WINDOW"},
  {L"GAMES START IN",s.gamesFullscreen?L"FULLSCREEN":L"WINDOW"},
  {L"BACKGROUND VIDEO",s.backgroundVideo?L"ON":L"OFF"},
  {L"NOW-PLAYING CAPTION",s.nowPlaying?L"ON":L"OFF"},
  {L"CONTROLS",L"▶"},
  {L"DEVELOPER TOOLS",L"▶"},
  {L"BACK",L""},
 };
 const float top=H*0.46f,step=H*0.047f;
 const bool blink=std::fmod(now,900.0)<600.0;
 for(int i=0;i<OptCount;++i){
  const bool sel=i==g->optRow;
  const float y=top+step*static_cast<float>(i);
  if(sel){g->brush->SetColor(rgba(0.12f,0.42f,1.f));g->brush->SetOpacity(0.35f);g->rt->FillRectangle(D2D1::RectF(W*0.18f,y,W*0.82f,y+step),g->brush.Get());g->brush->SetOpacity(1);}
  const auto drawLeft=[&](const std::wstring& t,float x0,float x1,D2D1_COLOR_F c){
   ComPtr<IDWriteTextLayout> l;
   g->dw->CreateTextLayout(t.c_str(),static_cast<UINT32>(t.size()),g->fBody.Get(),x1-x0,step,&l);
   if(l){l->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);g->brush->SetColor(c);g->rt->DrawTextLayout(D2D1::Point2F(x0,y),l.Get(),g->brush.Get());}
  };
  drawLeft(rows[i].label,W*0.21f,W*0.55f,sel?rgba(1,1,1):rgba(0.82f,0.87f,0.97f));
  std::wstring v=rows[i].value;
  const bool adjustable=i<=OptCaption;
  if(sel&&adjustable)v=(blink?L"◀  ":L"   ")+v+(blink?L"  ▶":L"   ");
  drawLeft(v,W*0.55f,W*0.82f,sel?rgba(1.f,0.84f,0.25f):rgba(0.65f,0.75f,0.95f));
 }
 text(L"↑↓ CHOOSE      ←→ CHANGE      ENTER  OPEN / TOGGLE      ESC  BACK      (SAVED AUTOMATICALLY)",g->fSmall.Get(),D2D1::RectF(0,H*0.94f,W,H*0.98f),rgba(0.6f,0.66f,0.8f),2,0.85f);
}
void drawControls(){
 const float W=g->W(),H=g->H();
 g->brush->SetColor(rgba(0,0,0.02f));g->brush->SetOpacity(0.82f);
 g->rt->FillRectangle(D2D1::RectF(W*0.06f,H*0.33f,W*0.94f,H*0.93f),g->brush.Get());g->brush->SetOpacity(1);
 text(L"CONTROLS",g->fHead.Get(),D2D1::RectF(0,H*0.335f,W,H*0.39f),rgba(1.f,0.84f,0.25f),H*0.004f);
 static const wchar_t* const lines[][2]={
  {L"MENUS",L"Arrows move   •   Enter select   •   Esc back   •   Alt+Enter window / fullscreen"},
  {L"",L"Xbox pad: D-pad move   •   A select   •   B back"},
  {L"ARCADE SELECT",L"Enter play   •   Backspace or Tab: cheats for that game"},
  {L"CHEAT CODES",L"On the title screen or main menu:"},
  {L"",L"CHEAT EX:   ↓ → ↓ ←  X  C  Space"},
  {L"",L"MASTER CHEATS:   ↑ ↓ ← →  X  C  X  C  Backspace  Space"},
  {L"",L"(X = ✕   C = ○   Backspace = Select   Space = Start;  pad: A / B / Back / Start)"},
  {L"IN GAME (MAME)",L"5 coin   •   1 start   •   Arrows move   •   Ctrl / Alt / Space punch   •   Shift / Z / X kick"},
  {L"PAUSE MENU",L"Hold Start + Select (1 + 5)   •   Up/Down choose   •   Button 1 or Start: OK   •   Button 2: resume"},
  {L"QUICK EXIT",L"Pad: hold L1 + R1 + R2 + Select + Start for 3 s → title screen   •   Keyboard: Esc quits the game"},
  {L"MvC2 (FLYCAST)",L"Tab opens Flycast's own menu (controls, quit)"},
 };
 const float top=H*0.40f,step=H*0.043f;
 for(size_t i=0;i<sizeof(lines)/sizeof(lines[0]);++i){
  const float y=top+step*static_cast<float>(i);
  ComPtr<IDWriteTextLayout> a,b;
  const std::wstring h=lines[i][0],t=lines[i][1];
  if(!h.empty()){g->dw->CreateTextLayout(h.c_str(),static_cast<UINT32>(h.size()),g->fSmall.Get(),W*0.17f,step,&a);
   if(a){a->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);g->brush->SetColor(rgba(1.f,0.84f,0.25f));g->rt->DrawTextLayout(D2D1::Point2F(W*0.08f,y),a.Get(),g->brush.Get());}}
  g->dw->CreateTextLayout(t.c_str(),static_cast<UINT32>(t.size()),g->fSmall.Get(),W*0.68f,step,&b);
  if(b){b->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);g->brush->SetColor(rgba(0.92f,0.95f,1.f));g->rt->DrawTextLayout(D2D1::Point2F(W*0.25f,y),b.Get(),g->brush.Get());}
 }
 text(L"ESC  BACK",g->fSmall.Get(),D2D1::RectF(0,H*0.94f,W,H*0.98f),rgba(0.6f,0.66f,0.8f),2,0.85f);
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
  case Screen::Select: drawSelect(now);break;
  case Screen::Options: drawOptions(now);break;
  case Screen::Controls: drawControls();break;
 }
 drawNowPlaying(now);
 if(g->gameLoading)drawLoading(now);
 if(!g->overlay.empty())drawOverlay();
 if(now<g->bannerUntil){
  const float k=static_cast<float>((g->bannerUntil-now)/2200.0);
  text(g->banner,g->fBanner.Get(),D2D1::RectF(0,H*0.35f,W,H*0.5f),rgba(1.f,0.84f,0.25f),H*0.006f,std::min(1.f,k*3));
 }
 // Calmer flashes behind the cards so the previews stay watchable.
 {const double wf=(now-g->whiteFlashStart)/700.0;
  if(wf>=0&&wf<1){g->brush->SetColor(rgba(1,1,1));g->brush->SetOpacity(static_cast<float>(1.0-wf));g->rt->FillRectangle(D2D1::RectF(0,0,W,H),g->brush.Get());g->brush->SetOpacity(1);}}
 const float flash=std::max(g->logoBolts.flash()*(g->screen==Screen::Select?0.35f:1.f),g->bannerFlash);
 if(flash>0.01f){g->brush->SetColor(rgba(0.75f,0.92f,1.f));g->brush->SetOpacity(flash*0.35f);g->rt->FillRectangle(D2D1::RectF(0,0,W,H),g->brush.Get());g->brush->SetOpacity(1);}
 g->bannerFlash=std::max(0.f,g->bannerFlash-0.06f);
 if(g->rt->EndDraw()==D2DERR_RECREATE_TARGET){g->rt.Reset();g->dc.Reset();g->brush.Reset();g->logo.Reset();g->videoBmp.Reset();g->previewBmp.Reset();g->cards.clear();}
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
   if(w==VK_RETURN&&(HIWORD(l)&KF_ALTDOWN)){setFullscreen(!g->fullscreen);g->settings.fullscreen=g->fullscreen;saveSettings();return 0;}
   break;
  case WM_KEYDOWN:{
   // Keyboard layout:
   //   Enter = select/confirm (never part of a cheat code)   Esc = back
   //   Arrows = move + code directions (no auto-repeat on title/menu, a held key would break a code)
   //   X = Cross, C = Circle, Backspace = Select, Space = Start: cheat-code buttons only on the
   //   title screen and main menu, so a mistyped code can never open a menu item.
   const bool repeat=(l&(1<<30))!=0;
   const bool lists=g&&(g->screen==Screen::Cheats||g->screen==Screen::Options); // auto-repeat only in lists
   switch(w){
    case VK_UP: if(!repeat||lists)onPad(Pad::Up);return 0;
    case VK_DOWN: if(!repeat||lists)onPad(Pad::Down);return 0;
    case VK_LEFT: if(!repeat)onPad(Pad::Left);return 0;
    case VK_RIGHT: if(!repeat)onPad(Pad::Right);return 0;
    case 'X': if(!repeat)onCodeKey(Pad::Cross);return 0;
    case 'C': if(!repeat)onCodeKey(Pad::Circle);return 0;
    case VK_BACK: if(!repeat)onCodeKey(Pad::Select);return 0;
    case VK_SPACE: if(!repeat)onCodeKey(Pad::Start);return 0;
    case VK_RETURN: if(!repeat)onConfirm();return 0;
    case VK_ESCAPE: if(!repeat)onBack();return 0;
    case VK_TAB: if(!repeat&&g&&g->screen==Screen::Select)onPad(Pad::Select);return 0;
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
 // App icon embedded from src/app.rc (resource id 1).
 wc.hIcon=LoadIconW(inst,MAKEINTRESOURCEW(1));
 wc.hIconSm=static_cast<HICON>(LoadImageW(inst,MAKEINTRESOURCEW(1),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0));
 if(!wc.hIcon)wc.hIcon=LoadIcon(nullptr,IDI_APPLICATION);
 RegisterClassExW(&wc);
 app.hwnd=CreateWindowExW(0,wc.lpszClassName,L"Marvel: Crossover Legacy",WS_POPUP,0,0,1280,720,nullptr,nullptr,inst,nullptr);
 if(!app.hwnd){g=nullptr;if(SUCCEEDED(co))CoUninitialize();return 2;}
 app.settings.load(appDir/"MCL_UserData"/"settings.txt");
 setFullscreen(app.settings.fullscreen);
 if(app.settings.backgroundVideo)app.video.start(appDir/"earth_background.mp4",1280,720);
 loadMusic();
 MSG msg{};bool quit=false;
 while(!quit){
  while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){
   if(msg.message==WM_QUIT){quit=true;break;}
   TranslateMessage(&msg);DispatchMessageW(&msg);
  }
  if(quit||!IsWindow(app.hwnd))break;
  if(app.gameRunning){
   // A game is running: draw nothing, keep the background video paused, wait for it to end.
   // Quick exit (Xbox-style pads): L1 + R1 + R2 + SELECT + START held for 3 seconds.
   {
    XINPUT_STATE st{};bool combo=false;
    for(DWORD i=0;i<XUSER_MAX_COUNT&&!combo;++i){
     ZeroMemory(&st,sizeof st);
     if(XInputGetState(i,&st)!=ERROR_SUCCESS)continue;
     const WORD need=XINPUT_GAMEPAD_LEFT_SHOULDER|XINPUT_GAMEPAD_RIGHT_SHOULDER|XINPUT_GAMEPAD_BACK|XINPUT_GAMEPAD_START;
     combo=(st.Gamepad.wButtons&need)==need&&st.Gamepad.bRightTrigger>200;
    }
    if(!combo)app.quickExitSince=-1;
    else if(app.quickExitSince<0)app.quickExitSince=app.now();
    else if(!app.quickExit&&app.now()-app.quickExitSince>=3000&&app.cb.launcher){app.quickExit=app.cb.launcher->requestClose();}
   }
   std::wstring failure;
   if(app.cb.launcher&&app.cb.launcher->pollEnded(failure)){
    if(app.gameLoading){app.gameLoading=false;SetWindowPos(app.hwnd,HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);}
    app.gameRunning=false;app.video.setPaused(false);
    ShowWindow(app.hwnd,SW_SHOW);SetForegroundWindow(app.hwnd);
    app.selSince=app.now();app.padPrev=0xFFFF; // ignore buttons still held from the game
    if(app.cb.launcher->takeExitSignal()){
     // EXIT GAME > YES in the pause menu: white flash + "Let's Go Crazy!" back on Arcade Select.
     app.whiteFlashStart=app.now();playVoice();
    }
    if(app.quickExit){
     // Quick exit goes all the way back to the title screen.
     app.quickExit=false;app.quickExitSince=-1;
     app.preview.stop();app.previewId.clear();app.previewReady.clear();
     setScreen(Screen::Title);goLayout(0,app.now());
    }
    if(!failure.empty())app.overlay=L"THE GAME COULD NOT START\n\n"+failure;
   }else if(app.gameLoading){
    // MAME is loading behind our cover: keep drawing NOW LOADING until it's ready (or 25 s).
    if(app.cb.launcher->gameReady()||app.now()-app.loadingSince>25000){
     app.gameLoading=false;app.video.setPaused(true);
     SetWindowPos(app.hwnd,HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
     app.cb.launcher->bringToFront();
    }else{render();continue;}
   }else{Sleep(100);continue;}
  }
  if(IsIconic(app.hwnd)||!IsWindowEnabled(app.hwnd)){app.video.setPaused(true);WaitMessage();continue;}
  app.video.setPaused(false);
  pollPad();
  updateMusic(app.now());
  if(IsWindow(app.hwnd))render();
 }
 app.video.stop();app.preview.stop();app.music.stop();
 mciSendStringW(L"close mcl_voice",nullptr,0,nullptr);
 app.rt.Reset();app.dc.Reset();
 g=nullptr;
 if(SUCCEEDED(co))CoUninitialize();
 return static_cast<int>(msg.wParam);
}
#endif
