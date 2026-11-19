#pragma once
namespace mcl {
enum class PausePhase { Idle, RequestSent, ConfirmedPaused, ResumeSent, Failed };
enum class PauseCommand { None, SendPauseKey, ShowMenu, HideMenu, Error };
class PauseTransaction {
 PausePhase phase_=PausePhase::Idle;
public:
 PausePhase phase() const { return phase_; }
 bool canShowOverlay() const { return phase_==PausePhase::ConfirmedPaused; }
 PauseCommand begin(bool running, bool foreground) {
  if(phase_!=PausePhase::Idle || !running || !foreground)return PauseCommand::Error;
  phase_=PausePhase::RequestSent;return PauseCommand::SendPauseKey;
 }
 PauseCommand requestDelivered(bool delivered){
  if(phase_!=PausePhase::RequestSent)return PauseCommand::Error;
  if(!delivered){phase_=PausePhase::Failed;return PauseCommand::Error;}
  // Key delivery does not prove the emulator is paused.
  return PauseCommand::None;
 }
 PauseCommand confirmPaused(bool independentlyVerified){
  if(phase_!=PausePhase::RequestSent || !independentlyVerified)return PauseCommand::Error;
  phase_=PausePhase::ConfirmedPaused;return PauseCommand::ShowMenu;
 }
 PauseCommand resume(){
  if(phase_!=PausePhase::ConfirmedPaused)return PauseCommand::Error;
  phase_=PausePhase::ResumeSent;return PauseCommand::SendPauseKey;
 }
 PauseCommand confirmResumed(bool independentlyVerified){
  if(phase_!=PausePhase::ResumeSent || !independentlyVerified)return PauseCommand::Error;
  phase_=PausePhase::Idle;return PauseCommand::HideMenu;
 }
 void reset(){phase_=PausePhase::Idle;}
};
}
