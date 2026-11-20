#include "../src/app_settings.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
namespace fs=std::filesystem;
int main(){
 const auto dir=fs::temp_directory_path()/"mcl_settings_test";fs::remove_all(dir);
 const auto file=dir/"settings.txt";
 mcl::AppSettings d;
 assert(!d.load(file)); // missing file keeps defaults
 assert(d.musicVolume==8&&d.voiceVolume==10&&d.fullscreen&&d.gamesFullscreen&&d.backgroundVideo&&d.nowPlaying);
 mcl::AppSettings s;s.musicVolume=3;s.voiceVolume=0;s.fullscreen=false;s.gamesFullscreen=false;s.backgroundVideo=false;s.nowPlaying=false;
 assert(s.save(file));
 mcl::AppSettings r;assert(r.load(file));
 assert(r.musicVolume==3&&r.voiceVolume==0&&!r.fullscreen&&!r.gamesFullscreen&&!r.backgroundVideo&&!r.nowPlaying);
 // Junk is ignored, out-of-range numbers are clamped, CRLF tolerated.
 {std::ofstream f(file,std::ios::trunc|std::ios::binary);f<<"music_volume=99\r\nvoice_volume=-4\r\nfullscreen=maybe\r\nnonsense\r\nnow_playing=on\r\nmystery=1\r\n";}
 mcl::AppSettings j;assert(j.load(file));
 assert(j.musicVolume==10&&j.voiceVolume==10&&j.fullscreen&&j.nowPlaying);
 fs::remove_all(dir);
 std::cout<<"PASS: settings defaults, round trip, junk/clamping\n";
}
