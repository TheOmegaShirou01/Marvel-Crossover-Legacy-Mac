#ifdef _WIN32
#include "game_launcher.hpp"
#include "achievement_lua.hpp"
#include "accessibility_lua.hpp"
#include "mvc2_onebutton.hpp"
#include "app_settings.hpp"
#include "cheats.hpp"
#include "controls.hpp"
#include "emulator_bridge.hpp"
#include "external_roms.hpp"
#include "mame_backend.hpp"
#include "mame_sets.hpp"
#include "pause_menu_lua.hpp"
#include <fstream>
#include <iterator>
namespace mcl {
namespace fs=std::filesystem;
static std::wstring widen(const std::string& s){return std::wstring(s.begin(),s.end());}
GameLauncher::GameLauncher(fs::path appDir,Collection& model):dir_(std::move(appDir)),model_(model){}
GameLauncher::~GameLauncher(){if(session_&&session_->state()!=RuntimeState::Idle)session_->stop();}
bool GameLauncher::running() const{
 return session_&&session_->state()==RuntimeState::Running&&emulator_&&emulator_->running();
}
GameLauncher::Result GameLauncher::launch(const std::string& game,bool cheatsUnlocked,bool cheatExUnlocked){
 Result r;
 if(session_&&session_->state()!=RuntimeState::Idle){
  if(running()){r.message=L"A game is already running.";return r;}
  session_->stop();
 }
 game_=game;
 // Fresh backend each time so edits to the config files take effect without a restart.
 session_.reset();
 emulator_=std::make_unique<MameBackend>(readMameExecutable(dir_/"mame_executable.txt"));
 emulator_->setFlycastExecutable(readMameExecutable(dir_/"flycast_executable.txt"));
 emulator_->setLogFile(dir_/"Logs"/"mame_last_launch.log");
 emulator_->setWindowed(gamesWindowed_);
 // Player's controls: only when changed from the defaults, so stock setups stay untouched.
 {ControlsConfig controls;
  const bool custom=controls.load(dir_/"MCL_UserData"/"controls.txt")&&!controls.isDefault();
  const fs::path mame=readMameExecutable(dir_/"mame_executable.txt");
  if(custom&&!mame.empty()){
   std::error_code ec;fs::create_directories(mame.parent_path()/"ctrlr",ec);
   std::ofstream(mame.parent_path()/"ctrlr"/"mcl_controls.cfg",std::ios::trunc)<<controls.mameCtrlrXml();
   emulator_->setControllerFile(L"mcl_controls");
  }}
 // MvC2 (Flycast): the player's own MvC2 keys and controller buttons (CONTROLS SETUP > GAME:
 // MARVEL VS. CAPCOM 2), written as Flycast's mappings for arcade games. Flycast keeps one
 // mapping per controller model, so one file is written for every pad Flycast has reported
 // in its log (remembered in MCL_UserData/flycast_pads.txt) plus the common models.
 if(usesFlycast(game)){
  const fs::path fly=readMameExecutable(dir_/"flycast_executable.txt");
  if(!fly.empty()){
   ControlsConfig mvc2(ControlsConfig::Layout::Mvc2);mvc2.load(dir_/"MCL_UserData"/"controls_mvc2.txt");
   // OPTIONS > ACCESSIBILITY: the buttons the front end takes over for 1-button moves
   AppSettings st;st.load(dir_/"MCL_UserData"/"settings.txt");
   const unsigned takeover=mvc2Takeover(st.specialsOn(),st.supersOn());
   const fs::path maps=fly.parent_path()/"mappings";
   std::error_code ec;fs::create_directories(maps,ec);
   std::ofstream(maps/"SDL_Keyboard_arcade.cfg",std::ios::trunc)<<flycastKeyboardCfg(mvc2,takeover);
   std::vector<std::pair<char,std::string>> pads={{'r',"Xbox 360 Controller"},{'r',"XInput Controller"},{'r',"Xbox One Controller"},
    {'r',"Xbox Series X Controller"},{'r',"Controller (XBOX 360 For Windows)"},{'h',"PS3 Controller"},{'h',"PS4 Controller"},
    {'h',"PS5 Controller"},{'h',"Nintendo Switch Pro Controller"}};
   const fs::path known=dir_/"MCL_UserData"/"flycast_pads.txt";
   {std::ifstream in(known);std::string line;
    while(std::getline(in,line))if(line.size()>2&&line[1]=='\t')pads.push_back({line[0],line.substr(2)});}
   {std::ifstream in(fly.parent_path()/"flycast.log");const std::string log((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
    for(const auto& p:flycastPadsFromLog(log))pads.push_back(p);}
   // last word wins (the log is newest), one entry per name
   std::vector<std::pair<char,std::string>> unique;
   for(auto it=pads.rbegin();it!=pads.rend();++it){bool seen=false;for(const auto& u:unique)seen=seen||u.second==it->second;if(!seen)unique.push_back(*it);}
   {std::ofstream out(known,std::ios::trunc);for(const auto& u:unique)if(u.first)out<<u.first<<'\t'<<u.second<<"\n";}
   for(const auto& u:unique){
    const std::string cfg=flycastPadCfg(mvc2,u.first,takeover);if(cfg.empty())continue;
    std::string file="SDL_"+u.second+"_arcade.cfg";
    for(char& ch:file)if(std::string("/\\:?*|\"<>").find(ch)!=std::string::npos)ch='-';
    std::ofstream(maps/fs::u8path(file),std::ios::trunc)<<cfg;
   }
  }
 }
 session_=std::make_unique<EmulatorBridge>(model_,*emulator_,resolveRuntimeRomSearchPath(dir_));
 // Cheats: Marvel ones need the master code, EX ones need CHEAT EX, this session.
 std::vector<Poke> pokes;
 const bool ex=game=="sfexp"||game=="sfex2p";
 if((ex&&cheatExUnlocked)||(!ex&&cheatsUnlocked)){
  CheatSettings s(dir_/"MCL_UserData"/"marvel_cheats.txt");s.load();
  pokes=s.activePokes(game);
 }
 if(usesFlycast(game))pokes.clear(); // Flycast has no Lua hook (no cheats, no custom pause menu)
 // MAME games always get the in-game pause menu script, plus cheats when enabled.
 const fs::path script=dir_/"MCL_UserData"/"game_script.lua";
 const fs::path signal=dir_/"MCL_UserData"/"game_signal.txt";
 const fs::path ready=dir_/"MCL_UserData"/"game_ready.txt";
 const fs::path events=dir_/"MCL_UserData"/"game_events.txt";
 {std::error_code ec;fs::create_directories(script.parent_path(),ec);fs::remove(signal,ec);fs::remove(ready,ec);fs::remove(events,ec);}
 needsReadySignal_=!usesFlycast(game);
 if(!usesFlycast(game)){
  std::string lua=buildPauseMenuLua(signal.u8string(),ready.u8string());
  if(!pokes.empty())lua+="\n"+buildCheatLua(game,pokes);
  lua+="\n"+buildAchievementLua(game,events.u8string()); // read-only result detector
  // OPTIONS > ACCESSIBILITY: 1-button specials / supers
  {AppSettings st;st.load(dir_/"MCL_UserData"/"settings.txt");
   const std::string one=buildOneButtonLua(game,st.specialsOn(),st.supersOn());
   if(!one.empty())lua+="\n"+one;}
  std::ofstream out(script,std::ios::trunc);out<<lua;
  emulator_->setCheatScript(out?script:fs::path{});
 }else emulator_->setCheatScript({});
 const auto prepared=session_->prepare(game);
 if(!prepared.ok){r.message=widen(prepared.message);return r;}
 const auto started=session_->start();
 if(!started.ok){
  session_->stop();
  const auto why=emulator_->lastStartError();
  r.message=why.empty()?L"The emulator did not start. Check the emulator path, game ROM and BIOS.":widen(why);
  return r;
 }
 if(!pokes.empty()){session_->onCheatApplicationStatus(true);r.cheats=true;
  VerifiedEvent c;c.game=game;c.type="cheat_used";model_.processVerifiedEvent(c);}
 {VerifiedEvent e;e.game=game;e.type="launch";session_->onVerifiedGameplayEvent(e);}
 r.ok=true;return r;
}
bool GameLauncher::takeExitSignal(){
 const fs::path signal=dir_/"MCL_UserData"/"game_signal.txt";
 std::ifstream in(signal);std::string word;in>>word;in.close();
 std::error_code ec;fs::remove(signal,ec);
 return word=="exit";
}
bool GameLauncher::requestClose(){return running()&&emulator_->requestClose();}
bool GameLauncher::gameReady() const{
 if(!needsReadySignal_)return true;
 std::error_code ec;return fs::is_regular_file(dir_/"MCL_UserData"/"game_ready.txt",ec);
}
void GameLauncher::bringToFront(){if(emulator_)emulator_->bringToFront();}
bool GameLauncher::flycastGame() const{return running()&&usesFlycast(game_);}
HWND GameLauncher::gameWindow() const{return emulator_?emulator_->window():nullptr;}
bool GameLauncher::freezeGame(bool frozen){return emulator_&&emulator_->freeze(frozen);}
int GameLauncher::gameAdvancing(){return emulator_?emulator_->naomiAdvancing():-1;}
bool GameLauncher::tapGameKey(unsigned vk){return emulator_&&emulator_->tapKey(vk);}
bool GameLauncher::readGameRam(std::uint32_t offset,void* out,std::size_t size){return emulator_&&emulator_->readNaomi(offset,out,size);}
bool GameLauncher::postGameKey(unsigned vk,unsigned scancode,bool down){return emulator_&&emulator_->postKey(vk,scancode,down);}
void GameLauncher::startCoverGuard(HWND cover){if(emulator_)emulator_->startCoverGuard(cover);}
void GameLauncher::stopCoverGuard(){if(emulator_)emulator_->stopCoverGuard();}
void GameLauncher::awardMasterCode(){VerifiedEvent e;e.type="master_code";model_.processVerifiedEvent(e);}
std::vector<std::string> GameLauncher::takeAchievementNotices(){return model_.consumeAchievementNotices();}
// Results reported by the in-game detector (achievement_lua.cpp) go through the session,
// which adds the run's real cheat state, so a script line can never clear that flag.
void GameLauncher::ingestDetectedEvents(){
 const fs::path events=dir_/"MCL_UserData"/"game_events.txt";
 std::string text;
 {std::ifstream in(events,std::ios::binary);if(!in)return;text.assign(std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>());}
 std::error_code ec;fs::remove(events,ec);
 const std::string game=session_->game();
 for(const auto& d:parseDetectedEvents(text)){
  if(d.type=="start"||d.type=="continue")continue;
  VerifiedEvent e;e.game=game;e.type=d.type;e.detail=d.detail;
  e.arcadeMode=d.type!="two_player_match";e.humanControlled=true;
  e.continued=d.continued;e.allCharactersAlive=d.allAlive;
  if(d.type=="match_win"||d.type=="arcade_clear"){e.characters=charactersFromDetail(game,d.detail);e.detail.clear();}
  session_->onVerifiedGameplayEvent(e);
 }
}
bool GameLauncher::pollEnded(std::wstring& failure){
 failure.clear();
 if(!session_||session_->state()!=RuntimeState::Running||!emulator_||emulator_->running())return false;
 unsigned long code=0;double secs=0;
 const bool known=emulator_->lastExit(code,secs);
 const std::string game=session_->game();
 std::string log;
 {std::ifstream in(emulator_->logFile(),std::ios::binary);log.assign(std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>());}
 ingestDetectedEvents(); // before stop(): the session still knows the game and cheat state
 session_->stop();
 if(known&&(code!=0||logShowsFailedStart(log))){
  const bool fly=usesFlycast(game);
  const std::wstring emu=fly?L"Flycast":L"MAME";
  failure=emu+L" could not run this game";
  if(!fly&&code==2)failure+=L" because ROM files are missing or don't match your MAME version";
  failure+=L".\n";
  for(const auto& line:summarizeMameLog(log,8))failure+=L"\n"+widen(line);
  failure+=L"\n\nFull output: "+emulator_->logFile().wstring();
 }
 return true;
}
}
#endif
