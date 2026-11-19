#include "pause_menu_win.hpp"
#include "pause_controls.hpp"
#ifdef _WIN32
#include <string>
namespace mcl_pause_ui {
namespace {
constexpr int CONTINUE_ID=901,EXIT_ID=902,YES_ID=903,NO_ID=904;
class PausePreview {
 HWND h_=nullptr,owner_=nullptr,label_=nullptr;
 HWND buttons_[4]{};
 HFONT font_=nullptr;
 HBRUSH brush_=nullptr;
 mcl::PauseControls state_;
 bool confirmedExit_=false;
 void update(){
  const bool confirm=state_.screen()==mcl::PauseScreen::ConfirmExit;
  SetWindowTextW(label_,confirm?L"ARE YOU SURE?":L"PAUSE MENU");
  ShowWindow(buttons_[0],confirm?SW_HIDE:SW_SHOW);
  ShowWindow(buttons_[1],confirm?SW_HIDE:SW_SHOW);
  ShowWindow(buttons_[2],confirm?SW_SHOW:SW_HIDE);
  ShowWindow(buttons_[3],confirm?SW_SHOW:SW_HIDE);
 }
 static LRESULT CALLBACK proc(HWND h,UINT msg,WPARAM w,LPARAM l){
  auto* self=reinterpret_cast<PausePreview*>(GetWindowLongPtrW(h,GWLP_USERDATA));
  if(msg==WM_NCCREATE){self=reinterpret_cast<PausePreview*>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams);SetWindowLongPtrW(h,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self));self->h_=h;}
  return self?self->handle(msg,w,l):DefWindowProcW(h,msg,w,l);
 }
 LRESULT handle(UINT msg,WPARAM w,LPARAM l){
  switch(msg){
   case WM_CREATE:{
    brush_=CreateSolidBrush(RGB(12,19,43));
    font_=CreateFontW(-25,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Arial Black");
    CreateWindowW(L"STATIC",L"PREVIEW ONLY - NO EMULATOR PAUSE",WS_CHILD|WS_VISIBLE,28,22,395,25,h_,nullptr,nullptr,nullptr);
    label_=CreateWindowW(L"STATIC",L"PAUSE MENU",WS_CHILD|WS_VISIBLE,65,65,340,42,h_,nullptr,nullptr,nullptr);
    const wchar_t* names[]={L"CONTINUE",L"EXIT GAME",L"YES",L"NO"};
    for(int i=0;i<4;++i){buttons_[i]=CreateWindowW(L"BUTTON",names[i],WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,58,125+i%2*54,340,44,h_,reinterpret_cast<HMENU>(static_cast<INT_PTR>(CONTINUE_ID+i)),nullptr,nullptr);SendMessageW(buttons_[i],WM_SETFONT,reinterpret_cast<WPARAM>(font_),TRUE);}
    SendMessageW(label_,WM_SETFONT,reinterpret_cast<WPARAM>(font_),TRUE);
    state_.press(mcl::GameButton::Start,0);state_.press(mcl::GameButton::Select,0);state_.tick(150);
    update();return 0;
   }
   case WM_CTLCOLORSTATIC:{SetBkColor(reinterpret_cast<HDC>(w),RGB(12,19,43));SetTextColor(reinterpret_cast<HDC>(w),RGB(244,214,95));return reinterpret_cast<LRESULT>(brush_);}
   case WM_COMMAND:{
    switch(LOWORD(w)){
     case CONTINUE_ID:state_.continueGame();DestroyWindow(h_);return 0;
     case EXIT_ID:state_.chooseExit();update();return 0;
     case YES_ID:confirmedExit_=state_.chooseYes()==mcl::PauseAction::ExitToGameSelect;DestroyWindow(h_);return 0;
     case NO_ID:state_.chooseNo();DestroyWindow(h_);return 0;
    }return 0;
   }
   case WM_CLOSE:state_.continueGame();DestroyWindow(h_);return 0;
   case WM_DESTROY:if(font_)DeleteObject(font_);if(brush_)DeleteObject(brush_);return 0;
  }
  return DefWindowProcW(h_,msg,w,l);
 }
public:
 bool run(HWND owner){
  owner_=owner;
  const wchar_t* name=L"MCL_Pause_Preview_034";
  WNDCLASSW wc{};wc.lpfnWndProc=proc;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=name;wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);
  if(!RegisterClassW(&wc)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)return false;
  HWND h=CreateWindowExW(WS_EX_DLGMODALFRAME,name,L"Marvel: Crossover Legacy - Pause Preview",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,CW_USEDEFAULT,CW_USEDEFAULT,475,265,owner,nullptr,wc.hInstance,this);
  if(!h)return false;
  if(owner)EnableWindow(owner,FALSE);
  ShowWindow(h,SW_SHOW);UpdateWindow(h);
  MSG msg{};while(IsWindow(h)){
   auto r=GetMessageW(&msg,nullptr,0,0);
   if(r<=0){if(r==0)PostQuitMessage(static_cast<int>(msg.wParam));break;}
   TranslateMessage(&msg);DispatchMessageW(&msg);
  }
  if(IsWindow(h))DestroyWindow(h);
  if(owner&&IsWindow(owner)){EnableWindow(owner,TRUE);SetForegroundWindow(owner);}
  return confirmedExit_;
 }
};
}
bool showPreview(HWND owner){PausePreview p;return p.run(owner);}
}
#endif
