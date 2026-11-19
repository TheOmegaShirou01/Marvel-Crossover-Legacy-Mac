#include "../src/pause_controls.hpp"
#include <cassert>
#include <iostream>
using namespace mcl;
int main(){
 PauseControls p;
 assert(p.press(GameButton::Start,0)==PauseAction::None);
 assert(p.press(GameButton::Select,10)==PauseAction::None);
 assert(p.tick(100)==PauseAction::None);
 assert(p.tick(160)==PauseAction::OpenPause);
 assert(p.screen()==PauseScreen::Options);
 assert(p.chooseExit()==PauseAction::RequestExit);
 assert(p.screen()==PauseScreen::ConfirmExit);
 assert(p.chooseNo()==PauseAction::Resume);
 assert(p.screen()==PauseScreen::Gameplay);
 p.reset();
 for(auto b:{GameButton::Start,GameButton::Select,GameButton::L1,GameButton::R1,GameButton::R2})p.press(b,100);
 assert(p.tick(250)==PauseAction::None); // no accidental pause
 assert(p.tick(3099)==PauseAction::None);
 assert(p.tick(3100)==PauseAction::ExitToTitle);
 assert(p.tick(3200)==PauseAction::None); // one shot
 p.reset();
 for(auto b:{GameButton::Start,GameButton::Select,GameButton::L1,GameButton::R1,GameButton::R2})p.press(b,0);
 p.release(GameButton::R2,1500);
 assert(p.tick(5000)==PauseAction::None);
 p.release(GameButton::Select,5000);
 p.release(GameButton::Start,5000);
 assert(p.screen()==PauseScreen::Gameplay);
 p.press(GameButton::Start,6000);p.press(GameButton::Select,6100);
 assert(p.tick(6250)==PauseAction::OpenPause);
 assert(p.chooseExit()==PauseAction::RequestExit);
 assert(p.chooseYes()==PauseAction::ExitToGameSelect);
 assert(p.screen()==PauseScreen::Gameplay);
 std::cout<<"PASS: pause, confirm-No resume, confirm-Yes exit, 3s quick-exit priority, cancellation, rearm\n";
}
