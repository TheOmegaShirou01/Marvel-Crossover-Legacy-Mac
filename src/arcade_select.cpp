#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <dshow.h>
#include <algorithm>
#include <array>
#include <filesystem>
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <iterator>
#include "cheat_ex_settings_win.hpp"
#include "external_roms.hpp"
#include "emulator_bridge.hpp"
#include "mame_backend.hpp"
#include "mame_sets.hpp"
#include "cheats.hpp"
#include "cheat_ex_settings.hpp"
#include "rom_audit.hpp"
#include "pause_menu_win.hpp"
#pragma comment(lib,"strmiids.lib")
#pragma comment(lib,"ole32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"gdi32.lib")

namespace mcl_select {
namespace fs=std::filesystem;
constexpr UINT_PTR TIMER_ID=904;
constexpr int CARD_BASE=8100;
constexpr int BACK_ID=8290;
constexpr int SWITCH_ID=8291;
constexpr int LAUNCH_ID=8292;
constexpr int STATUS_ID=8293;
constexpr int EX_CHEATS_ID=8294;
constexpr int EX_ROM_CHECK_ID=8295;
constexpr int ROM_AUDIT_ID=8296;
constexpr int PAUSE_PREVIEW_ID=8297;
constexpr int MAME_PAUSE_KEY_ID=8298;
struct Entry {const wchar_t* id;const wchar_t* label;};
constexpr Entry marvel[]={{L"xmcota",L"X-MEN: CHILDREN OF THE ATOM"},{L"msh",L"MARVEL SUPER HEROES"},{L"xmvsf",L"X-MEN VS. STREET FIGHTER"},{L"mshvsf",L"MARVEL SUPER HEROES VS. STREET FIGHTER"},{L"mvsc",L"MARVEL VS. CAPCOM"},{L"mvsc2",L"MARVEL VS. CAPCOM 2"}};
constexpr Entry ex[]={{L"sfexp",L"STREET FIGHTER EX PLUS"},{L"sfex2p",L"STREET FIGHTER EX2 PLUS"}};

struct Video {
 IGraphBuilder* graph=nullptr;
 IMediaControl* ctl=nullptr;
 IVideoWindow* video=nullptr;
 IMediaSeeking* seek=nullptr;
 HWND host=nullptr;
 bool running=false;
 void close(){
  if(ctl)ctl->Stop();
  if(video){video->put_Visible(OAFALSE);video->put_Owner((OAHWND)nullptr);video->Release();video=nullptr;}
  if(seek){seek->Release();seek=nullptr;}
  if(ctl){ctl->Release();ctl=nullptr;}
  if(graph){graph->Release();graph=nullptr;}
  if(host){DestroyWindow(host);host=nullptr;}
  running=false;
 }
 bool open(HWND parent,const fs::path& path,const RECT& r,bool behind){
  close();std::error_code ec;if(!fs::is_regular_file(path,ec))return false;
  host=CreateWindowExW(0,L"STATIC",L"",WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS,r.left,r.top,r.right-r.left,r.bottom-r.top,parent,nullptr,GetModuleHandleW(nullptr),nullptr);
  if(!host)return false;
  HRESULT hr=CoCreateInstance(CLSID_FilterGraph,nullptr,CLSCTX_INPROC_SERVER,IID_IGraphBuilder,(void**)&graph);
  if(FAILED(hr)){close();return false;}
  hr=graph->RenderFile(path.c_str(),nullptr);
  if(FAILED(hr)){close();return false;}
  hr=graph->QueryInterface(IID_IMediaControl,(void**)&ctl);
  if(FAILED(hr)){close();return false;}
  if(FAILED(graph->QueryInterface(IID_IVideoWindow,(void**)&video))){close();return false;}
  graph->QueryInterface(IID_IMediaSeeking,(void**)&seek);
  video->put_Owner((OAHWND)host);
  video->put_WindowStyle(WS_CHILD|WS_CLIPSIBLINGS|WS_CLIPCHILDREN);
  video->SetWindowPosition(0,0,r.right-r.left,r.bottom-r.top);
  video->put_Visible(OATRUE);
  if(behind)SetWindowPos(host,HWND_BOTTOM,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
  if(FAILED(ctl->Run())){close();return false;}
  running=true;return true;
 }
 void loop(){if(!running || !seek || !ctl)return;
  LONGLONG pos=0,duration=0;
  if(SUCCEEDED(seek->GetCurrentPosition(&pos)) && SUCCEEDED(seek->GetDuration(&duration)) && duration>0 && pos>=duration-2500000){
    ctl->Stop();LONGLONG start=0;seek->SetPositions(&start,AM_SEEKING_AbsolutePositioning,nullptr,AM_SEEKING_NoPositioning);ctl->Run();
  }
 }
};

class Selection {
 HWND wnd_=nullptr, info_=nullptr, switch_=nullptr, launch_=nullptr, exCheats_=nullptr, romCheck_=nullptr, audit_=nullptr, mamePauseKey_=nullptr;
 std::array<HWND,6> cards_{};
 std::array<HBITMAP,8> cardImages_{};
 bool exMode_=false;
 bool exUnlocked_=false; // unlocked by frontend code
 bool cheatsUnlocked_=false; // master cheat code entered on the title screen this session
 int selected_=-1;
 int flip_=0;
 Video globe_,preview_;
 fs::path assets_;
 mcl::Collection& model_;
 std::unique_ptr<mcl::MameBackend> emulator_;
 std::unique_ptr<mcl::EmulatorBridge> session_;
 void updateLaunch(){
  if(!launch_)return;
  if(mamePauseKey_)EnableWindow(mamePauseKey_,session_&&session_->state()==mcl::RuntimeState::Running&&emulator_&&emulator_->running());
  bool supported=selected_>=0&&selected_<size()&&(!exMode_||exUnlocked_);
  if(session_&&session_->state()==mcl::RuntimeState::Running){
   SetWindowTextW(launch_,L"EXIT RUNNING GAME");EnableWindow(launch_,TRUE);return;
  }
  SetWindowTextW(launch_,supported?(exMode_?L"LAUNCH EX ARCADE GAME":(selected_==5?L"LAUNCH NAOMI GAME (FLYCAST)":L"LAUNCH CPS-2 GAME")):L"EMULATION NOT CONNECTED");
  bool canLaunch=false;
  if(supported&&emulator_){const std::wstring id=items()[selected_].id;canLaunch=emulator_->available(std::string(id.begin(),id.end()));}
  EnableWindow(launch_,canLaunch);
 }
 void launchGame(){
  if(!emulator_||!session_||selected_<0)return;
  if(session_->state()==mcl::RuntimeState::Running){
   auto r=session_->stop();
   if(!r.ok){MessageBoxW(wnd_,L"Could not close the emulator safely. Exit it using its normal menu/Escape key.",L"Emulator Still Running",MB_ICONWARNING);return;}
   updateLaunch();return;
  }
  if(selected_>=size()||(exMode_&&!exUnlocked_))return; // Six Marvel or two unlocked EX titles.
  const std::wstring wId=items()[selected_].id;
  const std::string id(wId.begin(),wId.end());
  const auto pokes=cheatPokesFor(id);
  const fs::path script=assets_/"MCL_UserData"/"cheats_active.lua";
  if(!pokes.empty()){
   std::error_code ec;fs::create_directories(script.parent_path(),ec);
   std::ofstream out(script,std::ios::trunc);out<<mcl::buildCheatLua(id,pokes);
   emulator_->setCheatScript(out?script:fs::path{});
  }else emulator_->setCheatScript({});
  auto prepared=session_->prepare(id);
  if(!prepared.ok){
   auto text=std::wstring(prepared.message.begin(),prepared.message.end());
   MessageBoxW(wnd_,text.c_str(),L"Cannot prepare arcade game",MB_ICONWARNING);return;
  }
  auto started=session_->start();
  if(!started.ok){
   session_->stop();
   const std::string why=emulator_->lastStartError();
   std::wstring text=L"The game did not launch.\n\n";
   text+=why.empty()?L"Check the emulator path, game ROM and hardware BIOS.":std::wstring(why.begin(),why.end());
   MessageBoxW(wnd_,text.c_str(),L"Launch failed",MB_ICONWARNING);return;
  }
  if(!pokes.empty()&&!mcl::usesFlycast(id)){
   session_->onCheatApplicationStatus(true); // cheats on: mastery achievements stay locked for this run
   SetWindowTextW(info_,L"Game started WITH CHEATS ON. Achievements still unlock.");
  }else SetWindowTextW(info_,L"Game started. Press Esc in MAME to quit back to this screen.");
  updateLaunch();
 }

 // Cheats switched on for this game: Marvel cheats need the master code this session,
 // EX cheats need CHEAT EX (always true when the EX cards are reachable).
 std::vector<mcl::Poke> cheatPokesFor(const std::string& id) const{
  if(id=="sfexp"||id=="sfex2p"){
   if(!exUnlocked_)return {};
   mcl::ExCheatSettings ex(assets_/"MCL_UserData"/"cheat_ex_settings.txt");ex.load();
   const auto& o=ex.options(id);
   return mcl::exCheatPokes(id,o.get(mcl::ExCheat::Health),o.get(mcl::ExCheat::Power),o.get(mcl::ExCheat::Time));
  }
  if(!cheatsUnlocked_)return {};
  mcl::CheatSettings s(assets_/"MCL_UserData"/"marvel_cheats.txt");s.load();
  return s.activePokes(id);
 }
 HBRUSH dark_=nullptr;
 HFONT heading_=nullptr,font_=nullptr;
 static LRESULT CALLBACK proc(HWND h,UINT m,WPARAM w,LPARAM l){
  auto self=(Selection*)GetWindowLongPtrW(h,GWLP_USERDATA);
  if(m==WM_NCCREATE){self=(Selection*)((CREATESTRUCTW*)l)->lpCreateParams;SetWindowLongPtrW(h,GWLP_USERDATA,(LONG_PTR)self);self->wnd_=h;}
  return self?self->handle(m,w,l):DefWindowProcW(h,m,w,l);
 }
 const Entry* items()const{return exMode_?ex:marvel;}
 int size()const{return exMode_?2:6;}
 RECT cardRect(int index)const{RECT r{};GetClientRect(wnd_,&r);int avail=r.right;
   int width=std::max(200,(avail-84)/3);int x=24+(index%3)*(width+18), y=110+(index/3)*235;
   return {x,y,x+width,y+195};}
 void loadCardImages(){
  constexpr const wchar_t* ids[]={L"xmcota",L"msh",L"xmvsf",L"mshvsf",L"mvsc",L"mvsc2",L"sfexp",L"sfex2p"};
  for(int i=0;i<8;++i){
   auto path=assets_/L"Logos"/L"Cards"/(std::wstring(ids[i])+L".bmp");
   cardImages_[(size_t)i]=(HBITMAP)LoadImageW(nullptr,path.c_str(),IMAGE_BITMAP,0,0,LR_LOADFROMFILE|LR_CREATEDIBSECTION);
  }
 }
 void setCardArtwork(int index){
  int asset=exMode_?6+index:index;
  HBITMAP bitmap=cardImages_[(size_t)asset];
  SendMessageW(cards_[(size_t)index],BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)bitmap);
  SetWindowTextW(cards_[(size_t)index],bitmap?L"":items()[index].label);
 }
 void showSelected(){
  preview_.close();for(int i=0;i<size();++i)if(cards_[i]){
   setCardArtwork(i);
   ShowWindow(cards_[i],SW_SHOW);
  }
  if(selected_<0||selected_>=size())return;
  SetWindowTextW(info_,(std::wstring(L"SELECTED: ")+items()[selected_].label+L"  |  FIGHTING FOOTAGE ONLY").c_str());
  flip_=1;SetTimer(wnd_,TIMER_ID,65,nullptr);
 }
 void flipTick(){if(selected_<0)return;
  RECT r=cardRect(selected_);int width=r.right-r.left;int center=(r.left+r.right)/2;
  if(flip_<=4){int w=std::max(2,width*(4-flip_)/4);MoveWindow(cards_[selected_],center-w/2,r.top,w,r.bottom-r.top,TRUE);flip_++;return;}
  if(flip_<=8){int w=std::max(2,width*(flip_-4)/4);MoveWindow(cards_[selected_],center-w/2,r.top,w,r.bottom-r.top,TRUE);flip_++;return;}
  KillTimer(wnd_,TIMER_ID);flip_=0;
  auto path=assets_/"Previews"/(std::wstring(items()[selected_].id)+L".mp4");
  if(!preview_.open(wnd_,path,r,false)){
    SetWindowTextW(info_,L"Preview codec unavailable or media missing; original game label remains usable.");return;
  }
  ShowWindow(cards_[selected_],SW_HIDE); // reveal real gameplay video in the card's original location
  SetWindowPos(preview_.host,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
 }
 void select(int i){if(i<0||i>=size())return;
  if(flip_){KillTimer(wnd_,TIMER_ID);flip_=0;}
  if(selected_>=0){RECT r=cardRect(selected_);MoveWindow(cards_[selected_],r.left,r.top,r.right-r.left,r.bottom-r.top,TRUE);}
  selected_=i;showSelected();updateLaunch();SetFocus(wnd_);
 }
 void layout(){RECT r{};GetClientRect(wnd_,&r);
  for(int i=0;i<6;++i){RECT cr=cardRect(i);if(cards_[i])MoveWindow(cards_[i],cr.left,cr.top,cr.right-cr.left,cr.bottom-cr.top,TRUE);}
  if(globe_.video&&globe_.host){MoveWindow(globe_.host,0,0,r.right,r.bottom,TRUE);globe_.video->SetWindowPosition(0,0,r.right,r.bottom);SetWindowPos(globe_.host,HWND_BOTTOM,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);}
  if(preview_.host&&selected_>=0){RECT cr=cardRect(selected_);MoveWindow(preview_.host,cr.left,cr.top,cr.right-cr.left,cr.bottom-cr.top,TRUE);if(preview_.video)preview_.video->SetWindowPosition(0,0,cr.right-cr.left,cr.bottom-cr.top);}
 }
 void rebuild(){preview_.close();selected_=-1;flip_=0;for(int i=0;i<6;++i){ShowWindow(cards_[i],i<size()?SW_SHOW:SW_HIDE);if(i<size())setCardArtwork(i);}
 SetWindowTextW(switch_,exMode_?L"BACK TO MARVEL SIX":(exUnlocked_?L"CHEAT EX":L"CHEAT EX (LOCKED)"));
 if(exCheats_)ShowWindow(exCheats_,exMode_?SW_SHOW:SW_HIDE);
 if(romCheck_)ShowWindow(romCheck_,exMode_?SW_SHOW:SW_HIDE);SetWindowTextW(info_,exMode_?L"SECRET EX ARCADE FIGHTERS":L"MARVEL ARCADE GAME SELECT — USE ARROWS, ENTER TO PREVIEW");
 select(0);
 }
 LRESULT handle(UINT msg,WPARAM w,LPARAM l){switch(msg){
  case WM_CREATE:{dark_=CreateSolidBrush(RGB(10,14,28));heading_=CreateFontW(-27,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Arial Black");font_=CreateFontW(-17,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Arial");
   HWND title=CreateWindowW(L"STATIC",L"MARVEL: CROSSOVER LEGACY  |  ARCADE SELECT",WS_CHILD|WS_VISIBLE,24,16,1050,50,wnd_,nullptr,nullptr,nullptr);SendMessageW(title,WM_SETFONT,(WPARAM)heading_,TRUE);
   info_=CreateWindowW(L"STATIC",L"",WS_CHILD|WS_VISIBLE,24,73,1030,25,wnd_,(HMENU)STATUS_ID,nullptr,nullptr);SendMessageW(info_,WM_SETFONT,(WPARAM)font_,TRUE);
   loadCardImages();
   for(int i=0;i<6;++i){cards_[i]=CreateWindowW(L"BUTTON",L"",WS_CHILD|WS_VISIBLE|BS_BITMAP,0,0,140,140,wnd_,(HMENU)(CARD_BASE+i),nullptr,nullptr);SendMessageW(cards_[i],WM_SETFONT,(WPARAM)font_,TRUE);}
   switch_=CreateWindowW(L"BUTTON",L"CHEAT EX (LOCKED)",WS_CHILD|WS_VISIBLE,24,575,220,36,wnd_,(HMENU)SWITCH_ID,nullptr,nullptr);
   launch_=CreateWindowW(L"BUTTON",L"LAUNCH CPS-2 GAME",WS_CHILD|WS_VISIBLE,255,575,245,36,wnd_,(HMENU)LAUNCH_ID,nullptr,nullptr);
   HWND back=CreateWindowW(L"BUTTON",L"RETURN TO FEATURES",WS_CHILD|WS_VISIBLE,510,575,210,36,wnd_,(HMENU)BACK_ID,nullptr,nullptr);
   exCheats_=CreateWindowW(L"BUTTON",L"EX CHEATS",WS_CHILD|WS_VISIBLE,735,575,155,36,wnd_,(HMENU)EX_CHEATS_ID,nullptr,nullptr);
   romCheck_=CreateWindowW(L"BUTTON",L"CHECK EX ROMS",WS_CHILD|WS_VISIBLE,895,575,170,36,wnd_,(HMENU)EX_ROM_CHECK_ID,nullptr,nullptr);
   audit_=CreateWindowW(L"BUTTON",L"VERIFY SELECTED ROM (MAME)",WS_CHILD|WS_VISIBLE,24,615,330,32,wnd_,(HMENU)ROM_AUDIT_ID,nullptr,nullptr);
   HWND pausePreview=CreateWindowW(L"BUTTON",L"PAUSE MENU PREVIEW",WS_CHILD|WS_VISIBLE,370,615,235,32,wnd_,(HMENU)PAUSE_PREVIEW_ID,nullptr,nullptr);
   SendMessageW(pausePreview,WM_SETFONT,(WPARAM)font_,TRUE);
   mamePauseKey_=CreateWindowW(L"BUTTON",L"SEND MAME PAUSE KEY (F5)",WS_CHILD|WS_VISIBLE,620,615,288,32,wnd_,(HMENU)MAME_PAUSE_KEY_ID,nullptr,nullptr);
   SendMessageW(mamePauseKey_,WM_SETFONT,(WPARAM)font_,TRUE);
   SendMessageW(audit_,WM_SETFONT,(WPARAM)font_,TRUE);
   SendMessageW(romCheck_,WM_SETFONT,(WPARAM)font_,TRUE);SendMessageW(exCheats_,WM_SETFONT,(WPARAM)font_,TRUE);SendMessageW(switch_,WM_SETFONT,(WPARAM)font_,TRUE);SendMessageW(launch_,WM_SETFONT,(WPARAM)font_,TRUE);SendMessageW(back,WM_SETFONT,(WPARAM)font_,TRUE);
   {
    auto emulatorFile=mcl::readMameExecutable(assets_/"mame_executable.txt");
    emulator_=std::make_unique<mcl::MameBackend>(emulatorFile);
    emulator_->setLogFile(assets_/"Logs"/"mame_last_launch.log");
    emulator_->setFlycastExecutable(mcl::readMameExecutable(assets_/"flycast_executable.txt"));
    session_=std::make_unique<mcl::EmulatorBridge>(model_,*emulator_,mcl::resolveRuntimeRomSearchPath(assets_));
   }
   rebuild();RECT r{};GetClientRect(wnd_,&r);globe_.open(wnd_,assets_/"earth_background.mp4",r,true);SetTimer(wnd_,TIMER_ID+1,150,nullptr);return 0;}
  case WM_CTLCOLORSTATIC:{SetTextColor((HDC)w,RGB(250,217,89));SetBkColor((HDC)w,RGB(10,14,28));return (INT_PTR)dark_;}
  case WM_COMMAND:{int id=LOWORD(w);
   if(id>=CARD_BASE&&id<CARD_BASE+size()){select(id-CARD_BASE);return 0;}
   if(id==BACK_ID){
    if(session_&&session_->state()==mcl::RuntimeState::Running){
     MessageBoxW(wnd_,L"Exit the running game before closing this selection screen.",L"Game running",MB_ICONINFORMATION);return 0;
    }
    DestroyWindow(wnd_);return 0;
   }
   if(id==LAUNCH_ID){launchGame();return 0;}
   if(id==MAME_PAUSE_KEY_ID){
    if(!session_||session_->state()!=mcl::RuntimeState::Running||!emulator_||!emulator_->running())return 0;
    if(emulator_->requestPauseHotkey()){
     SetWindowTextW(info_,L"MAME F5 pause/resume key SENT. Emulator pause state NOT verified; check the MAME window.");
    }else{
     MessageBoxW(wnd_,L"Could not send pause key to foreground MAME window. Check focus, permissions, and MAME key bindings.",L"Pause key request failed",MB_ICONWARNING);
    }
    return 0;
   }
   if(id==PAUSE_PREVIEW_ID){
    if(session_&&session_->state()==mcl::RuntimeState::Running){
     MessageBoxW(wnd_,L"The current MAME backend cannot safely pause gameplay. This preview is unavailable during a running game.",L"Pause unavailable",MB_ICONWARNING);
     return 0;
    }
    if(mcl_pause_ui::showPreview(wnd_))SetWindowTextW(info_,L"Pause menu preview: Exit Game > Yes selected. No emulator was stopped.");
    else SetWindowTextW(info_,L"Pause menu preview closed. No emulator was paused.");
    return 0;
   }
   if(id==ROM_AUDIT_ID){
    if(selected_<0||selected_>=size()||(exMode_&&!exUnlocked_))return 0;
    if(session_&&session_->state()!=mcl::RuntimeState::Idle){
     MessageBoxW(wnd_,L"Finish the running session before auditing.",L"Audit",MB_ICONINFORMATION);return 0;
    }
    std::wstring wid=items()[selected_].id;
    std::string gid(wid.begin(),wid.end());
    if(mcl::usesFlycast(gid)){
     MessageBoxW(wnd_,L"This game runs in Flycast, so MAME's ROM audit does not apply.\n\nIts files are checked when the repaired set is built with tools\\build_flycast_set.py.",L"ROM check",MB_ICONINFORMATION);
     return 0;
    }
    mcl::MameLaunchSpec spec;std::string reason;
    auto executable=mcl::readMameExecutable(assets_/"mame_executable.txt");
    if(!mcl::createMameAuditSpec(executable,mcl::resolveRuntimeRomSearchPath(assets_),gid,spec,reason)){
     std::wstring why(reason.begin(),reason.end());
     MessageBoxW(wnd_,why.c_str(),L"Cannot audit ROM",MB_ICONWARNING);return 0;
    }
    SetWindowTextW(info_,L"Verifying ROM set with MAME (up to 15 seconds)...");
    auto result=mcl::runMameRomAudit(spec);
    std::wstring msg(result.message.begin(),result.message.end());
    MessageBoxW(wnd_,msg.c_str(),L"MAME ROM Audit",MB_OK|(result.status==mcl::AuditStatus::Pass?MB_ICONINFORMATION:MB_ICONWARNING));
    SetWindowTextW(info_,msg.c_str());
    return 0;
   }
   if(id==EX_ROM_CHECK_ID){
    if(!exUnlocked_||!exMode_||selected_<0||selected_>=2)return 0;
    const char* game=selected_==0?"sfexp":"sfex2p";
    auto roms=mcl::resolveRuntimeRomSearchPath(assets_);
    auto result=mcl::checkExternalRomFiles(roms,game);
    std::wstring report=L"External ROM folder(s): ";
    report+=roms.empty()?L"NOT CONFIGURED":roms.mameRompath();
    report+=L"\n\n";
    for(const auto& entry:result.files){
     report+=std::wstring(entry.filename.begin(),entry.filename.end());
     report+=entry.looksLikeZip?L" : found (ZIP signature valid)\n":(entry.present?L" : invalid ZIP header\n":L" : MISSING\n");
    }
    report+=L"\nThis is a file-presence check, NOT a game boot or ROM-content verification.";
    MessageBoxW(wnd_,report.c_str(),L"CHEAT EX - External ROM Readiness",MB_OK|(result.readyForFileLevelCheck()?MB_ICONINFORMATION:MB_ICONWARNING));
    return 0;
   }
   if(id==EX_CHEATS_ID){if(exUnlocked_&&exMode_&&selected_>=0&&selected_<2)mcl_exui::showSettings(wnd_,assets_,selected_==0?"sfexp":"sfex2p");return 0;}
   if(id==SWITCH_ID){if(session_&&session_->state()==mcl::RuntimeState::Running)return 0;if(!exUnlocked_){MessageBoxW(wnd_,L"Enter the approved secret code in the main window to unlock CHEAT EX.",L"CHEAT EX",MB_OK|MB_ICONINFORMATION);return 0;}exMode_=!exMode_;rebuild();return 0;}
   return 0;}
  case WM_KEYDOWN: if(w==VK_ESCAPE){if(session_&&session_->state()==mcl::RuntimeState::Running){MessageBoxW(wnd_,L"Exit the running arcade game first.",L"Game running",MB_ICONINFORMATION);return 0;}DestroyWindow(wnd_);return 0;}if(w==VK_LEFT||w==VK_UP||w==VK_RIGHT||w==VK_DOWN){select((selected_+(w==VK_LEFT||w==VK_UP?size()-1:1))%size());return 0;}if(w==VK_RETURN&&selected_>=0){select(selected_);return 0;}break;
  case WM_SIZE:layout();return 0;
  case WM_TIMER:if(w==TIMER_ID){flipTick();return 0;}if(w==TIMER_ID+1){globe_.loop();preview_.loop();
    if(session_&&session_->state()==mcl::RuntimeState::Running&&emulator_&&!emulator_->running()){
     unsigned long code=0;double secs=0;
     const bool known=emulator_->lastExit(code,secs);
     const std::string game=session_->game();
     session_->stop();updateLaunch();
     std::string log;
     {std::ifstream in(emulator_->logFile(),std::ios::binary);log.assign(std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>());}
     if(known&&(code!=0||mcl::logShowsFailedStart(log)))reportFailedStart(game,code,log);
     else SetWindowTextW(info_,L"Arcade emulator exited. Returned to game selection.");
    }
    return 0;
   }break;
  case WM_ERASEBKGND:{RECT r{};GetClientRect(wnd_,&r);FillRect((HDC)w,&r,dark_);return 1;}
  case WM_CLOSE:
    if(session_&&session_->state()==mcl::RuntimeState::Running){
     MessageBoxW(wnd_,L"Exit the running arcade game first.",L"Game running",MB_ICONINFORMATION);return 0;
    }
    DestroyWindow(wnd_);return 0;
  case WM_DESTROY:KillTimer(wnd_,TIMER_ID);KillTimer(wnd_,TIMER_ID+1);preview_.close();globe_.close();if(heading_)DeleteObject(heading_);if(font_)DeleteObject(font_);if(dark_)DeleteObject(dark_);for(HBITMAP bitmap:cardImages_)if(bitmap)DeleteObject(bitmap);return 0;
  }return DefWindowProcW(wnd_,msg,w,l);}
 // MAME quit with an error (usually missing/mismatched ROM files): explain it instead of failing silently.
 void reportFailedStart(const std::string& game,unsigned long code,const std::string& log){
  const auto lines=mcl::summarizeMameLog(log,12);
  const bool fly=mcl::usesFlycast(game);
  const wchar_t* emu=fly?L"Flycast":L"MAME";
  std::wstring msg=std::wstring(emu)+L" could not run this game";
  if(!fly&&code==2)msg+=L" because ROM files are missing or don't match your MAME version";
  msg+=L".\n\nGame: "+std::wstring(game.begin(),game.end())+L"   ("+emu+L" exit code "+std::to_wstring(code)+L")\n\n";
  if(lines.empty())msg+=std::wstring(emu)+L" did not report details.\n";
  for(const auto& s:lines){msg+=std::wstring(s.begin(),s.end());msg+=L"\n";}
  msg+=std::wstring(L"\nFull ")+emu+L" output: "+emulator_->logFile().wstring();
  SetWindowTextW(info_,L"Game failed to start - see message for missing files.");
  MessageBoxW(wnd_,msg.c_str(),L"Game could not start",MB_ICONWARNING|MB_OK);
 }
public:
 explicit Selection(fs::path root,bool exUnlocked,mcl::Collection& model,bool startInEx,bool cheatsUnlocked):exMode_(startInEx&&exUnlocked),exUnlocked_(exUnlocked),cheatsUnlocked_(cheatsUnlocked),assets_(std::move(root)),model_(model){}
 int run(HWND owner){
  HRESULT hr=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);bool com=SUCCEEDED(hr);
  HINSTANCE inst=GetModuleHandleW(nullptr);WNDCLASSEXW wc{};wc.cbSize=sizeof(wc);wc.hInstance=inst;wc.lpszClassName=L"MCLArcadeSelection_v033";wc.lpfnWndProc=proc;wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);RegisterClassExW(&wc);
  HWND h=CreateWindowExW(0,wc.lpszClassName,L"Marvel: Crossover Legacy - Arcade Select",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,1135,680,owner,nullptr,inst,this);
  if(!h){if(com)CoUninitialize();return 1;}
  if(owner)EnableWindow(owner,FALSE);ShowWindow(h,SW_SHOW);UpdateWindow(h);
  MSG m{};while(IsWindow(h)&&GetMessageW(&m,nullptr,0,0)>0){if(m.message==WM_QUIT){PostQuitMessage((int)m.wParam);break;}TranslateMessage(&m);DispatchMessageW(&m);}
  if(owner&&IsWindow(owner)){EnableWindow(owner,TRUE);SetForegroundWindow(owner);}
  if(com)CoUninitialize();return 0;
 }
};
int show(HWND owner,const fs::path& root,bool cheatExUnlocked,mcl::Collection& model,bool startInEx,bool cheatsUnlocked){Selection view(root,cheatExUnlocked,model,startInEx,cheatsUnlocked);return view.run(owner);}
}
