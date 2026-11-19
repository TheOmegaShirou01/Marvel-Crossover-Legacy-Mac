#ifdef _WIN32
#include "music_player.hpp"
#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>
#include <algorithm>
#include <string>
#pragma comment(lib,"winmm.lib")
namespace mcl {
static const wchar_t* kAlias=L"mcl_music";
static bool mci(const std::wstring& cmd,wchar_t* out=nullptr,UINT outLen=0){
 return mciSendStringW(cmd.c_str(),out,outLen,nullptr)==0;
}
bool MusicPlayer::play(const std::filesystem::path& file,int startMs,int endMs,int volume){
 stop();
 std::error_code ec;
 if(!std::filesystem::is_regular_file(file,ec))return false;
 if(!mci(L"open \""+file.wstring()+L"\" type mpegvideo alias "+kAlias))return false;
 open_=true;
 mci(std::wstring(L"set ")+kAlias+L" time format milliseconds");
 setVolume(volume);
 if(!mci(std::wstring(L"play ")+kAlias+L" from "+std::to_wstring(std::max(0,startMs))+L" to "+std::to_wstring(endMs))){stop();return false;}
 return true;
}
void MusicPlayer::stop(){
 if(!open_)return;
 mci(std::wstring(L"stop ")+kAlias);
 mci(std::wstring(L"close ")+kAlias);
 open_=false;
}
void MusicPlayer::setVolume(int v){
 if(open_)mci(std::wstring(L"setaudio ")+kAlias+L" volume to "+std::to_wstring(std::clamp(v,0,1000)));
}
int MusicPlayer::positionMs() const{
 if(!open_)return 0;
 wchar_t buf[64]{};
 if(!mci(std::wstring(L"status ")+kAlias+L" position",buf,64))return 0;
 return _wtoi(buf);
}
bool MusicPlayer::playing() const{
 if(!open_)return false;
 wchar_t buf[64]{};
 if(!mci(std::wstring(L"status ")+kAlias+L" mode",buf,64))return false;
 return std::wstring(buf)==L"playing";
}
}
#endif
