#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include "core.hpp"
#include "arcade_select.hpp"
#include "title_screen.hpp"
#include "game_launcher.hpp"
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"shell32.lib")
namespace {
using namespace mcl;
std::unique_ptr<Collection> model;
HWND win{},games{},items{},details{},caption{},openBtn{},prevBtn{};
std::vector<Media> selectedMedia;
int page=0;
bool cheatExUnlocked=false; // set by the title screen's main menu (CHEAT EX code)
constexpr int ID_GAME=101,ID_ITEMS=102,ID_OPEN=103,ID_BACK=104,ID_PREVIEW=105,ID_ARCADE=106,ID_NAV=200;
const wchar_t* tabs[]={L"ARCADE THEATER",L"SOUND TEST",L"CHARACTERS",L"ACHIEVEMENTS",L"SAVE STATES",L"STATISTICS",L"ARCADE MUSEUM"};
std::wstring wide(const std::string& s){if(s.empty())return {};int n=MultiByteToWideChar(CP_UTF8,0,s.data(),(int)s.size(),nullptr,0);std::wstring w(n,L'\0');MultiByteToWideChar(CP_UTF8,0,s.data(),(int)s.size(),w.data(),n);return w;}
std::wstring selectedGame(){int idx=(int)SendMessageW(games,LB_GETCURSEL,0,0);if(idx==LB_ERR)idx=0;return wide(model->games().at(idx).id);}
std::string gameId(){int idx=(int)SendMessageW(games,LB_GETCURSEL,0,0);if(idx==LB_ERR)idx=0;return model->games().at(idx).id;}
void setDetails(std::wstring s){SetWindowTextW(details,s.c_str());}
void add(std::wstring s){SendMessageW(items,LB_ADDSTRING,0,(LPARAM)s.c_str());}
void refresh(){
 if(!model)return;SendMessageW(items,LB_RESETCONTENT,0,0);selectedMedia.clear();SetWindowTextW(caption,tabs[page]);
 auto id=gameId();std::wstring summary;
 if(page==0||page==1){auto type=page==0?"theater":"music";selectedMedia=model->media(id,type);for(auto &m:selectedMedia)add(wide(m.title));
   summary=(page==0?L"ARCADE THEATER\r\n":L"SOUND TEST / MUSIC GALLERY\r\n");
   summary+=L"Place legally sourced game media in:\r\n"+wide((model->root()/"media"/id/type).string())+L"\r\n\r\n";
   summary+=L"Select an item and press OPEN MEDIA to play it using your Windows media player.\r\n";
   if(selectedMedia.empty())summary+=L"\r\nNo media imported yet. Original arcade ROM data is NOT extracted or simulated.\r\n";
 }else if(page==2){
   if(!model->galleryUnlocked()){summary=L"GALLERY LOCKED\r\n\r\nBeat Arcade Mode in ANY of the eight games once to unlock the Character Encyclopedia and Arcade Museum.\r\n\r\nUnlock motto: THE LEGENDS LIVE ON!\r\n\r\nThis requires a real verified arcade-clear event from the emulator; opening menus cannot unlock it.";}
   else if(!model->galleryDataLoaded()){summary=L"Gallery data file missing. Install Gallery\\arcade_gallery.mclgallery beside the application executable.";}
   else {for(auto const& c:model->characters())if(c.game==id)add(wide(c.name));summary=L"CHARACTER ENCYCLOPEDIA\r\nOriginal character notes and verified research references. Select a name. Arcade portrait images require later asset extraction.\r\n";}

 }else if(page==3){int earned=0,total=0;for(auto const& a:model->achievements())if(a.game==id){total++;bool done=model->progress(a.id).unlocked;if(done)earned++;add((done?L"[UNLOCKED] ":L"[LOCKED] ")+wide(a.title));}
   summary=L"ACHIEVEMENT GALLERY\r\nUnlocked: "+std::to_wstring(earned)+L" / "+std::to_wstring(total)+L"\r\n\r\nAchievements require verified emulator event hooks. They are not falsely awarded by the frontend.\r\n";
 }else if(page==4){for(int i=1;i<=10;i++){
    auto path=model->root()/"states"/id/("slot"+std::to_string(i)+".state");std::error_code ec;bool exists=std::filesystem::is_regular_file(path,ec);
    add(L"Slot "+std::to_wstring(i)+(exists?L"  [STATE DATA]":L"  [EMPTY]"));
   }summary=L"SAVE STATES\r\n10 reserved per-game state slots.\r\n\r\nSaving/loading will be enabled only after the emulator core supplies real serialization and restore callbacks.\r\nNo fake snapshots or invalid state files will be created.\r\n";
 }else if(page==5){auto s=model->stats(id);summary=L"PLAYER STATISTICS\r\n\r\nWins: "+std::to_wstring(s.wins)+L"\r\nLosses: "+std::to_wstring(s.losses)+L"\r\nCredits: "+std::to_wstring(s.credits)+L"\r\nGame launches: "+std::to_wstring(s.launches)+L"\r\nArcade clears: "+std::to_wstring(s.clears)+L"\r\nPlaytime: "+std::to_wstring(s.seconds/60)+L" minutes\r\n\r\nCounters update from verified gameplay hooks; not from clicking around this demo.\r\n";
 }else if(page==6){for(auto const& g:model->games())add(wide(g.title));auto const& g=model->games()[SendMessageW(games,LB_GETCURSEL,0,0)==LB_ERR?0:SendMessageW(games,LB_GETCURSEL,0,0)];
 summary=L"ARCADE MUSEUM\r\n\r\n"+wide(g.title)+L"\r\nReleased: "+wide(g.year)+L"\r\nHardware: "+wide(g.hardware)+L"\r\n\r\n"+wide(g.description)+L"\r\n\r\nBrowse the full game history list at right.\r\n";
 }
 EnableWindow(openBtn,(page==0||page==1)&&!selectedMedia.empty());SetWindowTextW(openBtn,(page==0||page==1)?L"OPEN MEDIA":L"MEDIA ONLY");setDetails(summary);
}
void itemSelected(){int idx=(int)SendMessageW(items,LB_GETCURSEL,0,0);if(idx==LB_ERR)return;auto id=gameId();
 if(!model->galleryUnlocked())return;
 if(page==2){int i=0;for(auto const& c:model->characters())if(c.game==id){if(i++==idx){setDetails(L"CHARACTER ENCYCLOPEDIA\r\n\r\n"+wide(c.name)+L"\r\n\r\n"+wide(c.notes)+L"\r\n\r\nRole: "+wide(c.role)+L"\r\nPortrait: awaiting original arcade extraction");break;}}}
 if(page==3){int i=0;for(auto const& a:model->achievements())if(a.game==id){if(i++==idx){auto p=model->progress(a.id);setDetails(wide(a.title)+L"\r\n\r\nCondition: "+wide(a.condition)+L"\r\nTier: "+wide(a.tier)+L"\r\nStatus: "+(p.unlocked?L"UNLOCKED":L"LOCKED")+L"\r\n"+(p.unlocked?L"Earned (UTC): "+wide(p.timestamp):L"Awaiting genuine gameplay event"));break;}}}
 if(page==6 && idx<(int)model->galleryGames().size()){auto const& g=model->galleryGames()[idx];setDetails(L"ARCADE MUSEUM\r\n\r\n"+wide(g.title)+L"\r\nYear: "+wide(g.year)+L"\r\nHardware: "+wide(g.hardware)+L"\r\nDeveloper: "+wide(g.developer)+L"\r\n\r\n"+wide(g.history)+L"\r\n\r\n"+wide(g.mechanics)+L"\r\nSources:\r\n"+wide(g.sources));}
}
void openPreview(){
 const auto path=model->root().parent_path()/"Previews"/(gameId()+".mp4");
 std::error_code ec;
 if(!std::filesystem::is_regular_file(path,ec)){
  MessageBoxW(win,L"Preview not installed. Run the build script to copy the Previews folder beside the executable.",L"Missing preview",MB_ICONWARNING);return;
 }
 auto w=path.wstring();
 auto r=(INT_PTR)ShellExecuteW(win,L"open",w.c_str(),nullptr,nullptr,SW_SHOWNORMAL);
 if(r<=32)MessageBoxW(win,L"Windows could not open this preview.",L"Media error",MB_ICONERROR);
}
void openMedia(){int i=(int)SendMessageW(items,LB_GETCURSEL,0,0);if((page!=0&&page!=1)||i<0||i>=(int)selectedMedia.size())return;
 auto w=selectedMedia[i].path.wstring();auto r=(INT_PTR)ShellExecuteW(win,L"open",w.c_str(),nullptr,nullptr,SW_SHOWNORMAL);if(r<=32)MessageBoxW(win,L"Windows could not open the selected media file.",L"Media error",MB_ICONERROR);}
HFONT makeFont(int points,bool bold){HDC dc=GetDC(nullptr);int h=-MulDiv(points,GetDeviceCaps(dc,LOGPIXELSY),72);ReleaseDC(nullptr,dc);return CreateFontW(h,0,0,0,bold?FW_BOLD:FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");}
HFONT font{},heading{};
HWND create(const wchar_t* type,const wchar_t* label,DWORD style,int x,int y,int w,int h,int id=0){HWND c=CreateWindowExW(0,type,label,style|WS_CHILD|WS_VISIBLE,x,y,w,h,win,(HMENU)(INT_PTR)id,GetModuleHandleW(nullptr),nullptr);SendMessageW(c,WM_SETFONT,(WPARAM)font,TRUE);return c;}
LRESULT CALLBACK wndProc(HWND h,UINT msg,WPARAM wp,LPARAM lp){
 switch(msg){
 case WM_CREATE:{win=h;font=makeFont(10,false);heading=makeFont(17,true);
 auto title=create(L"STATIC",L"MARVEL: CROSSOVER LEGACY   |   OPTIONS (DEVELOPER FEATURES)",SS_LEFT,18,12,860,38);SendMessageW(title,WM_SETFONT,(WPARAM)heading,TRUE);
 create(L"STATIC",L"SELECT GAME",SS_LEFT,18,56,245,22);games=create(L"LISTBOX",L"",LBS_NOTIFY|WS_BORDER|WS_VSCROLL,18,80,300,190,ID_GAME);
 for(auto const& g:model->games())if(g.id!="sfexp"&&g.id!="sfex2p")SendMessageW(games,LB_ADDSTRING,0,(LPARAM)wide(g.title).c_str());SendMessageW(games,LB_SETCURSEL,0,0);
 for(int i=0;i<7;i++){auto b=create(L"BUTTON",tabs[i],BS_PUSHBUTTON,18+(i%2)*152,285+(i/2)*42,148,35,ID_NAV+i);(void)b;}
 caption=create(L"STATIC",L"",SS_LEFT,344,58,565,32);SendMessageW(caption,WM_SETFONT,(WPARAM)heading,TRUE);
 items=create(L"LISTBOX",L"",LBS_NOTIFY|WS_BORDER|WS_VSCROLL,344,95,565,238,ID_ITEMS);
 details=create(L"EDIT",L"",WS_BORDER|WS_VSCROLL|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL,344,346,565,185);
 openBtn=create(L"BUTTON",L"OPEN MEDIA",BS_PUSHBUTTON,344,548,160,38,ID_OPEN);
 create(L"STATIC",L"Previews are authentic gameplay clips; emulator integration is pending.",SS_LEFT,715,550,190,48);
 prevBtn=create(L"BUTTON",L"PLAY GAME PREVIEW",BS_PUSHBUTTON,515,548,195,38,ID_PREVIEW);
 create(L"BUTTON",L"OPEN ARCADE SELECT (EARTH + VIDEO CARDS)",BS_PUSHBUTTON,18,555,300,38,ID_ARCADE);
 refresh();return 0;}
 case WM_COMMAND:{int id=LOWORD(wp);if(id>=ID_NAV&&id<ID_NAV+7){page=id-ID_NAV;refresh();return 0;}
 if(id==ID_GAME&&HIWORD(wp)==LBN_SELCHANGE){refresh();return 0;}
 if(id==ID_ITEMS&&HIWORD(wp)==LBN_SELCHANGE){itemSelected();return 0;}
 if(id==ID_ITEMS&&HIWORD(wp)==LBN_DBLCLK){openMedia();return 0;}
 if(id==ID_OPEN){openMedia();return 0;}
 if(id==ID_PREVIEW){openPreview();return 0;}
 if(id==ID_ARCADE){mcl_select::show(win,model->root().parent_path(),cheatExUnlocked,*model);return 0;}break;}
 case WM_DESTROY:DeleteObject(font);DeleteObject(heading);win=nullptr;return 0;
 }return DefWindowProcW(h,msg,wp,lp);
}
// OPTIONS: the original feature browser, shown as a modal window over the title screen.
void showFeatureWindow(HWND owner,bool exUnlocked){
 cheatExUnlocked=exUnlocked;
 static bool registered=false;
 HINSTANCE inst=GetModuleHandleW(nullptr);
 if(!registered){
  WNDCLASSEXW wc{};wc.cbSize=sizeof(wc);wc.lpfnWndProc=wndProc;wc.hInstance=inst;wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);wc.lpszClassName=L"MCLFeaturesFoundation";
  registered=RegisterClassExW(&wc)!=0;
 }
 HWND w=CreateWindowExW(0,L"MCLFeaturesFoundation",L"Marvel: Crossover Legacy - Options",WS_OVERLAPPEDWINDOW&~WS_MAXIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,944,650,owner,nullptr,inst,nullptr);
 if(!w)return;
 EnableWindow(owner,FALSE);ShowWindow(w,SW_SHOW);UpdateWindow(w);
 MSG msg;
 while(IsWindow(w)){
  const BOOL r=GetMessageW(&msg,nullptr,0,0);
  if(r<=0){if(r==0)PostQuitMessage(static_cast<int>(msg.wParam));break;}
  TranslateMessage(&msg);DispatchMessageW(&msg);
 }
 EnableWindow(owner,TRUE);SetForegroundWindow(owner);
}
}
int WINAPI wWinMain(HINSTANCE inst,HINSTANCE,LPWSTR,int){
 wchar_t exe[MAX_PATH];GetModuleFileNameW(nullptr,exe,MAX_PATH);
 const std::filesystem::path appDir=std::filesystem::path(exe).parent_path();
 model=std::make_unique<mcl::Collection>(appDir/"MCL_UserData");
 INITCOMMONCONTROLSEX ic{sizeof(ic),ICC_STANDARD_CLASSES};InitCommonControlsEx(&ic);
 mcl::GameLauncher launcher(appDir,*model);
 mcl_title::Callbacks cb;
 cb.launcher=&launcher;
 cb.options=[](HWND owner,bool exUnlocked){showFeatureWindow(owner,exUnlocked);};
 return mcl_title::run(inst,appDir,cb);
}
