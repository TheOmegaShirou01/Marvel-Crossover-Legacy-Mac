#include "../src/menu_model.hpp"
#include <cassert>
#include <iostream>
using namespace mcl;
using K=MainMenuModel::Kind;
int main(){
 MainMenuModel m;
 assert(m.items().size()==4&&m.selected()==MenuItem::Arcade);
 // Navigation wraps.
 assert(m.press(Pad::Up).kind==K::Moved&&m.selected()==MenuItem::Exit);
 assert(m.press(Pad::Down).kind==K::Moved&&m.selected()==MenuItem::Arcade);
 // Plain confirm / back.
 auto r=m.press(Pad::Cross);assert(r.kind==K::Activate&&r.item==MenuItem::Arcade);
 assert(m.press(Pad::Start).kind==K::Activate);
 assert(m.press(Pad::Circle).kind==K::Back);
 // CHEAT EX: Down Right Down Left Cross Circle Start. Cross/Circle/Start swallowed mid-code.
 {MainMenuModel e;
  e.press(Pad::Down);e.press(Pad::Right);e.press(Pad::Down);e.press(Pad::Left);
  assert(e.press(Pad::Cross).kind==K::None);
  assert(e.press(Pad::Circle).kind==K::None);
  const MenuItem before=e.selected();
  auto u=e.press(Pad::Start);assert(u.kind==K::CheatExUnlocked);
  assert(e.cheatExUnlocked()&&!e.cheatsUnlocked());
  assert(e.items().size()==5&&e.items()[2]==MenuItem::CheatEx);
  assert(e.selected()==before); // cursor stays on the same item
  // Entering it again is harmless and swallowed.
  e.press(Pad::Down);e.press(Pad::Right);e.press(Pad::Down);e.press(Pad::Left);e.press(Pad::Cross);e.press(Pad::Circle);
  assert(e.press(Pad::Start).kind==K::None);
 }
 // Master: Up Down Left Right Cross Circle Cross Circle Select Start.
 {MainMenuModel c;
  for(Pad p:{Pad::Up,Pad::Down,Pad::Left,Pad::Right})c.press(p);
  for(Pad p:{Pad::Cross,Pad::Circle,Pad::Cross,Pad::Circle,Pad::Select})assert(c.press(p).kind==K::None);
  assert(c.press(Pad::Start).kind==K::CheatsUnlocked);
  assert(c.cheatsUnlocked()&&c.items()[2]==MenuItem::Cheats);
  // Then CHEAT EX too: both appear in order.
  for(Pad p:{Pad::Down,Pad::Right,Pad::Down,Pad::Left,Pad::Cross,Pad::Circle})c.press(p);
  assert(c.press(Pad::Start).kind==K::CheatExUnlocked);
  const auto v=c.items();
  assert(v.size()==6&&v[2]==MenuItem::Cheats&&v[3]==MenuItem::CheatEx&&v[5]==MenuItem::Exit);
 }
 // Title screen: arrows do nothing, X/Enter start the menu, codes still work and
 // their X/C/Enter presses do not start the menu mid-code.
 {MainMenuModel t;
  assert(t.pressOnTitle(Pad::Down).kind==K::None);
  assert(t.pressOnTitle(Pad::Right).kind==K::None);
  assert(t.pressOnTitle(Pad::Down).kind==K::None);
  assert(t.pressOnTitle(Pad::Left).kind==K::None);
  assert(t.pressOnTitle(Pad::Cross).kind==K::None);
  assert(t.pressOnTitle(Pad::Circle).kind==K::None);
  assert(t.pressOnTitle(Pad::Start).kind==K::CheatExUnlocked);
  assert(t.cheatExUnlocked()&&t.items().size()==5);
  assert(t.pressOnTitle(Pad::Start).kind==K::Activate);
  assert(t.pressOnTitle(Pad::Cross).kind==K::Activate);
  MainMenuModel u;
  for(Pad p:{Pad::Up,Pad::Down,Pad::Left,Pad::Right,Pad::Cross,Pad::Circle,Pad::Cross,Pad::Circle,Pad::Select})assert(u.pressOnTitle(p).kind==K::None);
  assert(u.pressOnTitle(Pad::Start).kind==K::CheatsUnlocked);
 }
 // A wrong button breaks the code and acts normally.
 {MainMenuModel w;
  w.press(Pad::Down);w.press(Pad::Right);
  assert(w.press(Pad::Circle).kind==K::Back);
  assert(!w.cheatExUnlocked());
 }
 std::cout<<"PASS: menu navigation, confirm/back, CHEAT EX and master codes, input swallowing\n";
}
