#pragma once
#include <cstddef>
#include <vector>
namespace mcl {
// Logical controller inputs (keyboard and gamepads are mapped onto these by the host).
enum class Pad { Up, Down, Left, Right, Cross, Circle, Select, Start };
class SequenceCode {
 std::vector<Pad> seq_;
 std::size_t at_=0;
public:
 explicit SequenceCode(std::vector<Pad> seq):seq_(std::move(seq)){}
 std::size_t progress() const {return at_;}
 // Returns true when the final input of the sequence was just entered.
 bool feed(Pad p){
  at_=(p==seq_[at_])?at_+1:(p==seq_[0]?1:0);
  if(at_==seq_.size()){at_=0;return true;}
  return false;
 }
 void reset(){at_=0;}
};
enum class MenuItem { Arcade, Options, Cheats, CheatEx, Achievements, Gallery, Jukebox, Credits, Exit };
inline const wchar_t* menuLabel(MenuItem i){
 switch(i){
  case MenuItem::Arcade: return L"ARCADE";
  case MenuItem::Options: return L"OPTIONS";
  case MenuItem::Cheats: return L"CHEATS";
  case MenuItem::CheatEx: return L"CHEAT EX";
  case MenuItem::Achievements: return L"ACHIEVEMENTS";
  case MenuItem::Gallery: return L"GALLERY";
  case MenuItem::Jukebox: return L"JUKEBOX";
  case MenuItem::Credits: return L"CREDITS";
  case MenuItem::Exit: return L"EXIT";
 }
 return L"";
}
// Main menu: ARCADE / OPTIONS / [CHEATS] / [CHEAT EX] / ACHIEVEMENTS / GALLERY / JUKEBOX / CREDITS / EXIT.
// CHEATS and CHEAT EX appear only after their codes are entered on this menu.
//   Master cheat code: Up Down Left Right Cross Circle Cross Circle Select Start
//   CHEAT EX code:     Down Right Down Left Cross Circle Start
// Directions in a code still move the cursor (harmless). Once a code is under way, its
// button presses (Cross/Circle/Select/Start) are swallowed so they don't trigger menu actions.
class MainMenuModel {
 SequenceCode master_{{Pad::Up,Pad::Down,Pad::Left,Pad::Right,Pad::Cross,Pad::Circle,Pad::Cross,Pad::Circle,Pad::Select,Pad::Start}};
 SequenceCode ex_{{Pad::Down,Pad::Right,Pad::Down,Pad::Left,Pad::Cross,Pad::Circle,Pad::Start}};
 bool cheats_=false,cheatEx_=false;
 int cursor_=0;
 static bool directional(Pad p){return p==Pad::Up||p==Pad::Down||p==Pad::Left||p==Pad::Right;}
public:
 enum class Kind { None, Moved, Activate, Back, CheatsUnlocked, CheatExUnlocked };
 struct Result { Kind kind=Kind::None; MenuItem item=MenuItem::Arcade; };
 std::vector<MenuItem> items() const {
  std::vector<MenuItem> v{MenuItem::Arcade,MenuItem::Options};
  if(cheats_)v.push_back(MenuItem::Cheats);
  if(cheatEx_)v.push_back(MenuItem::CheatEx);
  v.push_back(MenuItem::Achievements);v.push_back(MenuItem::Gallery);v.push_back(MenuItem::Jukebox);v.push_back(MenuItem::Credits);v.push_back(MenuItem::Exit);
  return v;
 }
 int cursor() const {return cursor_;}
 MenuItem selected() const {return items()[static_cast<std::size_t>(cursor_)];}
 bool cheatsUnlocked() const {return cheats_;}
 bool cheatExUnlocked() const {return cheatEx_;}
 void resetCodes(){master_.reset();ex_.reset();}
 // ACCESSIBILITY shortcuts (X / C): unlock without the code, keeping the cursor on its item.
 void unlockCheats(){const MenuItem c=selected();cheats_=true;keepCursor(c);}
 void unlockCheatEx(){const MenuItem c=selected();cheatEx_=true;keepCursor(c);}
 void keepCursor(MenuItem it){const auto v=items();for(std::size_t i=0;i<v.size();++i)if(v[i]==it)cursor_=static_cast<int>(i);}
 // Feeds one input to both codes. `unlocked` is set when a code completes for the
 // first time; `swallowed` when the input belongs to a code already under way and
 // must not also trigger a screen action.
 struct CodeFeed { Kind unlocked=Kind::None; bool swallowed=false; };
 CodeFeed feedCodes(Pad p){
  const MenuItem current=selected();
  const auto mBefore=master_.progress(),eBefore=ex_.progress();
  const bool mDone=master_.feed(p),eDone=ex_.feed(p);
  const auto keep=[&](MenuItem it){const auto v=items();for(std::size_t i=0;i<v.size();++i)if(v[i]==it)cursor_=static_cast<int>(i);};
  if(eDone&&!cheatEx_){cheatEx_=true;keep(current);return {Kind::CheatExUnlocked,true};}
  if(mDone&&!cheats_){cheats_=true;keep(current);return {Kind::CheatsUnlocked,true};}
  if(!directional(p)){
   const bool mAdv=mDone||master_.progress()>mBefore,eAdv=eDone||ex_.progress()>eBefore;
   if((mAdv&&(mDone||master_.progress()>=2))||(eAdv&&(eDone||ex_.progress()>=2)))return {Kind::None,true};
  }
  return {};
 }
 // Title screen ("PRESS START"): codes work here too. Returns Activate when the
 // player pressed Start/Cross to enter the menu, an unlock kind, or None.
 Result pressOnTitle(Pad p){
  const auto c=feedCodes(p);
  if(c.unlocked!=Kind::None)return {c.unlocked,c.unlocked==Kind::CheatExUnlocked?MenuItem::CheatEx:MenuItem::Cheats};
  if(c.swallowed)return {};
  if(p==Pad::Start||p==Pad::Cross)return {Kind::Activate,selected()};
  return {};
 }
 Result press(Pad p){
  const auto c=feedCodes(p);
  if(c.unlocked!=Kind::None)return {c.unlocked,c.unlocked==Kind::CheatExUnlocked?MenuItem::CheatEx:MenuItem::Cheats};
  if(c.swallowed)return {};
  const int n=static_cast<int>(items().size());
  switch(p){
   case Pad::Up: cursor_=(cursor_+n-1)%n;return {Kind::Moved,selected()};
   case Pad::Down: cursor_=(cursor_+1)%n;return {Kind::Moved,selected()};
   case Pad::Cross: case Pad::Start: return {Kind::Activate,selected()};
   case Pad::Circle: return {Kind::Back,selected()};
   default: return {};
  }
 }
};
}
