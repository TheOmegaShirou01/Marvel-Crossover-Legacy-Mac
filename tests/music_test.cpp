#include "../src/music.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
namespace fs=std::filesystem;
int main(){
 const auto dir=fs::temp_directory_path()/"mcl_music_test";fs::remove_all(dir);fs::create_directories(dir);
 {
  std::ofstream f(dir/"catalog.tsv",std::ios::binary);
  f<<"# header\n";
  const char* games[]={"xmcota","msh","xmvsf","mshvsf","mvsc","mvsc2","sfexp","sfex2p","sfex3"};
  for(const char* g:games)for(int i=0;i<4;++i)f<<g<<'\t'<<g<<"/song"<<i<<".mp3\t0\t120000\tSong "<<i<<"\r\n";
  f<<"mvsc\tmvsc/MVC1_Character_Select_PS1.mp3\t0\t120000\tMVC1_Character_Select_PS1\n";
  f<<"mvsc\tmvsc/Unidentified_XA_Themes/Channel_03/Continue.mp3\t0\t60000\tContinue\n";       // stinger
  f<<"sfexp\tsfexp/x.mp3\t0\t90000\tStreet Fighter EX Plus Alpha - Game Over\n";             // stinger
  f<<"msh\tmsh/short.mp3\t0\t9000\tjingle\n";                                                  // too short
  f<<"bad line without tabs\n"<<"msh\t../evil.mp3\t0\t90000\tx\n"<<"msh\tm.mp3\t5\t2\tbackwards\n";
 }
 const auto all=mcl::loadMusicCatalog(dir/"catalog.tsv");
 assert(all.size()==9*4+4);
 // Marvel pool: six games, stingers / jingles / EX / EX3 excluded.
 mcl::MusicRotation marvel(all,mcl::marvelMusicGames(),7);
 assert(marvel.size()==6*4+1);
 std::map<std::string,int> perGame;std::string prevGame,prevPath;
 for(int i=0;i<120;++i){
  const auto* t=marvel.next();assert(t);
  assert(t->game!="sfexp"&&t->game!="sfex2p"&&t->game!="sfex3");
  assert(t->name.find("Continue")==std::string::npos);
  assert(t->game!=prevGame);          // never the same soundtrack twice in a row
  assert(t->path!=prevPath);
  prevGame=t->game;prevPath=t->path;++perGame[t->game];
 }
 for(const auto& [g,n]:perGame)assert(n==20); // perfectly balanced: 120 plays / 6 games
 // Within one game, every track plays before any repeats.
 {
  mcl::MusicRotation one(all,{"xmcota"},3);std::set<std::string> seen;
  for(int i=0;i<4;++i)assert(seen.insert(one.next()->path).second);
 }
 // Play MvC1 Character Select first, then rotation continues with a different game.
 {
  mcl::MusicRotation r(all,mcl::marvelMusicGames(),11);
  const auto* cs=r.pick("mvsc/MVC1_Character_Select_PS1.mp3");assert(cs&&cs->game=="mvsc");
  assert(r.next()->game!="mvsc");
  assert(!r.pick("nope.mp3"));
 }
 // EX pool: only EX and EX2.
 {
  mcl::MusicRotation ex(all,mcl::exMusicGames(),5);assert(ex.size()==8);
  for(int i=0;i<20;++i){const auto* t=ex.next();assert(t->game=="sfexp"||t->game=="sfex2p");}
 }
 // Captions.
 mcl::Track t;t.name="Street Fighter EX Plus Alpha - Arabesque";assert(mcl::trackCaption(t)=="Arabesque");
 t.name="Spider-Man theme";assert(mcl::trackCaption(t)=="Spider-Man theme");
 t.name="SLUS-00257_MUS01_01006401_0000";assert(mcl::trackCaption(t)=="Original Soundtrack");
 t.name="SLUS-00793_MS02_0000096C (part 2)";assert(mcl::trackCaption(t)=="Original Soundtrack (part 2)");
 t.name="adx_s0a0";assert(mcl::trackCaption(t)=="Stage Theme 11");
 t.name="adx_selc";assert(mcl::trackCaption(t)=="Character Select");
 assert(mcl::loadMusicCatalog(dir/"missing.tsv").empty());
 fs::remove_all(dir);
 std::cout<<"PASS: catalog parsing, stinger filter, balanced no-repeat rotation, EX pool, captions\n";
}
