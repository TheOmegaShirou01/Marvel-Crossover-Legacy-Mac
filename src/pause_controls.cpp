#include "pause_controls.hpp"
namespace mcl {
PauseAction PauseControls::press(GameButton b,std::uint64_t now){
 const auto i=static_cast<unsigned>(b);
 if(i>=down_.size()||down_[i])return PauseAction::None;
 down_[i]=true;
 if(allFive()){
  quickWaiting_=true;quickFired_=false;quickAt_=now;
  waiting_=false;comboConsumed_=true;return PauseAction::None;
 }
 if(both()&&!comboConsumed_&&!quickWaiting_){waiting_=true;startSelectAt_=now;}
 return PauseAction::None;
}
PauseAction PauseControls::release(GameButton b,std::uint64_t now){
 (void)now;
 const auto i=static_cast<unsigned>(b);
 if(i>=down_.size()||!down_[i])return PauseAction::None;
 down_[i]=false;
 // A five-button attempt must never turn into a START+SELECT pause on release.
 if(quickWaiting_){quickWaiting_=false;waiting_=false;comboConsumed_=true;}
 if(!both())waiting_=false;
 if(!down_[0]&&!down_[1])comboConsumed_=false;
 return PauseAction::None;
}
PauseAction PauseControls::tick(std::uint64_t now){
 if(quickWaiting_&&allFive()&&!quickFired_&&now>=quickAt_&&now-quickAt_>=QuickExitHoldMs){
  quickFired_=true;waiting_=false;comboConsumed_=true;
  return PauseAction::ExitToTitle;
 }
 if(waiting_&&both()&&!allFive()&&!comboConsumed_&&now>=startSelectAt_&&now-startSelectAt_>=PauseChordDebounceMs){
  waiting_=false;comboConsumed_=true;
  if(screen_==PauseScreen::Gameplay){screen_=PauseScreen::Options;return PauseAction::OpenPause;}
 }
 return PauseAction::None;
}
}
