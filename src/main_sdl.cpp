// macOS / Linux entry point (Windows keeps main_win.cpp).
//
// On a Mac the game ships as one app: everything (logos, music, videos, gallery, ROMs, MAME)
// is inside Marvel Crossover Legacy.app/Contents/Resources, which is read-only (and moved to
// a random read-only place by macOS "App Translocation" when an unsigned app is opened from
// Downloads). The front end expects one writable folder holding both its content and its
// saves, so it runs from ~/Library/Application Support/Marvel Crossover Legacy, where every
// content item is a symbolic link back into the app, refreshed at every start. Saves,
// settings, logs and the emulators' own config folders are real folders there.
#ifndef _WIN32
#include "title_screen.hpp"
#include "autoconfig.hpp"
#include "autosave.hpp"
#include "core.hpp"
#include "game_launcher.hpp"
#include <SDL3/SDL.h>
#include <cstdlib>
#include <filesystem>
#include <set>
#include <string>
namespace fs=std::filesystem;
namespace {
// Never links: written by the game or the player.
const std::set<std::string> kWritable={"MCL_UserData","Logs","Backups"};
// Copied once (the player or auto-setup may change them): emulator and ROM paths.
const std::set<std::string> kCopyOnce={"mame_executable.txt","flycast_executable.txt","rom_directory.txt"};
// Real folders whose contents are linked one by one: emulators write cfg, nvram, hiscore,
// ctrlr, mappings ... next to their program.
const std::set<std::string> kDeep={"MAME","Flycast"};

fs::path userDir(){
 const char* home=std::getenv("HOME");
#ifdef __APPLE__
 if(home&&*home)return fs::path(home)/"Library"/"Application Support"/"Marvel Crossover Legacy";
#endif
 if(char* pref=SDL_GetPrefPath("MarvelCrossoverLegacy","MarvelCrossoverLegacy")){fs::path p=fs::u8path(pref);SDL_free(pref);return p;}
 return fs::path(home?home:".")/".marvel-crossover-legacy";
}
// dst becomes a symbolic link to src (replaced when it points elsewhere).
void linkTo(const fs::path& src,const fs::path& dst){
 std::error_code ec;
 if(fs::is_symlink(fs::symlink_status(dst,ec))){
  if(fs::read_symlink(dst,ec)==src)return;
  fs::remove(dst,ec);
 }else if(fs::exists(fs::symlink_status(dst,ec))){
  return; // a real file or folder the player made: leave it
 }
 if(fs::is_directory(src,ec))fs::create_directory_symlink(src,dst,ec);
 else fs::create_symlink(src,dst,ec);
}
void mirror(const fs::path& from,const fs::path& to,bool top){
 std::error_code ec;
 fs::create_directories(to,ec);
 for(const auto& e:fs::directory_iterator(from,ec)){
  const std::string name=e.path().filename().u8string();
  if(name.empty()||name[0]=='.')continue;
  const fs::path dst=to/e.path().filename();
  if(top&&kWritable.count(name))continue;
  if(top&&kCopyOnce.count(name)){if(!fs::exists(dst,ec))fs::copy_file(e.path(),dst,ec);continue;}
  if(top&&kDeep.count(name)&&e.is_directory(ec)){
   if(fs::is_symlink(fs::symlink_status(dst,ec)))fs::remove(dst,ec);
   mirror(e.path(),dst,false);continue;
  }
  linkTo(e.path(),dst);
 }
 // links left over from an older version whose target is gone
 for(const auto& e:fs::directory_iterator(to,ec))
  if(fs::is_symlink(e.symlink_status(ec))&&!fs::exists(e.path(),ec))fs::remove(e.path(),ec);
}
}

int main(int,char**){
 const char* base=SDL_GetBasePath(); // .../Contents/Resources/ inside an app bundle
 const fs::path content=base?fs::u8path(base):fs::current_path();
 const fs::path appDir=userDir();
 mirror(content,appDir,true);
 {std::error_code ec;
  if(!fs::is_directory(appDir/"Logos",ec)&&!fs::is_directory(appDir/"Gallery",ec)){
   SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,"Marvel: Crossover Legacy",
    ("The game files were not found.\n\nLooked in:\n"+content.u8string()).c_str(),nullptr);
   return 1;
  }}
 try{mcl::autoConfigure(appDir);}catch(...){} // never let setup stop the game from starting
 {std::error_code ec;fs::create_directories(appDir/"MCL_UserData",ec);fs::create_directories(appDir/"Logs",ec);}
 mcl::restoreIfNeeded(appDir/"MCL_UserData");
 mcl::backupUserData(appDir/"MCL_UserData",mcl::backupStamp());
 mcl::Collection model(appDir/"MCL_UserData");
 mcl::GameLauncher launcher(appDir,model);
 mcl_title::Callbacks cb;
 cb.launcher=&launcher;
 return mcl_title::run(nullptr,appDir,cb);
}
#endif
