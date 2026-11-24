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
#include "autosave.hpp"
#include "controls.hpp"
#include "updater.hpp"
#include "mvc2_onebutton.hpp"
#include "gamepads.hpp"
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
#include <cstring>
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
enum class Screen { Intro, Title, Menu, Credits, Cheats, Select, Options, Controls, Achievements, Gallery, Jukebox, ControlSetup, Access };
enum OptRow { OptMusic, OptVoice, OptDisplay, OptGames, OptVideo, OptCaption, OptSetup, OptControls, OptAccess, OptUpdate, OptBack, OptCount };

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
 // Arcade Jukebox: one page per Music\<game> folder; plays whole pieces back to back.
 int jbGame=0,jbRow=0;std::vector<mcl::Track> jbTracks;std::map<std::string,std::string> trackTitles;
 int jbPlaying=-1;std::string jbPlayingGame;std::vector<mcl::Track> jbPlayingList;
 // GALLERY: 0 hub, 1 Arcade Museum, 2 Characters, 3 Artwork; images from Gallery\Art (PS1 rip)
 int galMode=0,galHub=0,galGame=0,galRow=0,galArtIdx=0;
 std::map<std::string,ComPtr<ID2D1Bitmap>> galArt;std::vector<std::string> galArtList;
 double galleryBannerUntil=-1;
 double savedAt=-1e9; // "AUTO-SAVED" toast
 // Controllers: XInput (Xbox) + DirectInput (PlayStation 3/4/5, Switch, generic pads)
 mcl::Gamepads pads;std::vector<std::uint64_t> diPrev;std::vector<int> diStickPrev;
 mcl::ControlsConfig controls;int csRow=0,csCol=0,csCapture=0;double csCaptureSince=0; // CONTROLS SETUP
 mcl::ControlsConfig mvc2Controls{mcl::ControlsConfig::Layout::Mvc2};int csGame=0,csPlayer=0;
 WORD capXB[4]{};std::vector<std::uint64_t> capDi; // MvC2 controller capture: state when it began
 // CONTROLS SETUP > GAME: MARVEL VS. CAPCOM 2 (controls_mvc2.txt)
 int achGroup=0,achRow=0;                 // ACHIEVEMENTS screen
 std::vector<std::string> achQueue;double achShowSince=-1;std::string achSoundFor; // "ACHIEVEMENT UNLOCKED!" popups
 mcl::AppSettings settings;int optRow=0; // OPTIONS screen (MCL_UserData\settings.txt)
 int accRow=0; // OPTIONS > ACCESSIBILITY
 std::wstring overlay; // modal message panel (failed launch etc.)
 // Menu music
 fs::path musicDir;std::vector<mcl::Track> library;
 std::unique_ptr<mcl::MusicRotation> marvelMusic,exMusic;
 mcl::MusicPlayer music;const mcl::Track* track=nullptr;std::string musicPool;
 double fadeStart=-1;std::string pendingPick;bool characterSelectPlayed=false;
 std::wstring nowPlaying;double nowPlayingSince=-1e9;
 double whiteFlashStart=-1e9;          // EXIT GAME > YES: white flash on return
 double quickExitSince=-1;bool quickExit=false; // L1+R1+R2+SELECT+START held 3 s
 // MvC2 (Flycast) pause screen, opened by Back/Select on any pad or Tab. Those are also Flycast's
 // own menu buttons, so Flycast's menu (Cheats, Settings, Save State...) opens underneath; it is
 // covered by this screen, which offers only CONTINUE and EXIT GAME, and closed again on CONTINUE.
 bool flyPause=false,flyConfirm=false,flyMenuOpen=false,flyExiting=false;int flyRow=0;
 bool flyPrevTab=false;
 mcl::Mvc2OneButton mvc2One; // ACCESSIBILITY 1-button moves in MvC2 (the front end types them)
 // Updates from the GitHub releases page: checked once at start (and from OPTIONS).
 // updPanel: 0 none, 1 "UPDATE AVAILABLE" offer, 2 downloading / installing.
 std::unique_ptr<mcl::Updater> updater;int updPanel=0;int updRow=0;bool updOffered=false,updManual=false;
 Screen screen=Screen::Intro;double screenSince=0;
 ComPtr<ID2D1Bitmap> mvcFont; // Fonts\mvc_font.png: MvC's own 8x8 font (tools/extract_mvc_font.py)
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
 g->mvcFont=loadPng(g->dir/"Fonts"/"mvc_font.png");
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
float menuStep(){const float n=static_cast<float>(g->menu.items().size());return g->H()*std::min(0.078f,0.42f/std::max(1.f,n-1));}

// ---------- audio ----------
// Achievement unlock: the game's own coin sound, recorded from the user's ROM by
// tools/extract_unlock_sounds.py into Audio\Unlock\<game>.wav. MvC2 (Flycast) and
// collection trophies use Marvel vs. Capcom's. Played at the VOICE VOLUME setting.
void playUnlockSound(const std::string& game){
 if(g->settings.voiceVolume<=0)return;
 std::error_code ec;
 fs::path f=g->dir/"Audio"/"Unlock"/(game+".wav");
 if(!fs::is_regular_file(f,ec))f=g->dir/"Audio"/"Unlock"/"mvsc.wav";
 if(!fs::is_regular_file(f,ec))return;
 mciSendStringW(L"close mcl_unlock",nullptr,0,nullptr);
 if(mciSendStringW((L"open \""+f.wstring()+L"\" type mpegvideo alias mcl_unlock").c_str(),nullptr,0,nullptr)==0){
  mciSendStringW((L"setaudio mcl_unlock volume to "+std::to_wstring(g->settings.voiceVolume*100)).c_str(),nullptr,0,nullptr);
  mciSendStringW(L"play mcl_unlock from 0",nullptr,0,nullptr);
 }
}
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
 if((s==Screen::Options||s==Screen::Controls||s==Screen::Achievements||s==Screen::Gallery||s==Screen::Jukebox||s==Screen::ControlSetup||s==Screen::Access)&&g->layoutTo!=2)goLayout(2,g->now());
 else if(s==Screen::Menu&&g->layoutTo==2)goLayout(1,g->now());
}
void showBanner(const std::wstring& s){g->banner=s;g->bannerUntil=g->now()+2200;g->bannerFlash=1.f;g->itemBolts.clear();}
// Master code entered: banner + "The Secret Code" achievement.
void masterCodeEntered(){showBanner(L"CHEATS UNLOCKED!");if(g->cb.launcher)g->cb.launcher->awardMasterCode();}
void achievementsPad(Pad p);
void controlSetupPad(Pad p);
void csAssign(const std::string& code);
void drawControlSetup(double now);
void jukeboxPad(Pad p);
void enterJukebox();
void leaveJukebox();
void enterGallery();
void galleryPad(Pad p);
void galleryBack();
void drawAchievements(double now);
void drawAchievementPopup(double now);
void drawGalleryUnlock(double now);
// Background video is paused while another window (Arcade Select, Options, MAME) is in front.
struct VideoPause{VideoPause(){g->video.setPaused(true);}~VideoPause(){g->video.setPaused(false);}};
// OPTIONS screen handlers (defined further down).
void optionsPad(Pad p);
void changeOption(int dir);
void endIntro();
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
 if(selected().id=="mvsc2")g->mvc2One.configure(g->settings.specialsOn(),g->settings.supersOn());
 else g->mvc2One.configure(false,false);
 if(!r.ok){g->overlay=L"COULD NOT START THE GAME\n\n"+r.message;return;}
 g->gameRunning=true;g->music.stop();g->track=nullptr;g->fadeStart=-1;
 // Stay on top with our own loading screen until MAME has finished loading its ROMs,
 // so its plain "Loading Machine (xx%)" box is never seen.
 g->gameLoading=true;g->loadingSince=g->now();
 SetWindowPos(g->hwnd,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
 g->cb.launcher->startCoverGuard(g->hwnd); // keeps the emulator's own loading box under the cover
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
  case MenuItem::Achievements: g->achRow=0;setScreen(Screen::Achievements);break;
  case MenuItem::Gallery: enterGallery();break;
  case MenuItem::Jukebox: enterJukebox();break;
  case MenuItem::Exit: DestroyWindow(g->hwnd);break;
 }
}
void flyPausePad(Pad p);
void accessPad(Pad p);
void leftText(const std::wstring& t,IDWriteTextFormat* f,float x0,float y,float w,float h,D2D1_COLOR_F c,DWRITE_TEXT_ALIGNMENT al);
void drawFlyPause();
void drawUpdatePanel(double now);
void updatePanelPad(Pad p);
void onPad(Pad p){
 if(g->flyPause){flyPausePad(p);return;}
 if(g->updPanel&&!g->gameRunning){updatePanelPad(p);return;}
 // Ignore input while another window (Arcade Select, Options, a game) is in front:
 // their modal loops still dispatch messages to this window.
 if(!IsWindowEnabled(g->hwnd)||g->gameRunning)return;
 if(g->screen==Screen::Intro){if(p==Pad::Cross||p==Pad::Circle||p==Pad::Start)endIntro();return;}
 if(!g->overlay.empty()){if(p==Pad::Cross||p==Pad::Circle||p==Pad::Start)g->overlay.clear();return;}
 const double now=g->now();
 switch(g->screen){
  case Screen::Options: optionsPad(p);break;
  case Screen::Achievements: achievementsPad(p);break;
  case Screen::ControlSetup: controlSetupPad(p);break;
  case Screen::Access: accessPad(p);break;
  case Screen::Gallery: galleryPad(p);break;
  case Screen::Jukebox: jukeboxPad(p);break;
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
   else if(r.kind==MK::CheatsUnlocked)masterCodeEntered();
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
    case MK::CheatsUnlocked: masterCodeEntered();break;
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
      if(c.isChoice())g->cheats->setChoice(game,c.id,(g->cheats->choice(game,c.id)+1)%(static_cast<int>(c.choices.size())+1)); // OFF -> 1 -> ... -> OFF
      else g->cheats->set(game,c.id,!g->cheats->enabled(game,c.id));
      if(g->cheats->save())g->savedAt=g->now();
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
 if(g->updPanel){onPad(Pad::Cross);return;}
 if(g->screen==Screen::Intro){endIntro();return;}
 if(!g->overlay.empty()){g->overlay.clear();return;}
 switch(g->screen){
  case Screen::Select: launchSelected();break;
  case Screen::Options: changeOption(0);break;
  case Screen::Controls: setScreen(Screen::Options);break;
  case Screen::Achievements: break;
  case Screen::ControlSetup: controlSetupPad(Pad::Cross);break;
  case Screen::Access: accessPad(Pad::Cross);break;
  case Screen::Gallery: galleryPad(Pad::Cross);break;
  case Screen::Jukebox: jukeboxPad(Pad::Cross);break;
  case Screen::Intro: endIntro();break;
  case Screen::Title: startMenu();break;
  case Screen::Menu: g->menu.resetCodes();activate(g->menu.selected());break;
  case Screen::Cheats: onPad(Pad::Cross);break;   // toggle the highlighted cheat
  case Screen::Credits: setScreen(Screen::Menu);break;
 }
}
// Esc: back on every screen.
void onBack(){
 if(!IsWindowEnabled(g->hwnd)||g->gameRunning)return;
 if(g->updPanel){onPad(Pad::Circle);return;}
 if(g->screen==Screen::Intro){endIntro();return;}
 if(!g->overlay.empty()){g->overlay.clear();return;}
 switch(g->screen){
  case Screen::Intro: endIntro();break;
  case Screen::Title: break;
  case Screen::Menu: g->menu.resetCodes();setScreen(Screen::Title);goLayout(0,g->now());break;
  case Screen::Cheats: onPad(Pad::Circle);break;
  case Screen::Credits: setScreen(Screen::Menu);break;
  case Screen::Select: onPad(Pad::Circle);break;
  case Screen::Options: setScreen(Screen::Menu);break;
  case Screen::Controls: setScreen(Screen::Options);break;
  case Screen::Achievements: setScreen(Screen::Menu);break;
  case Screen::ControlSetup: controlSetupPad(Pad::Circle);break;
  case Screen::Access: accessPad(Pad::Circle);break;
  case Screen::Gallery: galleryBack();break;
  case Screen::Jukebox: leaveJukebox();break;
 }
}
// X / C / Backspace / Space: cheat-code buttons. On the title screen and main menu they only
// feed the codes; elsewhere X/Space act like confirm and C like back.
void onCodeKey(Pad p){
 if(!IsWindowEnabled(g->hwnd))return;
 if(g->screen==Screen::Title||g->screen==Screen::Menu){
  const auto c=g->menu.feedCodes(p);
  if(c.unlocked==MK::CheatExUnlocked)showBanner(L"CHEAT EX UNLOCKED!");
  else if(c.unlocked==MK::CheatsUnlocked)masterCodeEntered();
  return;
 }
 if(p==Pad::Cross||p==Pad::Start)onConfirm();
 else if(p==Pad::Circle)onBack();
 else if(p==Pad::Select&&(g->screen==Screen::Jukebox||g->screen==Screen::ControlSetup))onPad(Pad::Select); // Backspace: stop / clear
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
 if(id=="mvsc2")return L"MARVEL VS. CAPCOM 2";
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
 const int rows=10,n=static_cast<int>(list.size());
 const int first=std::clamp(g->cheatRow-rows/2,0,std::max(0,n-rows)); // long lists scroll
 if(first>0)text(L"▲",g->fSmall.Get(),D2D1::RectF(W*0.84f,top-step,W*0.88f,top),rgba(1.f,0.84f,0.25f),1);
 if(first+rows<n)text(L"▼",g->fSmall.Get(),D2D1::RectF(W*0.84f,top+step*rows,W*0.88f,top+step*(rows+1)),rgba(1.f,0.84f,0.25f),1);
 for(size_t i=static_cast<size_t>(first);i<list.size()&&static_cast<int>(i)<first+rows;++i){
  const bool sel=static_cast<int>(i)==g->cheatRow;
  const int pick=g->cheats&&list[i].isChoice()?g->cheats->choice(game,list[i].id):0;
  const bool on=g->cheats&&(list[i].isChoice()?pick>0:g->cheats->enabled(game,list[i].id));
  const float y=top+step*static_cast<float>(static_cast<int>(i)-first);
  std::wstring label;for(const char* c=list[i].label;*c;++c)label+=static_cast<wchar_t>(*c);
  const bool choice=list[i].isChoice();
  const D2D1_RECT_F left=D2D1::RectF(W*0.18f,y,choice?W*0.52f:W*0.70f,y+step),right=D2D1::RectF(choice?W*0.52f:W*0.70f,y,W*0.82f,y+step);
  if(sel){g->brush->SetColor(rgba(0.12f,0.42f,1.f));g->brush->SetOpacity(0.35f);g->rt->FillRectangle(D2D1::RectF(W*0.16f,y,W*0.84f,y+step),g->brush.Get());g->brush->SetOpacity(1);}
  ComPtr<IDWriteTextLayout> l;
  g->dw->CreateTextLayout(label.c_str(),static_cast<UINT32>(label.size()),g->fBody.Get(),left.right-left.left,step,&l);
  if(l){l->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
   g->brush->SetColor(sel?rgba(1,1,1):rgba(0.82f,0.87f,0.97f));
   g->rt->DrawTextLayout(D2D1::Point2F(left.left,left.top),l.Get(),g->brush.Get());}
  std::wstring value=on?L"ON":L"OFF";
  if(choice&&pick>0){value.clear();for(const char* c=list[i].choices[static_cast<size_t>(pick-1)].label;*c;++c)value+=static_cast<wchar_t>(*c);value=L"◀ "+value+L" ▶";}
  text(value,g->fBody.Get(),right,on?rgba(0.35f,1.f,0.45f):rgba(0.55f,0.6f,0.7f),2);
 }
 text(L"Applies when you launch the game.  Achievements still unlock with cheats on.  Secret characters: ENTER picks the next one.",
      g->fSmall.Get(),D2D1::RectF(0,H*0.895f,W,H*0.93f),rgba(0.65f,0.72f,0.85f),2);
 text(L"←→ GAME      ↑↓ CHEAT      ENTER / ✕ ON-OFF / NEXT      ESC / ○ BACK",g->fSmall.Get(),D2D1::RectF(0,H*0.94f,W,H*0.98f),rgba(0.6f,0.66f,0.8f),2);
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
 if(id=="sfex3")return L"STREET FIGHTER EX3";
 return L"";
}
void loadMusic(){
 for(const auto& d:{g->dir/"Music",g->dir.parent_path()/"Music"}){
  std::error_code ec;
  if(fs::is_regular_file(d/"catalog.tsv",ec)){g->musicDir=d;g->library=mcl::loadMusicCatalog(d/"catalog.tsv");g->trackTitles=mcl::loadTrackTitles(d/"track_titles.tsv");break;}
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
 if(g->screen==Screen::Intro)return;
 if(g->screen==Screen::Jukebox&&!g->gameRunning){
  // Jukebox: whole pieces back to back through the chosen soundtrack; no menu rotation.
  if(g->jbPlaying>=0&&!g->music.playing()&&!g->jbPlayingList.empty()){
   const int n=static_cast<int>(g->jbPlayingList.size());
   const int next=(g->jbPlaying+1)%n;const auto& t=g->jbPlayingList[static_cast<size_t>(next)];
   g->jbPlaying=g->music.play(g->musicDir/fs::u8path(t.path),t.startMs,t.endMs,musicVolume())?next:-1;
  }
  return;
 }
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
// ---------- INTRO: "YAMANATSUCHI INITIATIVE PRESENTS" in Marvel vs. Capcom's font ----------
constexpr double kIntroMs=3800;
// Draws upper-case text with the MvC 8x8 font atlas at an integer pixel scale (crisp pixels).
void drawMvcText(const std::wstring& s,float cx,float top,float scale,float alpha){
 if(!g->mvcFont)return;
 const float w=8.f*scale*static_cast<float>(s.size());
 float x=cx-w/2;
 for(wchar_t c:s){
  if(c>=L'a'&&c<=L'z')c=static_cast<wchar_t>(c-32);
  if(c>0x20&&c<0x60){ // skip spaces: MvC's space tile has stray pixels
   const int i=c-0x20;
   const D2D1_RECT_F src=D2D1::RectF(static_cast<float>(i%16*8),static_cast<float>(i/16*8),static_cast<float>(i%16*8+8),static_cast<float>(i/16*8+8));
   const D2D1_RECT_F dst=D2D1::RectF(x,top,x+8*scale,top+8*scale);
   if(g->dc)g->dc->DrawBitmap(g->mvcFont.Get(),&dst,alpha,D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR,&src);
   else g->rt->DrawBitmap(g->mvcFont.Get(),dst,alpha,D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,src);
  }
  x+=8*scale;
 }
}
void endIntro(){setScreen(Screen::Title);g->layoutFrom=g->layoutTo=0;g->layoutStart=-1e9;}
void drawIntro(double now){
 const float W=g->W(),H=g->H();
 const double t=now-g->screenSince;
 if(t>=kIntroMs){endIntro();return;}
 const float a=static_cast<float>(std::clamp(std::min(t/600.0,(kIntroMs-t)/600.0),0.0,1.0));
 const std::wstring line1=L"YAMANATSUCHI INITIATIVE",line2=L"PRESENTS";
 const float big=std::max(2.f,std::floor(W*0.86f/(8.f*static_cast<float>(line1.size()))));
 const float smallScale=std::max(2.f,std::floor(big*0.75f));
 if(g->mvcFont){
  drawMvcText(line1,W/2,H*0.43f-8*big,big,a);
  drawMvcText(line2,W/2,H*0.43f+big*5,smallScale,a);
 }else{ // atlas not extracted yet: same words in the front end's own style
  text(line1,g->fBanner.Get(),D2D1::RectF(0,H*0.32f,W,H*0.46f),rgba(1.f,0.4f,0.45f),H*0.005f,a);
  text(line2,g->fHead.Get(),D2D1::RectF(0,H*0.47f,W,H*0.55f),rgba(1.f,0.6f,0.7f),H*0.004f,a);
 }
}

// ---------- OPTIONS ----------
void saveSettings(){if(g->settings.save(g->dir/"MCL_UserData"/"settings.txt"))g->savedAt=g->now();}
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
  case OptSetup: if(!dir){g->csRow=0;g->csCol=0;g->csCapture=0;setScreen(Screen::ControlSetup);}return;
  case OptControls: if(!dir)setScreen(Screen::Controls);return;
  case OptAccess: if(!dir){g->accRow=0;setScreen(Screen::Access);}return;
  case OptUpdate:
   if(!dir&&g->updater){
    if(g->updater->state()==mcl::Updater::State::Available){g->updPanel=1;g->updRow=0;}
    else{g->updManual=true;g->updater->startCheck();}
   }
   return;
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
// ---------- OPTIONS > ACCESSIBILITY ----------
// ACCESSIBILITY is the master switch; the four features below it only work while it is ON.
enum AccRow { AccMaster, AccSpecials, AccSupers, AccCheatKey, AccCheatExKey, AccBack, AccCount };
bool& accFlag(int row){
 auto& s=g->settings;
 switch(row){case AccSpecials:return s.oneButtonSpecials;case AccSupers:return s.oneButtonSupers;
  case AccCheatKey:return s.cheatMenuKey;case AccCheatExKey:return s.cheatExMenuKey;default:return s.accessibility;}
}
void accessPad(Pad p){
 switch(p){
  case Pad::Up: g->accRow=(g->accRow+AccCount-1)%AccCount;break;
  case Pad::Down: g->accRow=(g->accRow+1)%AccCount;break;
  case Pad::Left: case Pad::Right: case Pad::Cross: case Pad::Start:
   if(g->accRow==AccBack){if(p==Pad::Cross||p==Pad::Start)setScreen(Screen::Options);break;}
   accFlag(g->accRow)=!accFlag(g->accRow);saveSettings(); // auto-saved
   break;
  case Pad::Circle: setScreen(Screen::Options);break;
  default: break;
 }
}
void drawAccess(double){
 const float W=g->W(),H=g->H();
 const auto& s=g->settings;
 g->brush->SetColor(rgba(0,0,0.02f));g->brush->SetOpacity(0.78f);
 g->rt->FillRectangle(D2D1::RectF(W*0.1f,H*0.37f,W*0.9f,H*0.93f),g->brush.Get());g->brush->SetOpacity(1);
 text(L"ACCESSIBILITY",g->fHead.Get(),D2D1::RectF(0,H*0.38f,W,H*0.44f),rgba(1.f,0.84f,0.25f),H*0.004f);
 struct Row{const wchar_t* label;const wchar_t* help;};
 static const Row rows[AccCount]={
  {L"ACCESSIBILITY",L"Master switch: the options below only work while this is ON."},
  {L"1-BUTTON SPECIALS",L"LIGHT PUNCH, LIGHT KICK, MEDIUM PUNCH and MEDIUM KICK each perform a special move, in all 8 games."},
  {L"1-BUTTON SUPERS",L"STRONG PUNCH and STRONG KICK perform a super when the super meter is full, in all 8 games."},
  {L"CHEAT MENU KEY  (X)",L"Press X on the title screen or main menu to open CHEATS (no code needed)."},
  {L"CHEAT EX MENU KEY  (C)",L"Press C on the title screen or main menu to open CHEAT EX (no code needed)."},
  {L"BACK",L""},
 };
 const float top=H*0.46f,step=H*0.05f;
 for(int i=0;i<AccCount;++i){
  const bool sel=i==g->accRow;
  const float y=top+step*static_cast<float>(i);
  if(sel){g->brush->SetColor(rgba(0.12f,0.42f,1.f));g->brush->SetOpacity(0.35f);g->rt->FillRectangle(D2D1::RectF(W*0.14f,y,W*0.86f,y+step),g->brush.Get());g->brush->SetOpacity(1);}
  const bool dim=i!=AccMaster&&i!=AccBack&&!s.accessibility;
  const D2D1_COLOR_F c=sel?rgba(1,1,1):dim?rgba(0.5f,0.55f,0.65f):rgba(0.82f,0.87f,0.97f);
  leftText(rows[i].label,g->fBody.Get(),W*0.17f,y,W*0.45f,step,c,DWRITE_TEXT_ALIGNMENT_LEADING);
  if(i!=AccBack){
   const bool on=accFlag(i);
   leftText(on?L"ON":L"OFF",g->fBody.Get(),W*0.66f,y,W*0.2f,step,on?(dim?rgba(0.55f,0.6f,0.5f):rgba(0.55f,0.95f,0.6f)):rgba(0.95f,0.55f,0.5f),DWRITE_TEXT_ALIGNMENT_LEADING);
  }
 }
 const std::wstring help=rows[g->accRow].help;
 leftText(help,g->fSmall.Get(),W*0.12f,H*0.79f,W*0.76f,H*0.04f,rgba(0.75f,0.8f,0.95f),DWRITE_TEXT_ALIGNMENT_CENTER);
 if(!s.accessibility&&g->accRow!=AccMaster&&g->accRow!=AccBack)
  leftText(L"TURN ACCESSIBILITY ON FOR THIS TO WORK",g->fSmall.Get(),W*0.12f,H*0.83f,W*0.76f,H*0.04f,rgba(1.f,0.6f,0.4f),DWRITE_TEXT_ALIGNMENT_CENTER);
 text(L"↑↓ CHOOSE      ←→ / ENTER  ON / OFF      ESC  BACK      (SAVED AUTOMATICALLY)",g->fSmall.Get(),D2D1::RectF(0,H*0.94f,W,H*0.98f),rgba(0.6f,0.66f,0.8f),2,0.85f);
}
// X / C on the title screen or main menu (ACCESSIBILITY): open CHEATS / CHEAT EX without the code.
bool cheatShortcut(bool ex){
 const auto& s=g->settings;
 if(!s.accessibility||(ex?!s.cheatExMenuKey:!s.cheatMenuKey))return false;
 if(g->screen!=Screen::Title&&g->screen!=Screen::Menu)return false;
 const bool fromTitle=g->screen==Screen::Title;
 g->menu.resetCodes();
 if(ex){g->menu.unlockCheatEx();openCheats(mcl::exCheatGames(),0,Screen::Menu,L"CHEAT EX");}
 else{
  g->menu.unlockCheats();
  std::vector<std::string> list=mcl::marvelCheatGames();
  if(g->menu.cheatExUnlocked())for(const auto& e:mcl::exCheatGames())list.push_back(e);
  openCheats(list,0,Screen::Menu,L"CHEATS");
 }
 if(fromTitle)goLayout(1,g->now());
 return true;
}
std::wstring wide(const std::string& s);
std::wstring updateStatus(){
 if(!g->updater)return L"▶";
 using S=mcl::Updater::State;
 switch(g->updater->state()){
  case S::Checking: return L"CHECKING...";
  case S::UpToDate: return std::wstring(L"UP TO DATE  (v")+wide(mcl::kAppVersion)+L")";
  case S::Available: return L"UPDATE AVAILABLE ▶";
  case S::Failed: return L"COULD NOT CHECK ▶";
  case S::Downloading: case S::Extracting: case S::Ready: return L"DOWNLOADING...";
  default: return std::wstring(L"v")+wide(mcl::kAppVersion)+L"  ▶";
 }
}
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
  {L"CONTROLS SETUP",L"▶"},
  {L"CONTROLS HELP",L"▶"},
  {L"ACCESSIBILITY",s.accessibility?L"ON  ▶":L"OFF  ▶"},
  {L"CHECK FOR UPDATES",updateStatus()},
  {L"BACK",L""},
 };
 const float top=H*0.46f,step=H*0.042f;
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
// ---------- ACHIEVEMENTS ----------
// One page per game (12 each) plus the collection page. Unlocks come from real play: the
// in-game detector reports wins, perfects, streaks, bosses and clears (achievement_lua.cpp).
std::vector<std::string> achGroups(){
 std::vector<std::string> v;
 if(g->cb.launcher)for(const auto& gm:g->cb.launcher->collection().games())v.push_back(gm.id);
 v.push_back("collection");
 return v;
}
std::vector<const mcl::Achievement*> achList(const std::string& group){
 std::vector<const mcl::Achievement*> v;
 if(g->cb.launcher)for(const auto& a:g->cb.launcher->collection().achievements())if(a.game==group)v.push_back(&a);
 return v;
}
std::wstring wide(const std::string& s){return std::wstring(s.begin(),s.end());}
// Unlock times are stored in UTC ("2026-11-19T21:05:00Z"); show the player's local date.
std::wstring localDate(const std::string& utc){
 std::tm t{};
 if(sscanf_s(utc.c_str(),"%d-%d-%dT%d:%d:%d",&t.tm_year,&t.tm_mon,&t.tm_mday,&t.tm_hour,&t.tm_min,&t.tm_sec)!=6)return wide(utc.substr(0,10));
 t.tm_year-=1900;t.tm_mon-=1;
 const std::time_t when=_mkgmtime(&t);
 std::tm local{};
 if(when==-1||localtime_s(&local,&when)!=0)return wide(utc.substr(0,10));
 wchar_t b[16];std::wcsftime(b,16,L"%Y-%m-%d",&local);return b;
}
std::wstring upper(std::wstring s){for(auto& c:s)c=static_cast<wchar_t>(towupper(c));return s;}
std::wstring groupTitle(const std::string& id){
 if(id=="collection")return L"COLLECTION";
 if(g->cb.launcher)for(const auto& gm:g->cb.launcher->collection().games())if(gm.id==id)return upper(wide(gm.title));
 return upper(wide(id));
}
void achievementsPad(Pad p){
 const auto groups=achGroups();
 const int ng=static_cast<int>(groups.size());
 g->achGroup=std::clamp(g->achGroup,0,ng-1);
 const int nr=static_cast<int>(achList(groups[static_cast<size_t>(g->achGroup)]).size());
 switch(p){
  case Pad::Left: g->achGroup=(g->achGroup+ng-1)%ng;g->achRow=0;break;
  case Pad::Right: g->achGroup=(g->achGroup+1)%ng;g->achRow=0;break;
  case Pad::Up: if(nr)g->achRow=(g->achRow+nr-1)%nr;break;
  case Pad::Down: if(nr)g->achRow=(g->achRow+1)%nr;break;
  case Pad::Circle: setScreen(Screen::Menu);break;
  default: break;
 }
}
D2D1_COLOR_F tierColor(const std::string& t){
 if(t=="Platinum")return rgba(0.75f,0.95f,1.f);
 if(t=="Gold")return rgba(1.f,0.84f,0.25f);
 if(t=="Silver")return rgba(0.8f,0.84f,0.9f);
 return rgba(0.86f,0.58f,0.36f);
}
void leftText(const std::wstring& t,IDWriteTextFormat* f,float x0,float y,float w,float h,D2D1_COLOR_F c,DWRITE_TEXT_ALIGNMENT al=DWRITE_TEXT_ALIGNMENT_LEADING){
 ComPtr<IDWriteTextLayout> l;
 g->dw->CreateTextLayout(t.c_str(),static_cast<UINT32>(t.size()),f,w,h,&l);
 if(!l)return;
 l->SetTextAlignment(al);
 DWRITE_TRIMMING trim{DWRITE_TRIMMING_GRANULARITY_CHARACTER,0,0};l->SetTrimming(&trim,nullptr);
 g->brush->SetColor(c);g->rt->DrawTextLayout(D2D1::Point2F(x0,y),l.Get(),g->brush.Get());
}
void drawAchievements(double now){
 const float W=g->W(),H=g->H();
 g->brush->SetColor(rgba(0,0,0.02f));g->brush->SetOpacity(0.82f);
 g->rt->FillRectangle(D2D1::RectF(W*0.06f,H*0.33f,W*0.94f,H*0.93f),g->brush.Get());g->brush->SetOpacity(1);
 if(!g->cb.launcher){text(L"ACHIEVEMENTS UNAVAILABLE",g->fHead.Get(),D2D1::RectF(0,H*0.5f,W,H*0.56f),rgba(1,1,1),2);return;}
 const auto& col=g->cb.launcher->collection();
 int total=0,got=0;
 for(const auto& a:col.achievements()){++total;if(col.progress(a.id).unlocked)++got;}
 text(L"ACHIEVEMENTS",g->fHead.Get(),D2D1::RectF(0,H*0.335f,W,H*0.385f),rgba(1.f,0.84f,0.25f),H*0.004f);
 const auto groups=achGroups();
 g->achGroup=std::clamp(g->achGroup,0,static_cast<int>(groups.size())-1);
 const auto& group=groups[static_cast<size_t>(g->achGroup)];
 const auto list=achList(group);
 int gGot=0;for(auto* a:list)if(col.progress(a->id).unlocked)++gGot;
 const bool blink=std::fmod(now,900.0)<600.0;
 const std::wstring head=(blink?L"◀   ":L"    ")+groupTitle(group)+L"   "+std::to_wstring(gGot)+L" / "+std::to_wstring(list.size())+(blink?L"   ▶":L"    ");
 text(head,g->fBody.Get(),D2D1::RectF(0,H*0.385f,W,H*0.425f),rgba(1,1,1),2);
 leftText(std::to_wstring(got)+L" / "+std::to_wstring(total)+L" UNLOCKED",g->fSmall.Get(),W*0.66f,H*0.345f,W*0.26f,H*0.03f,rgba(0.65f,0.75f,0.95f),DWRITE_TEXT_ALIGNMENT_TRAILING);
 const float top=H*0.435f,step=H*0.037f;
 g->achRow=std::clamp(g->achRow,0,std::max(0,static_cast<int>(list.size())-1));
 for(size_t i=0;i<list.size();++i){
  const auto* a=list[i];
  const auto pr=col.progress(a->id);
  const float y=top+step*static_cast<float>(i);
  const bool sel=static_cast<int>(i)==g->achRow;
  if(sel){g->brush->SetColor(rgba(0.12f,0.42f,1.f));g->brush->SetOpacity(0.35f);g->rt->FillRectangle(D2D1::RectF(W*0.08f,y,W*0.92f,y+step),g->brush.Get());g->brush->SetOpacity(1);}
  const D2D1_COLOR_F main=pr.unlocked?rgba(1.f,0.9f,0.45f):rgba(0.55f,0.6f,0.72f);
  leftText(pr.unlocked?L"★":L"☆",g->fSmall.Get(),W*0.09f,y,W*0.03f,step,pr.unlocked?tierColor(a->tier):rgba(0.4f,0.45f,0.55f));
  leftText(upper(wide(a->title)),g->fSmall.Get(),W*0.12f,y,W*0.30f,step,sel?rgba(1,1,1):main);
  leftText(wide(a->condition),g->fSmall.Get(),W*0.43f,y,W*0.33f,step,sel?rgba(0.92f,0.95f,1.f):rgba(0.6f,0.66f,0.78f));
  const std::wstring right=pr.unlocked?localDate(pr.timestamp):upper(wide(a->tier));
  leftText(right,g->fSmall.Get(),W*0.77f,y,W*0.14f,step,pr.unlocked?rgba(0.65f,0.9f,0.65f):tierColor(a->tier),DWRITE_TEXT_ALIGNMENT_TRAILING);
 }
 const std::wstring note=group=="mvsc2"?L"MvC2 RUNS IN FLYCAST: ITS RESULTS CAN'T BE READ YET, SO ONLY LAUNCH-BASED TROPHIES COUNT":
                         L"EARNED IN ARCADE MODE  •  PERFECTS, STREAKS AND BOSSES DON'T COUNT WITH CHEATS ON  •  A CONTINUE BLOCKS 'CLEAR WITHOUT A CONTINUE'";
 text(note,g->fSmall.Get(),D2D1::RectF(0,H*0.885f,W,H*0.92f),rgba(0.55f,0.62f,0.78f),1.5f,0.9f);
 text(L"←→ GAME      ↑↓ CHOOSE      ESC  BACK",g->fSmall.Get(),D2D1::RectF(0,H*0.94f,W,H*0.98f),rgba(0.6f,0.66f,0.8f),2,0.85f);
}
// Unlock popup, bottom right, one at a time (4 s each). Unlocks found when a game ends
// show as soon as the front end is back.
void drawAchievementPopup(double now){
 if(!g->cb.launcher)return;
 if(!g->gameRunning){for(auto& id:g->cb.launcher->takeAchievementNotices())g->achQueue.push_back(id);}
 if(g->achQueue.empty())return;
 if(g->achShowSince<0)g->achShowSince=now;
 const double t=now-g->achShowSince;
 if(t>4000){g->achQueue.erase(g->achQueue.begin());g->achShowSince=g->achQueue.empty()?-1:now;return;}
 const mcl::Achievement* a=nullptr;
 for(const auto& x:g->cb.launcher->collection().achievements())if(x.id==g->achQueue.front())a=&x;
 if(!a){g->achQueue.erase(g->achQueue.begin());g->achShowSince=-1;return;}
 if(g->achSoundFor!=a->id+std::to_string(static_cast<long long>(g->achShowSince))){ // new popup on screen
  g->achSoundFor=a->id+std::to_string(static_cast<long long>(g->achShowSince));
  playUnlockSound(a->game);
 }
 const float W=g->W(),H=g->H();
 const float slide=ease(static_cast<float>(std::min(t,600.0)/600.0))*(t>3600?ease(static_cast<float>((4000-t)/400.0)):1.f);
 const float pw=W*0.30f,ph=H*0.115f;
 const float x=W-(pw+W*0.02f)*slide,y=H*0.83f-ph;
 g->brush->SetColor(rgba(0.02f,0.04f,0.12f));g->brush->SetOpacity(0.93f);
 g->rt->FillRectangle(D2D1::RectF(x,y,x+pw,y+ph),g->brush.Get());g->brush->SetOpacity(1);
 g->brush->SetColor(tierColor(a->tier));g->rt->DrawRectangle(D2D1::RectF(x,y,x+pw,y+ph),g->brush.Get(),H*0.003f);
 leftText(L"★ ACHIEVEMENT UNLOCKED!",g->fSmall.Get(),x+W*0.012f,y+ph*0.06f,pw-W*0.024f,ph*0.3f,rgba(1.f,0.84f,0.25f));
 leftText(upper(wide(a->title)),g->fBody.Get(),x+W*0.012f,y+ph*0.34f,pw-W*0.024f,ph*0.34f,rgba(1,1,1));
 leftText(groupTitle(a->game)+L"  •  "+upper(wide(a->tier)),g->fSmall.Get(),x+W*0.012f,y+ph*0.68f,pw-W*0.024f,ph*0.28f,tierColor(a->tier));
}
// ---------- ARCADE JUKEBOX ----------
void jbLoadGame(){
 const auto& games=mcl::jukeboxGames();
 g->jbGame=(g->jbGame%static_cast<int>(games.size())+static_cast<int>(games.size()))%static_cast<int>(games.size());
 g->jbTracks=mcl::jukeboxTracks(g->library,games[static_cast<size_t>(g->jbGame)]);
 g->jbRow=0;
}
void enterJukebox(){
 g->music.stop();g->track=nullptr;g->fadeStart=-1; // the menu rotation steps aside
 g->jbPlaying=-1;g->jbPlayingList.clear();
 jbLoadGame();setScreen(Screen::Jukebox);
}
void leaveJukebox(){
 g->music.stop();g->jbPlaying=-1;g->jbPlayingList.clear(); // menu rotation resumes by itself
 setScreen(Screen::Menu);
}
void jbPlay(int row){
 if(row<0||row>=static_cast<int>(g->jbTracks.size()))return;
 const auto& t=g->jbTracks[static_cast<size_t>(row)];
 g->jbPlayingList=g->jbTracks;g->jbPlayingGame=mcl::jukeboxGames()[static_cast<size_t>(g->jbGame)];
 g->jbPlaying=g->music.play(g->musicDir/fs::u8path(t.path),t.startMs,t.endMs,musicVolume())?row:-1;
 if(g->jbPlaying<0)g->overlay=L"THIS TRACK COULD NOT BE PLAYED\n\n"+fs::u8path(t.path).wstring();
}
void jukeboxPad(Pad p){
 const int n=static_cast<int>(g->jbTracks.size());
 switch(p){
  case Pad::Left: --g->jbGame;jbLoadGame();break;
  case Pad::Right: ++g->jbGame;jbLoadGame();break;
  case Pad::Up: if(n)g->jbRow=(g->jbRow+n-1)%n;break;
  case Pad::Down: if(n)g->jbRow=(g->jbRow+1)%n;break;
  case Pad::Cross: case Pad::Start: jbPlay(g->jbRow);break;
  case Pad::Select: g->music.stop();g->jbPlaying=-1;g->jbPlayingList.clear();break; // stop
  case Pad::Circle: leaveJukebox();break;
 }
}
std::wstring mmss(int ms){ms=std::max(0,ms)/1000;wchar_t b[16];swprintf(b,16,L"%d:%02d",ms/60,ms%60);return b;}
void drawJukebox(double now){
 const float W=g->W(),H=g->H();
 g->brush->SetColor(rgba(0,0,0.02f));g->brush->SetOpacity(0.84f);
 g->rt->FillRectangle(D2D1::RectF(W*0.06f,H*0.33f,W*0.94f,H*0.93f),g->brush.Get());g->brush->SetOpacity(1);
 text(L"ARCADE JUKEBOX",g->fHead.Get(),D2D1::RectF(0,H*0.335f,W,H*0.385f),rgba(1.f,0.84f,0.25f),H*0.004f);
 const auto& games=mcl::jukeboxGames();
 const float top=H*0.405f,step=H*0.040f;
 // Soundtracks (left)
 for(size_t i=0;i<games.size();++i){
  const float y=top+step*static_cast<float>(i);
  const bool sel=static_cast<int>(i)==g->jbGame;
  if(sel){g->brush->SetColor(rgba(0.12f,0.42f,1.f));g->brush->SetOpacity(0.35f);g->rt->FillRectangle(D2D1::RectF(W*0.08f,y,W*0.37f,y+step),g->brush.Get());g->brush->SetOpacity(1);}
  const int count=static_cast<int>(mcl::jukeboxTracks(g->library,games[i]).size());
  std::wstring name=musicGameTitle(games[i]);if(games[i]=="mshvsf")name=L"MARVEL SUPER HEROES VS. SF";
  leftText(name,g->fSmall.Get(),W*0.09f,y,W*0.23f,step,sel?rgba(1,1,1):rgba(0.7f,0.78f,0.95f));
  leftText(std::to_wstring(count),g->fSmall.Get(),W*0.32f,y,W*0.045f,step,rgba(0.55f,0.62f,0.78f),DWRITE_TEXT_ALIGNMENT_TRAILING);
 }
 // Tracks (right), scrolled so the cursor stays visible
 const int rows=10,n=static_cast<int>(g->jbTracks.size());
 const int first=std::clamp(g->jbRow-rows/2,0,std::max(0,n-rows));
 const std::string thisGame=games[static_cast<size_t>(g->jbGame)];
 if(n==0)leftText(L"NO TRACKS IN Music\\"+wide(thisGame),g->fSmall.Get(),W*0.40f,top,W*0.5f,step,rgba(0.7f,0.7f,0.75f));
 for(int i=first;i<std::min(n,first+rows);++i){
  const float y=top+step*static_cast<float>(i-first);
  const auto& t=g->jbTracks[static_cast<size_t>(i)];
  const bool sel=i==g->jbRow,playing=g->jbPlaying==i&&g->jbPlayingGame==thisGame;
  if(sel){g->brush->SetColor(rgba(0.12f,0.42f,1.f));g->brush->SetOpacity(0.35f);g->rt->FillRectangle(D2D1::RectF(W*0.39f,y,W*0.92f,y+step),g->brush.Get());g->brush->SetOpacity(1);}
  const std::wstring label=upper(wide(mcl::jukeboxTitle(t,i+1,g->trackTitles)));
  leftText(playing?L"♪":L"",g->fSmall.Get(),W*0.395f,y,W*0.02f,step,rgba(1.f,0.84f,0.25f));
  leftText(label,g->fSmall.Get(),W*0.42f,y,W*0.40f,step,playing?rgba(1.f,0.84f,0.25f):(sel?rgba(1,1,1):rgba(0.78f,0.84f,0.97f)));
  leftText(mmss(t.endMs-t.startMs),g->fSmall.Get(),W*0.83f,y,W*0.08f,step,rgba(0.55f,0.62f,0.78f),DWRITE_TEXT_ALIGNMENT_TRAILING);
 }
 if(n>rows)leftText(std::to_wstring(g->jbRow+1)+L" / "+std::to_wstring(n),g->fSmall.Get(),W*0.39f,top+step*rows,W*0.53f,step*0.9f,rgba(0.5f,0.56f,0.7f),DWRITE_TEXT_ALIGNMENT_TRAILING);
 // Now playing + progress
 const float by=H*0.888f;
 if(g->jbPlaying>=0&&g->jbPlaying<static_cast<int>(g->jbPlayingList.size())){
  const auto& t=g->jbPlayingList[static_cast<size_t>(g->jbPlaying)];
  const int len=t.endMs-t.startMs,pos=std::clamp(g->music.positionMs()-t.startMs,0,len);
  leftText(L"♪  "+upper(wide(mcl::jukeboxTitle(t,g->jbPlaying+1,g->trackTitles)))+L"   —   "+musicGameTitle(t.game),g->fSmall.Get(),W*0.09f,by-H*0.035f,W*0.70f,H*0.03f,rgba(1.f,0.84f,0.25f));
  leftText(mmss(pos)+L" / "+mmss(len),g->fSmall.Get(),W*0.78f,by-H*0.035f,W*0.13f,H*0.03f,rgba(0.8f,0.85f,1.f),DWRITE_TEXT_ALIGNMENT_TRAILING);
  g->brush->SetColor(rgba(0.2f,0.25f,0.4f));g->rt->FillRectangle(D2D1::RectF(W*0.09f,by,W*0.91f,by+H*0.008f),g->brush.Get());
  g->brush->SetColor(rgba(1.f,0.84f,0.25f));g->rt->FillRectangle(D2D1::RectF(W*0.09f,by,W*0.09f+W*0.82f*(len?static_cast<float>(pos)/static_cast<float>(len):0.f),by+H*0.008f),g->brush.Get());
 }else{
  const std::wstring hint=g->settings.musicVolume<=0?L"MUSIC VOLUME IS OFF  —  TURN IT UP IN OPTIONS TO HEAR THE JUKEBOX":L"CHOOSE A TRACK AND PRESS ENTER";
  text(hint,g->fSmall.Get(),D2D1::RectF(0,by-H*0.03f,W,by+H*0.01f),rgba(0.6f,0.66f,0.8f),1.5f,0.9f);
 }
 text(L"←→ SOUNDTRACK      ↑↓ CHOOSE      ENTER  PLAY      BACKSPACE  STOP      ESC  BACK",g->fSmall.Get(),D2D1::RectF(0,H*0.94f,W,H*0.98f),rgba(0.6f,0.66f,0.8f),2,0.85f);
 (void)now;
}
// ---------- GALLERY: Arcade Museum & Character Encyclopedia ----------
// Unlocks for good after the first verified arcade clear of any game (Collection rule).
// Text comes from Gallery\arcade_gallery.mclgallery (tools/build_gallery.py); pictures are
// the user's PS1 Marvel vs. Capcom gallery, installed to Gallery\Art by
// tools/import_gallery_art.py. Characters without a picture show their text only.
const wchar_t* const kGalHub[]={L"ARCADE MUSEUM",L"CHARACTERS",L"ARTWORK"};
std::string artSlug(std::string name){
 const auto paren=name.find(" (");if(paren!=std::string::npos)name=name.substr(0,paren); // Wolverine (Bone Claws) -> Wolverine
 std::string out;for(unsigned char c:name)if(std::isalnum(c))out+=static_cast<char>(std::tolower(c));
 return out;
}
ID2D1Bitmap* galImage(const std::string& slug,const char* file){
 const std::string key=slug+"/"+file;
 auto it=g->galArt.find(key);
 if(it==g->galArt.end()){
  std::error_code ec;const fs::path p=g->dir/"Gallery"/"Art"/slug/file;
  it=g->galArt.emplace(key,fs::is_regular_file(p,ec)?loadPng(p):ComPtr<ID2D1Bitmap>()).first;
 }
 return it->second.Get();
}
// Portrait for a character in a game: the PS1 gallery art when the player has it,
// otherwise that game's own select-screen portrait extracted from the ROM
// (tools/extract_portraits.py -> Art\<slug>\rom_<game>.png), otherwise any game's.
ID2D1Bitmap* charPortrait(const std::string& name,const std::string& game,bool& pixelArt){
 const std::string s=artSlug(name);
 pixelArt=false;
 if(auto* b=galImage(s,"portrait.png"))return b;
 pixelArt=true;
 if(auto* b=galImage(s,("rom_"+game+".png").c_str()))return b;
 static const char* order[]={"xmcota","msh","xmvsf","mshvsf","mvsc","sfexp","sfex2p"};
 for(const char* o:order)if(auto* b=galImage(s,(std::string("rom_")+o+".png").c_str()))return b;
 return nullptr;
}
bool galleryOpen(){return g->cb.launcher&&g->cb.launcher->collection().galleryUnlocked();}
std::vector<const mcl::Character*> galRoster(const std::string& game){
 std::vector<const mcl::Character*> v;
 if(g->cb.launcher)for(const auto& c:g->cb.launcher->collection().characters())if(c.game==game)v.push_back(&c);
 return v;
}
void scanGalleryArt(){
 g->galArtList.clear();
 std::error_code ec;const fs::path dir=g->dir/"Gallery"/"Art";
 if(!fs::is_directory(dir,ec))return;
 for(const auto& e:fs::directory_iterator(dir,ec))if(e.is_directory(ec)&&fs::is_regular_file(e.path()/"full.png",ec))g->galArtList.push_back(e.path().filename().string());
 std::sort(g->galArtList.begin(),g->galArtList.end());
 // group artwork last
 const auto b=std::find(g->galArtList.begin(),g->galArtList.end(),"_bonus");
 if(b!=g->galArtList.end()){g->galArtList.erase(b);g->galArtList.push_back("_bonus");}
}
std::wstring artTitle(const std::string& slug){
 if(slug=="_bonus")return L"GROUP ARTWORK";
 if(g->cb.launcher)for(const auto& c:g->cb.launcher->collection().characters())if(artSlug(c.name)==slug)return upper(wide(c.name));
 return upper(wide(slug));
}
void enterGallery(){g->galMode=0;g->galHub=0;setScreen(Screen::Gallery);}
void galleryBack(){if(galleryOpen()&&g->galMode!=0){g->galMode=0;return;}setScreen(Screen::Menu);}
void galleryPad(Pad p){
 if(p==Pad::Circle){galleryBack();return;}
 if(!galleryOpen())return; // locked: only back works
 const auto& games=g->cb.launcher->collection().galleryGames();
 const int ng=static_cast<int>(games.size());
 switch(g->galMode){
  case 0:
   if(p==Pad::Up)g->galHub=(g->galHub+2)%3;
   else if(p==Pad::Down)g->galHub=(g->galHub+1)%3;
   else if(p==Pad::Cross||p==Pad::Start){
    g->galMode=g->galHub+1;g->galRow=0;
    if(g->galMode==3){scanGalleryArt();g->galArtIdx=0;}
   }
   break;
  case 1: // museum: one page per game
   if(ng&&(p==Pad::Left||p==Pad::Up))g->galGame=(g->galGame+ng-1)%ng;
   if(ng&&(p==Pad::Right||p==Pad::Down))g->galGame=(g->galGame+1)%ng;
   break;
  case 2:{ // characters: game tabs, list + profile
   const int nr=ng?static_cast<int>(galRoster(games[static_cast<size_t>(g->galGame%ng)].id).size()):0;
   if(ng&&p==Pad::Left){g->galGame=(g->galGame+ng-1)%ng;g->galRow=0;}
   if(ng&&p==Pad::Right){g->galGame=(g->galGame+1)%ng;g->galRow=0;}
   if(nr&&p==Pad::Up)g->galRow=(g->galRow+nr-1)%nr;
   if(nr&&p==Pad::Down)g->galRow=(g->galRow+1)%nr;
   break;}
  case 3:{ // artwork viewer
   const int na=static_cast<int>(g->galArtList.size());
   if(na&&(p==Pad::Left||p==Pad::Up))g->galArtIdx=(g->galArtIdx+na-1)%na;
   if(na&&(p==Pad::Right||p==Pad::Down))g->galArtIdx=(g->galArtIdx+1)%na;
   break;}
 }
}
// Wrapped paragraph; returns the height used.
float paragraph(const std::wstring& t,IDWriteTextFormat* f,float x,float y,float w,float maxH,D2D1_COLOR_F c){
 ComPtr<IDWriteTextLayout> l;
 g->dw->CreateTextLayout(t.c_str(),static_cast<UINT32>(t.size()),f,w,maxH,&l);
 if(!l)return 0;
 l->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);l->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
 l->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
 DWRITE_TRIMMING trim{DWRITE_TRIMMING_GRANULARITY_WORD,0,0};l->SetTrimming(&trim,nullptr);
 g->brush->SetColor(c);g->rt->DrawTextLayout(D2D1::Point2F(x,y),l.Get(),g->brush.Get());
 DWRITE_TEXT_METRICS m{};l->GetMetrics(&m);return std::min(m.height,maxH);
}
void drawFitted(ID2D1Bitmap* b,D2D1_RECT_F box,float alpha=1.f,bool pixelArt=false){
 if(!b)return;
 const auto sz=b->GetSize();
 const float s=std::min((box.right-box.left)/sz.width,(box.bottom-box.top)/sz.height);
 const float w=sz.width*s,h=sz.height*s,x=(box.left+box.right-w)/2,y=(box.top+box.bottom-h)/2;
 g->rt->DrawBitmap(b,D2D1::RectF(x,y,x+w,y+h),alpha,pixelArt?D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR:D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
}
void galPanel(){
 const float W=g->W(),H=g->H();
 g->brush->SetColor(rgba(0,0,0.02f));g->brush->SetOpacity(0.86f);
 g->rt->FillRectangle(D2D1::RectF(W*0.06f,H*0.33f,W*0.94f,H*0.93f),g->brush.Get());g->brush->SetOpacity(1);
}
void drawGallery(double now){
 const float W=g->W(),H=g->H();
 galPanel();
 const D2D1_COLOR_F gold=rgba(1.f,0.84f,0.25f),soft=rgba(0.6f,0.66f,0.8f),body=rgba(0.9f,0.93f,1.f);
 if(!galleryOpen()){
  text(L"GALLERY LOCKED",g->fHead.Get(),D2D1::RectF(0,H*0.42f,W,H*0.49f),gold,H*0.004f);
  text(L"BEAT ARCADE MODE IN ANY GAME TO OPEN THE ARCADE MUSEUM AND CHARACTER ENCYCLOPEDIA",g->fSmall.Get(),D2D1::RectF(0,H*0.53f,W,H*0.57f),body,1.5f);
  text(L"THE LEGENDS LIVE ON!",g->fBody.Get(),D2D1::RectF(0,H*0.61f,W,H*0.66f),rgba(1.f,0.45f,0.3f),2.f);
  text(L"ESC  BACK",g->fSmall.Get(),D2D1::RectF(0,H*0.94f,W,H*0.98f),soft,2,0.85f);
  return;
 }
 const auto& col=g->cb.launcher->collection();
 const auto& games=col.galleryGames();
 const bool blink=std::fmod(now,900.0)<600.0;
 if(games.empty()){text(L"GALLERY DATA MISSING  (Gallery\\arcade_gallery.mclgallery)",g->fBody.Get(),D2D1::RectF(0,H*0.5f,W,H*0.56f),body,2);return;}
 g->galGame=std::clamp(g->galGame,0,static_cast<int>(games.size())-1);
 const auto& gm=games[static_cast<size_t>(g->galGame)];
 switch(g->galMode){
  case 0:{
   text(L"GALLERY",g->fHead.Get(),D2D1::RectF(0,H*0.345f,W,H*0.40f),gold,H*0.004f);
   text(L"THE LEGENDS LIVE ON!",g->fSmall.Get(),D2D1::RectF(0,H*0.40f,W,H*0.435f),rgba(1.f,0.45f,0.3f),1.5f);
   static const wchar_t* const sub[]={L"The story of all eight games",L"Bios, backgrounds and portraits for all 95 fighters",L"Original artwork from the PlayStation Marvel vs. Capcom gallery"};
   for(int i=0;i<3;++i){
    const float y=H*0.49f+H*0.115f*static_cast<float>(i);
    const bool sel=i==g->galHub;
    text(kGalHub[i],g->fMenu.Get(),D2D1::RectF(0,y-H*0.04f,W,y+H*0.035f),sel?gold:rgba(0.86f,0.9f,1.f),H*(sel?0.0045f:0.0035f));
    text(sub[i],g->fSmall.Get(),D2D1::RectF(0,y+H*0.035f,W,y+H*0.07f),sel?body:soft,1.5f);
   }
   text(L"↑↓ CHOOSE      ENTER  OPEN      ESC  BACK",g->fSmall.Get(),D2D1::RectF(0,H*0.94f,W,H*0.98f),soft,2,0.85f);
   break;}
  case 1:{
   text((blink?L"◀   ":L"    ")+upper(wide(gm.title))+(blink?L"   ▶":L"    "),g->fHead.Get(),D2D1::RectF(0,H*0.345f,W,H*0.40f),gold,H*0.004f);
   text(wide(gm.year)+L"   •   "+upper(wide(gm.hardware))+L"   •   "+upper(wide(gm.developer)),g->fSmall.Get(),D2D1::RectF(0,H*0.40f,W,H*0.435f),soft,1.5f);
   ID2D1Bitmap* card=g->cards.count(gm.id)?g->cards[gm.id].Get():nullptr;
   const float tx=card?W*0.36f:W*0.09f;
   if(card){drawCover(card,D2D1::RectF(W*0.09f,H*0.46f,W*0.33f,H*0.59f),1.f,1.f);}
   float y=H*0.455f;
   leftText(L"HISTORY",g->fSmall.Get(),tx,y,W*0.3f,H*0.03f,gold);y+=H*0.032f;
   y+=paragraph(wide(gm.history),g->fSmall.Get(),tx,y,W*0.91f-tx,H*0.25f,body)+H*0.02f;
   leftText(L"GAMEPLAY",g->fSmall.Get(),tx,y,W*0.3f,H*0.03f,gold);y+=H*0.032f;
   paragraph(wide(gm.mechanics),g->fSmall.Get(),tx,y,W*0.91f-tx,H*0.90f-y,body);
   text(L"←→ GAME      ESC  BACK",g->fSmall.Get(),D2D1::RectF(0,H*0.94f,W,H*0.98f),soft,2,0.85f);
   break;}
  case 2:{
   const auto roster=galRoster(gm.id);
   text((blink?L"◀   ":L"    ")+upper(wide(gm.title))+(blink?L"   ▶":L"    "),g->fBody.Get(),D2D1::RectF(0,H*0.345f,W,H*0.385f),gold,2.f);
   const int n=static_cast<int>(roster.size()),rows=12;
   g->galRow=std::clamp(g->galRow,0,std::max(0,n-1));
   const int first=std::clamp(g->galRow-rows/2,0,std::max(0,n-rows));
   const float top=H*0.40f,step=H*0.0405f;
   for(int i=first;i<std::min(n,first+rows);++i){
    const float y=top+step*static_cast<float>(i-first);
    const bool sel=i==g->galRow;
    if(sel){g->brush->SetColor(rgba(0.12f,0.42f,1.f));g->brush->SetOpacity(0.35f);g->rt->FillRectangle(D2D1::RectF(W*0.08f,y,W*0.30f,y+step),g->brush.Get());g->brush->SetOpacity(1);}
    bool px=false;const bool pic=charPortrait(roster[static_cast<size_t>(i)]->name,gm.id,px)!=nullptr;
    leftText((pic?L"■ ":L"   ")+upper(wide(roster[static_cast<size_t>(i)]->name)),g->fSmall.Get(),W*0.09f,y,W*0.20f,step,sel?rgba(1,1,1):rgba(0.78f,0.84f,0.97f));
   }
   if(n>rows)leftText(std::to_wstring(g->galRow+1)+L" / "+std::to_wstring(n),g->fSmall.Get(),W*0.08f,top+step*rows,W*0.22f,step*0.9f,rgba(0.5f,0.56f,0.7f),DWRITE_TEXT_ALIGNMENT_TRAILING);
   if(n==0)break;
   const auto* ch=roster[static_cast<size_t>(g->galRow)];
   const auto* pf=col.profile(ch->name);
   // portrait
   bool pixel=false;ID2D1Bitmap* pic=charPortrait(ch->name,gm.id,pixel);
   const D2D1_RECT_F box=D2D1::RectF(W*0.32f,H*0.40f,W*0.47f,H*0.66f);
   g->brush->SetColor(rgba(0.05f,0.08f,0.2f));g->rt->FillRectangle(box,g->brush.Get());
   if(pic)drawFitted(pic,box,1.f,pixel);
   else text(L"NO ARTWORK\nIN YOUR\nCOLLECTION",g->fSmall.Get(),box,rgba(0.45f,0.5f,0.65f),1.f);
   g->brush->SetColor(rgba(0.3f,0.4f,0.7f));g->rt->DrawRectangle(box,g->brush.Get(),1.5f);
   // name + facts
   const float tx=W*0.49f,tw=W*0.43f;
   leftText(upper(wide(ch->name)),g->fBody.Get(),tx,H*0.40f,tw,H*0.045f,rgba(1,1,1));
   std::wstring role=wide(ch->role);
   if(role.find(L"boss")!=std::wstring::npos)role=L"BOSS";else if(role.find(L"hidden")!=std::wstring::npos||role.find(L"regional")!=std::wstring::npos)role=L"SECRET FIGHTER";else role=L"FIGHTER";
   std::wstring apps;
   for(const auto& other:games){
    bool in=false;for(const auto& c:col.characters())if(c.game==other.id&&artSlug(c.name)==artSlug(ch->name))in=true;
    if(in){if(!apps.empty())apps+=L"  •  ";apps+=upper(wide(other.id=="xmcota"?"X-Men COTA":other.id=="msh"?"MSH":other.id=="xmvsf"?"XvSF":other.id=="mshvsf"?"MSHvSF":other.id=="mvsc"?"MvC":other.id=="mvsc2"?"MvC2":other.id=="sfexp"?"EX Plus":"EX2 Plus"));}
   }
   leftText((pf?upper(wide(pf->universe)):L"")+L"   •   "+role,g->fSmall.Get(),tx,H*0.445f,tw,H*0.03f,gold);
   leftText(L"APPEARS IN:  "+apps,g->fSmall.Get(),tx,H*0.475f,tw,H*0.03f,soft);
   float y=H*0.515f;
   if(pf){
    leftText(L"BIO",g->fSmall.Get(),tx,y,tw,H*0.03f,gold);y+=H*0.03f;
    paragraph(wide(pf->bio),g->fSmall.Get(),tx,y,tw,H*0.66f-y,body);
    y=H*0.68f;
    leftText(L"BACKGROUND",g->fSmall.Get(),W*0.32f,y,W*0.6f,H*0.03f,gold);y+=H*0.03f;
    y+=paragraph(wide(pf->story),g->fSmall.Get(),W*0.32f,y,W*0.60f,H*0.86f-y,body)+H*0.012f;
   }
   paragraph(L"In "+wide(gm.title)+L": "+wide(ch->notes),g->fSmall.Get(),W*0.32f,std::max(y,H*0.855f),W*0.60f,H*0.92f-std::max(y,H*0.855f),soft);
   text(L"←→ GAME      ↑↓ CHARACTER      ESC  BACK      ■ = ARTWORK",g->fSmall.Get(),D2D1::RectF(0,H*0.94f,W,H*0.98f),soft,2,0.85f);
   break;}
  case 3:{
   const int na=static_cast<int>(g->galArtList.size());
   if(!na){
    text(L"NO ARTWORK INSTALLED",g->fHead.Get(),D2D1::RectF(0,H*0.5f,W,H*0.56f),gold,2);
    text(L"Run tools\\import_gallery_art.py to add your PS1 gallery images",g->fSmall.Get(),D2D1::RectF(0,H*0.58f,W,H*0.62f),body,1.5f);
    break;
   }
   g->galArtIdx=std::clamp(g->galArtIdx,0,na-1);
   const auto& slug=g->galArtList[static_cast<size_t>(g->galArtIdx)];
   text((blink?L"◀   ":L"    ")+artTitle(slug)+L"   "+std::to_wstring(g->galArtIdx+1)+L" / "+std::to_wstring(na)+(blink?L"   ▶":L"    "),g->fBody.Get(),D2D1::RectF(0,H*0.345f,W,H*0.385f),gold,2.f);
   drawFitted(galImage(slug,"full.png"),D2D1::RectF(W*0.1f,H*0.395f,W*0.9f,H*0.90f));
   text(L"←→ BROWSE      ESC  BACK      ARTWORK: MARVEL VS. CAPCOM (PLAYSTATION) GALLERY",g->fSmall.Get(),D2D1::RectF(0,H*0.94f,W,H*0.98f),soft,2,0.85f);
   break;}
 }
}
// "GALLERY UNLOCKED! THE LEGENDS LIVE ON!" after the first arcade clear (shown on return).
void drawGalleryUnlock(double now){
 if(g->cb.launcher&&!g->gameRunning&&g->cb.launcher->takeGalleryUnlock()){g->galleryBannerUntil=now+5000;g->bannerFlash=1.f;playVoice();}
 if(now>=g->galleryBannerUntil)return;
 const float W=g->W(),H=g->H();
 const float k=static_cast<float>((g->galleryBannerUntil-now)/5000.0);
 const float a=std::min(1.f,k*4);
 g->brush->SetColor(rgba(0,0,0.03f));g->brush->SetOpacity(0.7f*a);
 g->rt->FillRectangle(D2D1::RectF(0,H*0.33f,W,H*0.62f),g->brush.Get());g->brush->SetOpacity(1);
 text(L"GALLERY UNLOCKED!",g->fBanner.Get(),D2D1::RectF(0,H*0.35f,W,H*0.5f),rgba(1.f,0.84f,0.25f),H*0.006f,a);
 text(L"THE LEGENDS LIVE ON!",g->fHead.Get(),D2D1::RectF(0,H*0.5f,W,H*0.58f),rgba(1.f,0.45f,0.3f),H*0.004f,a);
}
// ---------- Controllers: DirectInput pads in the menus ----------
// PlayStation/Nintendo/generic pads (Xbox pads use XInput above). Menu buttons follow the
// detected family (Sony: Cross/Circle/Share/Options, Nintendo: A/B/-/+) unless the player
// set their own in CONTROLS SETUP.
void pollDirectPads(){
 const int n=g->pads.count();
 g->diPrev.resize(static_cast<size_t>(n),0);g->diStickPrev.resize(static_cast<size_t>(n),0);
 for(int i=0;i<n;++i){
  const auto& info=g->pads.info(i);
  if(info.isXInput)continue;
  mcl::PadSnapshot s;if(!g->pads.poll(i,s))continue;
  const auto def=mcl::defaultMenuButtons(info.vid,info.pid);
  const auto pick=[](int mine,int dflt){return mine>=0?mine:dflt;};
  const int bc=pick(g->controls.menuConfirm,def.confirm),bb=pick(g->controls.menuBack,def.back);
  const int bs=pick(g->controls.menuSelect,def.select),bt=pick(g->controls.menuStart,def.start);
  const auto bit=[&](int k){return k>=0&&k<64&&(s.buttons&(std::uint64_t(1)<<k));};
  std::uint64_t now=0; // packed: 0 up 1 down 2 left 3 right 4 confirm 5 back 6 select 7 start
  if(s.up)now|=1;if(s.down)now|=2;if(s.left)now|=4;if(s.right)now|=8;
  if(bit(bc))now|=16;if(bit(bb))now|=32;if(bit(bs))now|=64;if(bit(bt))now|=128;
  const std::uint64_t pressed=now&~g->diPrev[static_cast<size_t>(i)];g->diPrev[static_cast<size_t>(i)]=now;
  if(pressed&1)onPad(Pad::Up);if(pressed&2)onPad(Pad::Down);if(pressed&4)onPad(Pad::Left);if(pressed&8)onPad(Pad::Right);
  if(pressed&16)onPad(Pad::Cross);if(pressed&32)onPad(Pad::Circle);if(pressed&64)onPad(Pad::Select);if(pressed&128)onPad(Pad::Start);
 }
}
// ---------- CONTROLS SETUP ----------
// Row 0 picks the game set: MAME GAMES (keyboard + controller columns, then the 4 menu
// buttons) or MARVEL VS. CAPCOM 2 (Flycast: its own keyboard layout; controllers use
// Flycast's arcade layout). Then RESET TO DEFAULTS and BACK. Enter = assign, Backspace = clear.
const wchar_t* const kMenuRows[]={L"MENU: CONFIRM",L"MENU: BACK",L"MENU: SELECT",L"MENU: START"};
enum class CsRow { Game, Player, Input, Menu, Reset, Back };
bool csMvc2(){return g->csGame==1;}
mcl::ControlsConfig& csCfg(){return csMvc2()?g->mvc2Controls:g->controls;}
constexpr int kInputsPerPlayer=12;
int csGameRows(){return kInputsPerPlayer;}
const mcl::GameInput& csInput(int i){return mcl::gameInputs()[static_cast<size_t>(g->csPlayer*kInputsPerPlayer+i)];}
bool csMenuRows(){return !csMvc2()&&g->csPlayer==0;} // controller menu buttons: shown on the Player 1 page
int csRows(){return 2+csGameRows()+(csMenuRows()?4:0)+2;}
CsRow csKind(int r,int& index){
 index=0;
 if(r==0)return CsRow::Game;
 if(r==1)return CsRow::Player;
 r-=2;if(r<csGameRows()){index=r;return CsRow::Input;}
 r-=csGameRows();
 if(csMenuRows()){if(r<4){index=r;return CsRow::Menu;}r-=4;}
 return r==0?CsRow::Reset:CsRow::Back;
}
// MvC2 controller rows: directions are always D-pad + stick and COIN is unused (FREE PLAY).
bool csPadFixed(int input){
 // MvC2 player 2's controller is the 2nd pad, with the same buttons as player 1.
 if(g->csPlayer==1)return true;
 const std::string port=csInput(input).port;
 return port.find("_JOYSTICK_")!=std::string::npos||port.rfind("COIN",0)==0;
}
// MvC2: the next controller button pressed, by name (PAD_A...), "" if none. XInput pads are read
// by name; DirectInput pads by their family's numbering (PlayStation, Switch, otherwise Xbox order).
std::string namedPadPress(bool begin){
 std::string hit;
 static const std::pair<WORD,const char*> xb[]={{XINPUT_GAMEPAD_A,"PAD_A"},{XINPUT_GAMEPAD_B,"PAD_B"},{XINPUT_GAMEPAD_X,"PAD_X"},{XINPUT_GAMEPAD_Y,"PAD_Y"},
  {XINPUT_GAMEPAD_LEFT_SHOULDER,"PAD_LB"},{XINPUT_GAMEPAD_RIGHT_SHOULDER,"PAD_RB"},{XINPUT_GAMEPAD_START,"PAD_START"},
  {XINPUT_GAMEPAD_LEFT_THUMB,"PAD_LS"},{XINPUT_GAMEPAD_RIGHT_THUMB,"PAD_RS"}};
 for(DWORD i=0;i<XUSER_MAX_COUNT;++i){
  XINPUT_STATE st{};WORD now=0;
  if(XInputGetState(i,&st)==ERROR_SUCCESS){
   now=st.Gamepad.wButtons;
   if(st.Gamepad.bLeftTrigger>150)now|=0x0400; // spare bits for the triggers
   if(st.Gamepad.bRightTrigger>150)now|=0x0800;
  }
  const WORD pressed=static_cast<WORD>(now&~g->capXB[i]);g->capXB[i]=now;
  if(begin||!hit.empty())continue;
  for(const auto& b:xb)if(pressed&b.first){hit=b.second;break;}
  if(hit.empty()&&(pressed&0x0400))hit="PAD_LT";
  if(hit.empty()&&(pressed&0x0800))hit="PAD_RT";
 }
 static const char* const sony[]={"PAD_X","PAD_A","PAD_B","PAD_Y","PAD_LB","PAD_RB","PAD_LT","PAD_RT","","PAD_START","PAD_LS","PAD_RS"};
 static const char* const nintendo[]={"PAD_B","PAD_A","PAD_Y","PAD_X","PAD_LB","PAD_RB","PAD_LT","PAD_RT","","PAD_START","PAD_LS","PAD_RS"};
 static const char* const xbox[]={"PAD_A","PAD_B","PAD_X","PAD_Y","PAD_LB","PAD_RB","","PAD_START","PAD_LS","PAD_RS"};
 const int n=g->pads.count();g->capDi.resize(static_cast<size_t>(n),0);
 for(int i=0;i<n;++i){
  const auto& info=g->pads.info(i);if(info.isXInput)continue;
  mcl::PadSnapshot snap;if(!g->pads.poll(i,snap))continue;
  const std::uint64_t pressed=snap.buttons&~g->capDi[static_cast<size_t>(i)];g->capDi[static_cast<size_t>(i)]=snap.buttons;
  if(begin||!hit.empty())continue;
  const char* const* table=info.vid==0x054C?sony:info.vid==0x057E?nintendo:xbox;
  const int size=info.vid==0x054C||info.vid==0x057E?12:10;
  for(int k=0;k<size;++k)if((pressed&(std::uint64_t(1)<<k))&&table[k][0]){hit=table[k];break;}
 }
 return hit;
}
void csSave(){
 const bool ok=csMvc2()?g->mvc2Controls.save(g->dir/"MCL_UserData"/"controls_mvc2.txt"):g->controls.save(g->dir/"MCL_UserData"/"controls.txt");
 if(ok)g->savedAt=g->now();
}
int& csMenuSlot(int i){return i==0?g->controls.menuConfirm:i==1?g->controls.menuBack:i==2?g->controls.menuSelect:g->controls.menuStart;}
void csAssign(const std::string& code){
 int i=0;const CsRow k=csKind(g->csRow,i);
 if(k==CsRow::Input){
  auto& b=csCfg().binding(csInput(i).port);
  if(g->csCapture==1)b.key=code;else b.pad=code;
 }else if(k==CsRow::Menu){
  const auto at=code.find("_BUTTON");
  if(at==std::string::npos)return; // menu buttons must be buttons
  try{csMenuSlot(i)=std::stoi(code.substr(at+7))-1;}catch(...){return;}
 }
 g->csCapture=0;csSave();
}
void controlSetupPad(Pad p){
 if(g->csCapture)return;
 const int n=csRows();
 int i=0;const CsRow k=csKind(g->csRow,i);
 switch(p){
  case Pad::Up: g->csRow=(g->csRow+n-1)%n;break;
  case Pad::Down: g->csRow=(g->csRow+1)%n;break;
  case Pad::Left: case Pad::Right:
   if(k==CsRow::Game){g->csGame=1-g->csGame;g->csCol=0;}
   else if(k==CsRow::Player){g->csPlayer=1-g->csPlayer;g->csCol=0;}
   else if(k==CsRow::Input&&!(csMvc2()&&csPadFixed(i)))g->csCol=1-g->csCol;
   break;
  case Pad::Cross: case Pad::Start:
   if(k==CsRow::Game){g->csGame=1-g->csGame;g->csCol=0;}
   else if(k==CsRow::Player){g->csPlayer=1-g->csPlayer;g->csCol=0;}
   else if(k==CsRow::Input){
    if(g->csCol==0)g->csCapture=1;
    else if(!(csMvc2()&&csPadFixed(i)))g->csCapture=2;
   }
   else if(k==CsRow::Menu){g->csCapture=2;}
   else if(k==CsRow::Reset){
    if(csMvc2())g->mvc2Controls.resetDefaults();
    else{const int c=g->controls.menuConfirm,b=g->controls.menuBack,s=g->controls.menuSelect,t=g->controls.menuStart;(void)c;(void)b;(void)s;(void)t;g->controls.resetDefaults();}
    csSave();showBanner(L"CONTROLS RESET");
   }
   else{setScreen(Screen::Options);}
   if(g->csCapture){g->csCaptureSince=g->now();if(g->csCapture==2){g->pads.rescan();g->pads.resetCapture();if(csMvc2())namedPadPress(true);}}
   break;
  case Pad::Select:
   if(k==CsRow::Input&&!(g->csCol==1&&csMvc2()&&csPadFixed(i))){auto& b=csCfg().binding(csInput(i).port);(g->csCol==0?b.key:b.pad).clear();csSave();}
   else if(k==CsRow::Menu){csMenuSlot(i)=-1;csSave();}
   break;
  case Pad::Circle: setScreen(Screen::Options);break;
 }
}
void drawControlSetup(double now){
 const float W=g->W(),H=g->H();
 const bool mvc2=csMvc2();
 g->brush->SetColor(rgba(0,0,0.02f));g->brush->SetOpacity(0.86f);
 g->rt->FillRectangle(D2D1::RectF(W*0.06f,H*0.33f,W*0.94f,H*0.93f),g->brush.Get());g->brush->SetOpacity(1);
 text(L"CONTROLS SETUP",g->fHead.Get(),D2D1::RectF(0,H*0.335f,W,H*0.385f),rgba(1.f,0.84f,0.25f),H*0.004f);
 // connected controllers
 std::wstring pads;
 for(int i=0;i<g->pads.count();++i){if(!pads.empty())pads+=L"   •   ";pads+=L"PAD "+std::to_wstring(i+1)+L": "+upper(g->pads.info(i).name);}
 if(pads.empty())pads=L"NO CONTROLLER DETECTED  (KEYBOARD ONLY)";
 leftText(pads,g->fSmall.Get(),W*0.08f,H*0.385f,W*0.84f,H*0.03f,rgba(0.55f,0.62f,0.78f),DWRITE_TEXT_ALIGNMENT_CENTER);
 const float top=H*0.45f,step=H*0.031f;
 leftText(L"KEYBOARD",g->fSmall.Get(),W*0.42f,top-step,W*0.2f,step,rgba(1.f,0.84f,0.25f));
 leftText(L"CONTROLLER",g->fSmall.Get(),W*0.66f,top-step,W*0.27f,step,rgba(1.f,0.84f,0.25f));
 const int n=csRows(),rows=14;
 const int first=std::clamp(g->csRow-rows/2,0,std::max(0,n-rows));
 const bool blink=std::fmod(now,600.0)<350.0;
 // MvC2 rows that can't be changed: directions (D-pad + left stick) and COIN (the game is on FREE PLAY).
 static const wchar_t* const flyPad[]={L"D-PAD / STICK",L"D-PAD / STICK",L"D-PAD / STICK",L"D-PAD / STICK",L"",L"",L"",L"",L"",L"",L"",L"- (FREE PLAY)"};
 for(int r=first;r<std::min(n,first+rows);++r){
  const float y=top+step*static_cast<float>(r-first);
  const bool sel=r==g->csRow;
  if(sel){g->brush->SetColor(rgba(0.12f,0.42f,1.f));g->brush->SetOpacity(0.30f);g->rt->FillRectangle(D2D1::RectF(W*0.08f,y,W*0.92f,y+step),g->brush.Get());g->brush->SetOpacity(1);}
  int i=0;const CsRow k=csKind(r,i);
  std::wstring label,key,pad;bool keyCell=false,padCell=false,padFixed=false;
  if(k==CsRow::Game){
   label=L"GAME";key=mvc2?L"◀ MARVEL VS. CAPCOM 2 ▶":L"◀ MAME GAMES ▶";
  }else if(k==CsRow::Player){
   label=L"PLAYER";key=g->csPlayer?L"◀ PLAYER 2 ▶":L"◀ PLAYER 1 ▶";
  }else if(k==CsRow::Input){
   const auto& gi=csInput(i);const auto& b=csCfg().binding(gi.port);
   label=wide(mvc2?mcl::mvc2Label(gi.port):gi.label);key=wide(mcl::inputLabel(b.key));keyCell=true;
   pad=wide(mcl::inputLabel(b.pad));
   if(mvc2&&csPadFixed(i)){pad=g->csPlayer?(i==11?L"- (FREE PLAY)":L"2ND CONTROLLER, SAME BUTTONS"):flyPad[i];padFixed=true;}else padCell=true;
  }else if(k==CsRow::Menu){
   label=kMenuRows[i];const int v=csMenuSlot(i);pad=v<0?L"AUTO":L"BUTTON "+std::to_wstring(v+1);padCell=true;
  }else if(k==CsRow::Reset){label=L"RESET TO DEFAULTS";}
  else{label=L"BACK";}
  leftText(label,g->fSmall.Get(),W*0.10f,y,W*0.30f,step,sel?rgba(1,1,1):rgba(0.8f,0.86f,0.98f));
  const auto cell=[&](const std::wstring& t,int col,float x){
   const bool on=sel&&(k==CsRow::Input?g->csCol==col:k==CsRow::Menu?col==1:false);
   std::wstring v=t;
   if(on&&g->csCapture)v=blink?(col==0?L"PRESS A KEY...":L"PRESS A BUTTON..."):L"";
   else if(on)v=L"▶ "+t;
   leftText(v,g->fSmall.Get(),x,y,W*0.26f,step,on?rgba(1.f,0.84f,0.25f):rgba(0.65f,0.75f,0.95f));
  };
  if(k==CsRow::Game||k==CsRow::Player)leftText(key,g->fSmall.Get(),W*0.42f,y,W*0.48f,step,sel?rgba(1.f,0.84f,0.25f):rgba(0.65f,0.75f,0.95f));
  if(keyCell)cell(key,0,W*0.42f);
  if(padCell)cell(pad,1,W*0.66f);
  if(padFixed)leftText(pad,g->fSmall.Get(),W*0.66f,y,W*0.26f,step,rgba(0.5f,0.56f,0.7f));
 }
 const std::wstring hint=g->csCapture?L"PRESS THE KEY / BUTTON TO USE      ESC  CANCEL      (8 SECONDS)":
  L"↑↓ CHOOSE      ←→ KEYBOARD / CONTROLLER      ENTER  ASSIGN      BACKSPACE  CLEAR      ESC  BACK";
 text(hint,g->fSmall.Get(),D2D1::RectF(0,H*0.94f,W,H*0.98f),rgba(0.6f,0.66f,0.8f),2,0.85f);
 text(mvc2?L"MARVEL VS. CAPCOM 2 ONLY (FLYCAST)  •  TAB / BACK BUTTON = PAUSE":L"APPLIES TO ALL MAME GAMES  •  CHOOSE MARVEL VS. CAPCOM 2 IN THE GAME ROW FOR ITS OWN KEYS",
  g->fSmall.Get(),D2D1::RectF(0,H*0.895f,W,H*0.925f),rgba(0.5f,0.56f,0.7f),1.5f,0.9f);
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
 if(g->screen==Screen::Intro){
  drawIntro(now);
  if(g->rt->EndDraw()==D2DERR_RECREATE_TARGET){g->rt.Reset();g->dc.Reset();g->brush.Reset();g->logo.Reset();g->videoBmp.Reset();g->previewBmp.Reset();g->cards.clear();g->mvcFont.Reset();g->galArt.clear();}
  return;
 }
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
  case Screen::Achievements: drawAchievements(now);break;
  case Screen::ControlSetup: drawControlSetup(now);break;
  case Screen::Access: drawAccess(now);break;
  case Screen::Gallery: drawGallery(now);break;
  case Screen::Jukebox: drawJukebox(now);break;
 }
 drawNowPlaying(now);
 if(g->flyPause||g->flyExiting)drawFlyPause();
 if(!g->gameRunning)drawUpdatePanel(now);
 if(g->gameLoading)drawLoading(now);
 if(!g->overlay.empty())drawOverlay();
 if(now<g->bannerUntil){
  const float k=static_cast<float>((g->bannerUntil-now)/2200.0);
  text(g->banner,g->fBanner.Get(),D2D1::RectF(0,H*0.35f,W,H*0.5f),rgba(1.f,0.84f,0.25f),H*0.006f,std::min(1.f,k*3));
 }
 drawGalleryUnlock(now);
 // "AUTO-SAVED" toast, bottom left
 {const double t=now-g->savedAt;
  if(t>=0&&t<1800){const float a=static_cast<float>(std::min(1.0,std::min(t/150.0,(1800-t)/400.0)));
   text(L"✔ AUTO-SAVED",g->fSmall.Get(),D2D1::RectF(W*0.02f,H*0.94f,W*0.25f,H*0.98f),rgba(0.55f,0.95f,0.6f),1.5f,a);}}
 drawAchievementPopup(now);
 // Calmer flashes behind the cards so the previews stay watchable.
 {const double wf=(now-g->whiteFlashStart)/700.0;
  if(wf>=0&&wf<1){g->brush->SetColor(rgba(1,1,1));g->brush->SetOpacity(static_cast<float>(1.0-wf));g->rt->FillRectangle(D2D1::RectF(0,0,W,H),g->brush.Get());g->brush->SetOpacity(1);}}
 const float flash=std::max(g->logoBolts.flash()*(g->screen==Screen::Select?0.35f:1.f),g->bannerFlash);
 if(flash>0.01f){g->brush->SetColor(rgba(0.75f,0.92f,1.f));g->brush->SetOpacity(flash*0.35f);g->rt->FillRectangle(D2D1::RectF(0,0,W,H),g->brush.Get());g->brush->SetOpacity(1);}
 g->bannerFlash=std::max(0.f,g->bannerFlash-0.06f);
 if(g->rt->EndDraw()==D2DERR_RECREATE_TARGET){g->rt.Reset();g->dc.Reset();g->brush.Reset();g->logo.Reset();g->videoBmp.Reset();g->previewBmp.Reset();g->cards.clear();g->mvcFont.Reset();g->galArt.clear();}
}

// ---------- input ----------
void pollDirectPads();
void pollPad(){
 if(g->screen==Screen::ControlSetup&&g->csCapture){ // capturing: buttons go to the setup, not the menus
  if(g->csCapture==2){const std::string c=csMvc2()?namedPadPress(false):g->pads.capturePress();if(!c.empty())csAssign(c);}
  if(g->now()-g->csCaptureSince>8000)g->csCapture=0;
  WORD held=0;for(DWORD i=0;i<XUSER_MAX_COUNT;++i){XINPUT_STATE s{};if(XInputGetState(i,&s)==ERROR_SUCCESS)held|=s.Gamepad.wButtons;}
  g->padPrev=held;
  return;
 }
 pollDirectPads();
 // Every connected Xbox-style pad drives the menus (buttons combined, strongest stick wins).
 WORD b=0;SHORT y=0;bool any=false;
 for(DWORD i=0;i<XUSER_MAX_COUNT;++i){
  XINPUT_STATE st{};if(XInputGetState(i,&st)!=ERROR_SUCCESS)continue;
  any=true;b|=st.Gamepad.wButtons;if(std::abs(st.Gamepad.sThumbLY)>std::abs(y))y=st.Gamepad.sThumbLY;
 }
 if(!any){g->padPrev=0;g->stickPrev=0;return;}
 const WORD pressed=static_cast<WORD>(b&~g->padPrev);g->padPrev=b;
 if(pressed&XINPUT_GAMEPAD_DPAD_UP)onPad(Pad::Up);
 if(pressed&XINPUT_GAMEPAD_DPAD_DOWN)onPad(Pad::Down);
 if(pressed&XINPUT_GAMEPAD_DPAD_LEFT)onPad(Pad::Left);
 if(pressed&XINPUT_GAMEPAD_DPAD_RIGHT)onPad(Pad::Right);
 if((pressed&XINPUT_GAMEPAD_X)&&!g->gameRunning&&g->overlay.empty()&&!g->updPanel)cheatShortcut(false);
 if((pressed&XINPUT_GAMEPAD_Y)&&!g->gameRunning&&g->overlay.empty()&&!g->updPanel)cheatShortcut(true);
 if(pressed&XINPUT_GAMEPAD_A)onPad(Pad::Cross);
 if(pressed&XINPUT_GAMEPAD_B)onPad(Pad::Circle);
 if(pressed&XINPUT_GAMEPAD_BACK)onPad(Pad::Select);
 if(pressed&XINPUT_GAMEPAD_START)onPad(Pad::Start);
 const int dir=y>20000?1:(y<-20000?-1:0);
 if(dir!=g->stickPrev){if(dir==1)onPad(Pad::Up);else if(dir==-1)onPad(Pad::Down);g->stickPrev=dir;}
}
// ---------- MvC2 pause screen (Flycast) ----------
// Back/Select newly pressed on any pad, or Tab, while Flycast's window has the focus.
// ---------- MvC2 1-button moves (ACCESSIBILITY) ----------
// Windows key for a MAME key name from CONTROLS SETUP ("KEYCODE_LCONTROL" -> VK_LCONTROL).
int vkFromMameKey(const std::string& name){
 static std::map<std::string,int> m;
 if(m.empty()){
  for(int vk=1;vk<255;++vk){const std::string n=mcl::mameKeyName(vk);if(!n.empty()&&!m.count(n))m[n]=vk;}
  for(int vk:{VK_LSHIFT,VK_RSHIFT,VK_LCONTROL,VK_RCONTROL,VK_LMENU,VK_RMENU})m[mcl::mameKeyName(vk)]=vk; // left/right exactly
 }
 const auto it=m.find(name);return it==m.end()?0:it->second;
}
bool keyHeld(const std::string& seq){
 for(size_t at=0;;){
  const size_t end=seq.find(" OR ",at);
  const int vk=vkFromMameKey(seq.substr(at,end==std::string::npos?std::string::npos:end-at));
  if(vk&&(GetAsyncKeyState(vk)&0x8000))return true;
  if(end==std::string::npos)return false;at=end+4;
 }
}
// Named controller buttons held on the player's pad (pads in order: Xbox-style first, then others).
bool padHeld(int player,const std::string& name){
 if(name.empty())return false;
 int k=0;
 for(DWORD i=0;i<XUSER_MAX_COUNT;++i){
  XINPUT_STATE st{};if(XInputGetState(i,&st)!=ERROR_SUCCESS)continue;
  if(k++!=player)continue;
  const WORD b=st.Gamepad.wButtons;
  if(name=="PAD_A")return b&XINPUT_GAMEPAD_A;if(name=="PAD_B")return b&XINPUT_GAMEPAD_B;
  if(name=="PAD_X")return b&XINPUT_GAMEPAD_X;if(name=="PAD_Y")return b&XINPUT_GAMEPAD_Y;
  if(name=="PAD_LB")return b&XINPUT_GAMEPAD_LEFT_SHOULDER;if(name=="PAD_RB")return b&XINPUT_GAMEPAD_RIGHT_SHOULDER;
  if(name=="PAD_LS")return b&XINPUT_GAMEPAD_LEFT_THUMB;if(name=="PAD_RS")return b&XINPUT_GAMEPAD_RIGHT_THUMB;
  if(name=="PAD_START")return b&XINPUT_GAMEPAD_START;
  if(name=="PAD_LT")return st.Gamepad.bLeftTrigger>150;if(name=="PAD_RT")return st.Gamepad.bRightTrigger>150;
  return false;
 }
 static const char* const sony[]={"PAD_X","PAD_A","PAD_B","PAD_Y","PAD_LB","PAD_RB","PAD_LT","PAD_RT","","PAD_START","PAD_LS","PAD_RS"};
 static const char* const nintendo[]={"PAD_B","PAD_A","PAD_Y","PAD_X","PAD_LB","PAD_RB","PAD_LT","PAD_RT","","PAD_START","PAD_LS","PAD_RS"};
 static const char* const xbox[]={"PAD_A","PAD_B","PAD_X","PAD_Y","PAD_LB","PAD_RB","","PAD_START","PAD_LS","PAD_RS"};
 for(int i=0;i<g->pads.count();++i){
  const auto& info=g->pads.info(i);if(info.isXInput)continue;
  if(k++!=player)continue;
  mcl::PadSnapshot snap;if(!g->pads.poll(i,snap))return false;
  const char* const* t=info.vid==0x054C?sony:info.vid==0x057E?nintendo:xbox;
  const int n=info.vid==0x054C||info.vid==0x057E?12:10;
  for(int b=0;b<n;++b)if(name==t[b])return (snap.buttons>>b)&1;
  return false;
 }
 return false;
}
// The player's real LP/HP/LK/HK (keyboard keys + controller buttons set for MvC2).
unsigned mvc2Held(int player){
 static const std::pair<const char*,unsigned> btn[]={{"_BUTTON1",mcl::kLP},{"_BUTTON2",mcl::kHP},{"_BUTTON4",mcl::kLK},{"_BUTTON5",mcl::kHK}};
 unsigned h=0;
 for(const auto& b:btn){
  const std::string port=std::string("P")+(player?"2":"1")+b.first;
  // Player 2's controller uses the same buttons as player 1's (the MvC2 page shows it so).
  if(keyHeld(g->mvc2Controls.binding(port).key)||padHeld(player,g->mvc2Controls.binding(std::string("P1")+b.first).pad))h|=b.second;
 }
 return h;
}
void mvc2OneButtonTick(){
 auto* L=g->cb.launcher;
 const auto post=[L](const mcl::InjectKey& k,bool down){L->postGameKey(k.vk,k.scancode,down);};
 const HWND fw=L->gameWindow();
 if(!fw||GetForegroundWindow()!=fw){g->mvc2One.releaseAll(post);return;} // only while MvC2 has the focus
 const unsigned held[2]={mvc2Held(0),mvc2Held(1)};
 g->mvc2One.tick(g->now(),held,[L](std::uint32_t off,void* out,std::size_t n){return L->readGameRam(off,out,n);},post);
}
bool flyPauseTriggered(){
 const HWND fw=g->cb.launcher?g->cb.launcher->gameWindow():nullptr;
 const bool front=fw&&GetForegroundWindow()==fw;
 // Keyboard only (Tab): on controllers Back/Select stays out of the way.
 bool hit=false;
 const bool tab=(GetAsyncKeyState(VK_TAB)&0x8000)!=0;
 if(tab&&!g->flyPrevTab)hit=true;
 g->flyPrevTab=tab;
 return hit&&front;
}
void openFlyPause(){
 auto* L=g->cb.launcher;const HWND fw=L->gameWindow();
 g->mvc2One.releaseAll([L](const mcl::InjectKey& k,bool down){L->postGameKey(k.vk,k.scancode,down);});
 Sleep(100);
 g->flyMenuOpen=L->gameAdvancing()==0; // the game stopped: Flycast's own menu came up
 // Take the focus while Flycast can still answer, then freeze it (no sound, no input).
 const DWORD me=GetCurrentThreadId(),them=fw?GetWindowThreadProcessId(fw,nullptr):0;
 if(them)AttachThreadInput(me,them,TRUE);
 SetWindowPos(g->hwnd,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
 SetForegroundWindow(g->hwnd);SetFocus(g->hwnd);
 if(them)AttachThreadInput(me,them,FALSE);
 L->freezeGame(true);
 g->flyPause=true;g->flyConfirm=false;g->flyRow=0;
 g->padPrev=0xFFFF;std::fill(g->diPrev.begin(),g->diPrev.end(),~std::uint64_t(0)); // ignore buttons still held
}
void closeFlyPause(bool exitGame){
 auto* L=g->cb.launcher;
 g->flyPause=false;g->flyConfirm=false;
 L->freezeGame(false);
 if(exitGame){g->flyExiting=true;L->requestClose();return;} // stay on top until Flycast has closed
 if(g->flyMenuOpen){
  // Tab closes Flycast's menu again; check the game is running and retry once if not.
  L->tapGameKey(VK_TAB);Sleep(200);
  if(L->gameAdvancing()==0)L->tapGameKey(VK_TAB);
 }
 SetWindowPos(g->hwnd,HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
 L->bringToFront();
 g->flyPrevTab=(GetAsyncKeyState(VK_TAB)&0x8000)!=0;
}
void flyPausePad(Pad p){
 const bool vertical=p==Pad::Up||p==Pad::Down||(g->flyConfirm&&(p==Pad::Left||p==Pad::Right));
 if(vertical){g->flyRow^=1;return;}
 if(!g->flyConfirm){
  if(p==Pad::Cross||p==Pad::Start){if(g->flyRow==0)closeFlyPause(false);else{g->flyConfirm=true;g->flyRow=1;}}
  else if(p==Pad::Circle||p==Pad::Select)closeFlyPause(false);
 }else{
  if(p==Pad::Cross){if(g->flyRow==0)closeFlyPause(true);else{g->flyConfirm=false;g->flyRow=1;}}
  else if(p==Pad::Circle||p==Pad::Select){g->flyConfirm=false;g->flyRow=1;}
 }
}
void drawFlyPause(){
 const float W=g->W(),H=g->H();
 g->brush->SetColor(rgba(0,0,0));g->brush->SetOpacity(0.82f);g->rt->FillRectangle(D2D1::RectF(0,0,W,H),g->brush.Get());
 const D2D1_RECT_F panel=D2D1::RectF(W*0.3f,H*0.28f,W*0.7f,H*0.74f);
 g->brush->SetColor(rgba(0.03f,0.05f,0.13f));g->brush->SetOpacity(0.96f);g->rt->FillRectangle(panel,g->brush.Get());g->brush->SetOpacity(1);
 g->brush->SetColor(rgba(1.f,0.45f,0.25f));g->rt->DrawRectangle(panel,g->brush.Get(),3.f);
 if(g->flyExiting){text(L"EXITING...",g->fHead.Get(),panel,rgba(1.f,0.84f,0.25f),3);return;}
 text(g->flyConfirm?L"EXIT GAME?":L"PAUSED",g->fHead.Get(),D2D1::RectF(panel.left,panel.top+H*0.03f,panel.right,panel.top+H*0.11f),rgba(1.f,0.84f,0.25f),3);
 text(L"MARVEL VS. CAPCOM 2",g->fSmall.Get(),D2D1::RectF(panel.left,panel.top+H*0.11f,panel.right,panel.top+H*0.15f),rgba(0.75f,0.82f,1.f),1.5f);
 const wchar_t* items[2]={g->flyConfirm?L"YES":L"CONTINUE",g->flyConfirm?L"NO":L"EXIT GAME"};
 for(int i=0;i<2;++i){
  const float top=panel.top+H*(0.19f+0.09f*i);const bool on=i==g->flyRow;
  text(items[i],g->fMenu.Get(),D2D1::RectF(panel.left,top,panel.right,top+H*0.08f),on?rgba(1.f,0.84f,0.25f):rgba(0.8f,0.84f,0.95f),on?4.f:2.f,on?1.f:0.7f);
 }
 text(L"A / ENTER  SELECT     B / ESC  BACK",g->fSmall.Get(),D2D1::RectF(panel.left,panel.bottom-H*0.06f,panel.right,panel.bottom-H*0.015f),rgba(0.7f,0.75f,0.9f),1.5f);
}
// ---------- updates (GitHub releases) ----------
void updatePanelPad(Pad p){
 using S=mcl::Updater::State;
 if(g->updPanel==1){
  if(p==Pad::Up||p==Pad::Down||p==Pad::Left||p==Pad::Right){g->updRow^=1;return;}
  if(p==Pad::Cross||p==Pad::Start){
   if(g->updRow==0&&g->updater&&g->updater->state()==S::Available){g->updater->startDownload();g->updPanel=2;}
   else g->updPanel=0;
  }else if(p==Pad::Circle)g->updPanel=0;
 }else if(g->updPanel==2){
  if(p==Pad::Circle&&g->updater&&g->updater->state()==S::Downloading)g->updater->cancel();
 }
}
// Called every frame while no game runs.
// No connection: one clear message (the updater tags these errors "NETWORK ERROR: ...").
std::wstring updateFailure(const std::wstring& what,bool changedNothing){
 const std::wstring e=g->updater->error();
 if(e.rfind(L"NETWORK ERROR",0)==0)
  return L"NETWORK ERROR\n\n"+what+L": NO INTERNET CONNECTION, OR THE UPDATE SERVER COULD NOT BE REACHED.\n\n"
         L"CHECK THAT THIS PC IS ONLINE AND TRY AGAIN (OPTIONS > CHECK FOR UPDATES)."+(changedNothing?L"\n\nYOUR GAME WAS NOT CHANGED.":L"");
 return what+L"\n\n"+upper(e)+L"."+(changedNothing?L"\n\nYOUR GAME WAS NOT CHANGED.":L"");
}
std::wstring utf8Wide(const std::string& s){
 if(s.empty())return {};
 const int n=MultiByteToWideChar(CP_UTF8,0,s.data(),static_cast<int>(s.size()),nullptr,0);
 std::wstring w(static_cast<size_t>(n),L'\0');MultiByteToWideChar(CP_UTF8,0,s.data(),static_cast<int>(s.size()),w.data(),n);return w;
}
void updateTick(){
 if(!g->updater)return;
 using S=mcl::Updater::State;
 const S st=g->updater->state();
 const bool calm=g->screen==Screen::Title||g->screen==Screen::Menu||g->screen==Screen::Options;
 if(g->updManual&&(st==S::UpToDate||st==S::Failed||st==S::Available)){
  g->updManual=false;
  if(st==S::UpToDate)showBanner(L"UP TO DATE");
  else if(st==S::Failed)g->overlay=updateFailure(L"COULD NOT CHECK FOR UPDATES",false);
  else{g->updOffered=true;g->updPanel=1;g->updRow=0;}
 }
 if(st==S::Available&&!g->updOffered&&calm&&g->overlay.empty()&&!g->updPanel){g->updOffered=true;g->updPanel=1;g->updRow=0;}
 if(g->updPanel==2){
  if(st==S::Available){g->updPanel=0;showBanner(L"UPDATE CANCELLED");}
  else if(st==S::Failed){
   g->updPanel=0;g->overlay=updateFailure(L"UPDATE UNSUCCESSFUL",true);
   applyVideoSetting(); // menus come back to life (music restarts on its own)
  }
  else if(st==S::Ready){
   // Install inside the game: let go of the files the menus hold open first.
   g->music.stop();g->track=nullptr;g->fadeStart=-1;
   g->video.stop();g->videoBmp.Reset();g->videoSerial=0;
   g->preview.stop();g->previewId.clear();g->previewReady.clear();
   mciSendStringW(L"close mcl_voice",nullptr,0,nullptr);
   saveSettings();
   g->updater->startInstall();
  }
  else if(st==S::Installed){
   // Restart into the new version; it says "UPDATE SUCCESSFUL" when it opens.
   if(g->updater->restartGame()){g->updPanel=0;DestroyWindow(g->hwnd);}
   else{g->updPanel=0;g->overlay=L"UPDATE INSTALLED\n\nPLEASE CLOSE AND REOPEN MARVEL: CROSSOVER LEGACY TO FINISH.";}
  }
 }
}
void drawUpdatePanel(double now){
 if(!g->updPanel||!g->updater)return;
 const float W=g->W(),H=g->H();
 using S=mcl::Updater::State;
 g->brush->SetColor(rgba(0,0,0));g->brush->SetOpacity(0.7f);g->rt->FillRectangle(D2D1::RectF(0,0,W,H),g->brush.Get());
 // The offer is taller: it shows the release notes from GitHub above the choices.
 const D2D1_RECT_F panel=g->updPanel==1?D2D1::RectF(W*0.16f,H*0.08f,W*0.84f,H*0.94f):D2D1::RectF(W*0.2f,H*0.27f,W*0.8f,H*0.78f);
 g->brush->SetColor(rgba(0.03f,0.05f,0.13f));g->brush->SetOpacity(0.97f);g->rt->FillRectangle(panel,g->brush.Get());g->brush->SetOpacity(1);
 g->brush->SetColor(rgba(1.f,0.45f,0.25f));g->rt->DrawRectangle(panel,g->brush.Get(),3.f);
 const mcl::ReleaseInfo r=g->updater->release();
 const std::wstring name=upper(wide(r.name.empty()?r.tag:r.name));
 wchar_t size[64];swprintf(size,64,L"DOWNLOAD: %.1f GB",static_cast<double>(r.assetSize)/(1024.0*1024.0*1024.0));
 text(g->updPanel==1?L"UPDATE AVAILABLE!":L"UPDATING...",g->fHead.Get(),D2D1::RectF(panel.left,panel.top+H*0.03f,panel.right,panel.top+H*0.11f),rgba(1.f,0.84f,0.25f),3);
 text(name,g->fBody.Get(),D2D1::RectF(panel.left,panel.top+H*0.115f,panel.right,panel.top+H*0.16f),rgba(0.93f,0.95f,1.f),2);
 if(g->updPanel==1){
  // What's new (the GitHub release notes)
  const D2D1_RECT_F notes=D2D1::RectF(panel.left+W*0.03f,panel.top+H*0.205f,panel.right-W*0.03f,panel.top+H*0.52f);
  g->brush->SetColor(rgba(0.06f,0.09f,0.2f));g->rt->FillRectangle(notes,g->brush.Get());
  g->brush->SetColor(rgba(0.3f,0.4f,0.7f));g->rt->DrawRectangle(notes,g->brush.Get(),1.5f);
  text(L"WHAT'S NEW",g->fSmall.Get(),D2D1::RectF(notes.left,notes.top-H*0.032f,notes.right,notes.top-H*0.002f),rgba(1.f,0.84f,0.25f),1.5f);
  std::wstring body=utf8Wide(mcl::releaseNotesText(r.body,16));
  if(body.empty())body=L"(No release notes were published with this update.)";
  ComPtr<IDWriteTextLayout> l;
  const float pad=W*0.012f;
  g->dw->CreateTextLayout(body.c_str(),static_cast<UINT32>(body.size()),g->fSmall.Get(),notes.right-notes.left-2*pad,notes.bottom-notes.top-2*pad,&l);
  if(l){
   l->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);l->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);l->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
   g->rt->PushAxisAlignedClip(notes,D2D1_ANTIALIAS_MODE_ALIASED);
   g->brush->SetColor(rgba(0.88f,0.91f,1.f));g->rt->DrawTextLayout(D2D1::Point2F(notes.left+pad,notes.top+pad),l.Get(),g->brush.Get());
   g->rt->PopAxisAlignedClip();
  }
  text(size,g->fSmall.Get(),D2D1::RectF(panel.left,panel.top+H*0.535f,panel.right,panel.top+H*0.565f),rgba(0.7f,0.78f,0.95f),1.5f);
  text(L"YOUR PROGRESS, SETTINGS, CONTROLS AND HIGH SCORES ARE KEPT.",g->fSmall.Get(),D2D1::RectF(panel.left,panel.top+H*0.565f,panel.right,panel.top+H*0.595f),rgba(0.7f,0.78f,0.95f),1.5f);
  const wchar_t* items[2]={L"UPDATE NOW",L"LATER"};
  for(int i=0;i<2;++i){
   const float top=panel.top+H*(0.61f+0.075f*i);const bool on=i==g->updRow;
   text(items[i],g->fMenu.Get(),D2D1::RectF(panel.left,top,panel.right,top+H*0.075f),on?rgba(1.f,0.84f,0.25f):rgba(0.8f,0.84f,0.95f),on?4.f:2.f,on?1.f:0.7f);
  }
  text(L"A / ENTER  SELECT     B / ESC  LATER     (ALSO IN OPTIONS > CHECK FOR UPDATES)",g->fSmall.Get(),D2D1::RectF(panel.left,panel.bottom-H*0.06f,panel.right,panel.bottom-H*0.015f),rgba(0.7f,0.75f,0.9f),1.5f);
  return;
 }
 const S st=g->updater->state();
 const float p=static_cast<float>(g->updater->progress());
 const D2D1_RECT_F bar=D2D1::RectF(panel.left+W*0.05f,panel.top+H*0.25f,panel.right-W*0.05f,panel.top+H*0.29f);
 g->brush->SetColor(rgba(0.1f,0.14f,0.3f));g->rt->FillRectangle(bar,g->brush.Get());
 const float fill=st==S::Downloading||st==S::Installing?p:1.f;
 g->brush->SetColor(rgba(1.f,0.7f,0.2f));g->rt->FillRectangle(D2D1::RectF(bar.left,bar.top,bar.left+(bar.right-bar.left)*fill,bar.bottom),g->brush.Get());
 g->brush->SetColor(rgba(1.f,0.84f,0.25f));g->rt->DrawRectangle(bar,g->brush.Get(),2.f);
 std::wstring status;
 if(st==S::Downloading){wchar_t b[96];swprintf(b,96,L"DOWNLOADING  %d%%  (%.1f / %.1f GB)",static_cast<int>(p*100),p*static_cast<double>(r.assetSize)/(1<<30),static_cast<double>(r.assetSize)/(1<<30));status=b;}
 else if(st==S::Extracting)status=std::fmod(now,900.0)<600.0?L"UNPACKING THE UPDATE...":L"";
 else if(st==S::Installing){wchar_t b[64];swprintf(b,64,L"INSTALLING THE UPDATE  %d%%",static_cast<int>(p*100));status=b;}
 else status=L"RESTARTING THE GAME...";
 text(status,g->fBody.Get(),D2D1::RectF(panel.left,panel.top+H*0.31f,panel.right,panel.top+H*0.36f),rgba(0.93f,0.95f,1.f),2);
 text(st==S::Downloading?L"B / ESC  CANCEL":L"THE GAME WILL RESTART BY ITSELF",g->fSmall.Get(),D2D1::RectF(panel.left,panel.bottom-H*0.06f,panel.right,panel.bottom-H*0.015f),rgba(0.7f,0.75f,0.9f),1.5f);
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
   if(g&&g->flyPause){ // MvC2 pause screen
    if(repeat)return 0;
    switch(w){
     case VK_UP: onPad(Pad::Up);break;
     case VK_DOWN: onPad(Pad::Down);break;
     case VK_LEFT: onPad(Pad::Left);break;
     case VK_RIGHT: onPad(Pad::Right);break;
     case VK_RETURN: case 'X': case VK_SPACE: onPad(Pad::Cross);break;
     case VK_ESCAPE: case 'C': case VK_TAB: case VK_BACK: onPad(Pad::Circle);break;
    }
    return 0;
   }
   if(g&&g->screen==Screen::ControlSetup&&g->csCapture==1){ // CONTROLS SETUP: waiting for a key
    if(w==VK_ESCAPE){g->csCapture=0;return 0;}
    UINT vk=static_cast<UINT>(w);
    if(vk==VK_SHIFT||vk==VK_CONTROL||vk==VK_MENU){const UINT sc=(l>>16)&0xFF;const bool ext=(l&(1<<24))!=0;
     vk=vk==VK_SHIFT?MapVirtualKeyW(sc,MAPVK_VSC_TO_VK_EX):vk==VK_CONTROL?(ext?VK_RCONTROL:VK_LCONTROL):(ext?VK_RMENU:VK_LMENU);}
    const std::string name=mcl::mameKeyName(static_cast<int>(vk));
    if(!name.empty())csAssign(name);
    return 0;
   }
   const bool lists=g&&(g->screen==Screen::Cheats||g->screen==Screen::Options||g->screen==Screen::Achievements||g->screen==Screen::Gallery||g->screen==Screen::Jukebox||g->screen==Screen::ControlSetup); // auto-repeat only in lists
   switch(w){
    case VK_UP: if(!repeat||lists)onPad(Pad::Up);return 0;
    case VK_DOWN: if(!repeat||lists)onPad(Pad::Down);return 0;
    case VK_LEFT: if(!repeat)onPad(Pad::Left);return 0;
    case VK_RIGHT: if(!repeat)onPad(Pad::Right);return 0;
    case 'X': if(!repeat&&!(IsWindowEnabled(g->hwnd)&&!g->gameRunning&&g->overlay.empty()&&!g->updPanel&&cheatShortcut(false)))onCodeKey(Pad::Cross);return 0;
    case 'C': if(!repeat&&!(IsWindowEnabled(g->hwnd)&&!g->gameRunning&&g->overlay.empty()&&!g->updPanel&&cheatShortcut(true)))onCodeKey(Pad::Circle);return 0;
    case VK_BACK: if(!repeat)onCodeKey(Pad::Select);return 0;
    case VK_SPACE: if(!repeat)onCodeKey(Pad::Start);return 0;
    case VK_RETURN: if(!repeat)onConfirm();return 0;
    case VK_ESCAPE: if(!repeat)onBack();return 0;
    case VK_TAB: if(!repeat&&g&&g->screen==Screen::Select)onPad(Pad::Select);return 0;
   }
   break;}
  case WM_LBUTTONDOWN:
   if(g&&g->screen==Screen::Intro){endIntro();return 0;}
   if(g&&g->screen==Screen::Title)onPad(Pad::Start);
   return 0;
  case WM_ERASEBKGND: return 1;
  case WM_DEVICECHANGE: if(g)g->pads.rescan();return TRUE; // controller plugged in or out
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
 app.controls.load(appDir/"MCL_UserData"/"controls.txt");
 app.mvc2Controls.load(appDir/"MCL_UserData"/"controls_mvc2.txt");
 app.updater=std::make_unique<mcl::Updater>(appDir);app.updater->startCheck(); // GitHub releases, in the background
 // Just restarted by an update: say how it went, and clear the update's leftovers.
 {mcl::UpdateResult res;
  if(mcl::takeUpdateResult(appDir,res)){
   app.overlay=res.ok?L"UPDATE SUCCESSFUL!\n\nMARVEL: CROSSOVER LEGACY HAS BEEN UPDATED TO v"+std::wstring(mcl::kAppVersion,mcl::kAppVersion+std::strlen(mcl::kAppVersion))+
                      L".\n\nYOUR PROGRESS, SETTINGS, CONTROLS AND HIGH SCORES WERE KEPT."
                     :L"UPDATE UNSUCCESSFUL\n\nTHE UPDATE COULD NOT BE FINISHED. YOUR GAME WAS NOT CHANGED.";
  }
  mcl::cleanupUpdateFolder(appDir);}
 app.pads.init(app.hwnd);
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
   if(app.flyPause){pollPad();render();Sleep(10);continue;} // MvC2 pause screen (Flycast frozen)
   std::wstring failure;
   if(app.cb.launcher&&app.cb.launcher->pollEnded(failure)){
    app.cb.launcher->stopCoverGuard();
    SetWindowPos(app.hwnd,HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
    const bool flyExit=app.flyExiting;app.flyExiting=false;app.flyPause=false;
    if(app.gameLoading){app.gameLoading=false;SetWindowPos(app.hwnd,HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);}
    app.gameRunning=false;app.video.setPaused(false);
    // Results were just saved by the launcher; keep a dated backup and say so.
    mcl::backupUserData(app.dir/"MCL_UserData",mcl::backupStamp());app.savedAt=app.now();
    ShowWindow(app.hwnd,SW_SHOW);SetForegroundWindow(app.hwnd);
    app.selSince=app.now();app.padPrev=0xFFFF; // ignore buttons still held from the game
    if(app.cb.launcher->takeExitSignal()||flyExit){
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
     app.cb.launcher->stopCoverGuard();
     app.gameLoading=false;app.video.setPaused(true);
     SetWindowPos(app.hwnd,HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
     app.cb.launcher->bringToFront();
    }else{
     // The emulator window is held out of "always on top" by the cover guard (1 ms checks);
     // the cover re-asserts its own top position every frame from this thread, so nothing
     // is left queued that could re-raise it once loading is over.
     SetWindowPos(app.hwnd,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_SHOWWINDOW);
     render();continue;
    }
   }else if(app.flyExiting){render();Sleep(15);continue;}
   else if(app.cb.launcher&&app.cb.launcher->flycastGame()){
    if(flyPauseTriggered())openFlyPause();
    else{
     if(app.mvc2One.takeover())mvc2OneButtonTick();
     app.cb.launcher->applyFlycastCheats(); // MvC2 cheats (written into NAOMI RAM)
    }
    Sleep(8);continue;
   }else{Sleep(100);continue;}
  }
  if(IsIconic(app.hwnd)||!IsWindowEnabled(app.hwnd)){app.video.setPaused(true);WaitMessage();continue;}
  app.video.setPaused(false);
  pollPad();
  updateTick();
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
