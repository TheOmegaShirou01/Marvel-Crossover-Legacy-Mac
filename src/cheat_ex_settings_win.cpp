#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <array>
#include <string>
#include "cheat_ex_settings_win.hpp"
#include "cheat_ex_settings.hpp"
namespace mcl_exui {
constexpr int BOX_ID=9100,SAVE_ID=9200,CANCEL_ID=9201;
constexpr std::array<const wchar_t*,4> labels={L"Infinite Health",L"Infinite Power (Super Gauge)",L"Infinite Time",L"Unlock All Hidden Characters"};
struct Dialog {
 HWND hwnd=nullptr,owner=nullptr;
 std::array<HWND,4> checks{};
 mcl::ExCheatSettings store;
 std::string game;
 Dialog(HWND o,const std::filesystem::path& p,std::string id):owner(o),store(p/L"MCL_UserData"/L"cheat_ex_settings.txt"),game(std::move(id)){store.load();}
 static LRESULT CALLBACK proc(HWND h,UINT m,WPARAM w,LPARAM l){
  auto* self=reinterpret_cast<Dialog*>(GetWindowLongPtrW(h,GWLP_USERDATA));
  if(m==WM_NCCREATE){self=static_cast<Dialog*>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams);self->hwnd=h;SetWindowLongPtrW(h,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self));}
  return self?self->handle(m,w,l):DefWindowProcW(h,m,w,l);
 }
 LRESULT handle(UINT m,WPARAM w,LPARAM l){
  switch(m){
  case WM_CREATE:{
   CreateWindowW(L"STATIC",game=="sfexp"?L"STREET FIGHTER EX PLUS":L"STREET FIGHTER EX2 PLUS",WS_CHILD|WS_VISIBLE,20,15,420,27,hwnd,nullptr,nullptr,nullptr);
   CreateWindowW(L"STATIC",L"Configuration only: emulator cheat activation not yet implemented.",WS_CHILD|WS_VISIBLE,20,43,440,34,hwnd,nullptr,nullptr,nullptr);
   const auto opts=store.options(game);
   for(int i=0;i<4;++i){checks[i]=CreateWindowW(L"BUTTON",labels[i],WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,24,92+i*45,420,34,hwnd,(HMENU)(INT_PTR)(BOX_ID+i),nullptr,nullptr);SendMessageW(checks[i],BM_SETCHECK,opts.flags[i]?BST_CHECKED:BST_UNCHECKED,0);}
   CreateWindowW(L"BUTTON",L"SAVE",WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,125,293,105,32,hwnd,(HMENU)SAVE_ID,nullptr,nullptr);
   CreateWindowW(L"BUTTON",L"CANCEL",WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,260,293,105,32,hwnd,(HMENU)CANCEL_ID,nullptr,nullptr);
   return 0;
  }
  case WM_COMMAND:
   if(LOWORD(w)==SAVE_ID){
    auto& o=store.options(game);
    for(int i=0;i<4;++i)o.flags[i]=SendMessageW(checks[i],BM_GETCHECK,0,0)==BST_CHECKED;
    if(!store.save()){MessageBoxW(hwnd,L"Could not save CHEAT EX preferences.",L"Save error",MB_OK|MB_ICONERROR);return 0;}
    DestroyWindow(hwnd);return 0;
   }
   if(LOWORD(w)==CANCEL_ID){DestroyWindow(hwnd);return 0;}
   break;
  case WM_CLOSE:DestroyWindow(hwnd);return 0;
  case WM_DESTROY:return 0;
  }
  return DefWindowProcW(hwnd,m,w,l);
 }
 void run(){
  WNDCLASSEXW wc{};wc.cbSize=sizeof(wc);wc.lpfnWndProc=proc;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"MCL_CHEAT_EX_SETTINGS_036";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);RegisterClassExW(&wc);
  auto h=CreateWindowExW(WS_EX_DLGMODALFRAME,wc.lpszClassName,L"CHEAT EX SETTINGS",WS_CAPTION|WS_SYSMENU,CW_USEDEFAULT,CW_USEDEFAULT,480,380,owner,nullptr,wc.hInstance,this);
  if(!h)return;
  if(owner)EnableWindow(owner,FALSE);
  ShowWindow(h,SW_SHOW);UpdateWindow(h);
  MSG msg{};while(IsWindow(h)&&GetMessageW(&msg,nullptr,0,0)>0){if(msg.message==WM_QUIT){PostQuitMessage((int)msg.wParam);break;}TranslateMessage(&msg);DispatchMessageW(&msg);}
  if(owner&&IsWindow(owner)){EnableWindow(owner,TRUE);SetForegroundWindow(owner);}
 }
};
void showSettings(HWND owner,const std::filesystem::path& p,const char* id){if(!mcl::ExCheatSettings::validGame(id))return;Dialog d(owner,p,id);d.run();}
}
