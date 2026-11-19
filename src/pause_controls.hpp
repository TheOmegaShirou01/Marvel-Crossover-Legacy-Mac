#pragma once
#include <array>
#include <cstdint>
namespace mcl {
// Logical controls after user-configurable device mapping. No hardware keycodes here.
enum class GameButton : unsigned { Start, Select, L1, R1, R2, Count };
enum class PauseAction { None, OpenPause, Resume, RequestExit, ExitToGameSelect, ExitToTitle };
enum class PauseScreen { Gameplay, Options, ConfirmExit };
class PauseControls {
 std::array<bool,static_cast<unsigned>(GameButton::Count)> down_{};
 PauseScreen screen_=PauseScreen::Gameplay;
 std::uint64_t startSelectAt_=0,quickAt_=0;
 bool waiting_=false,quickWaiting_=false,quickFired_=false,comboConsumed_=false;
 bool both()const{return down_[0]&&down_[1];}
 bool allFive()const{return down_[0]&&down_[1]&&down_[2]&&down_[3]&&down_[4];}
public:
 static constexpr std::uint64_t QuickExitHoldMs=3000;
 static constexpr std::uint64_t PauseChordDebounceMs=150;
 PauseScreen screen()const{return screen_;}
 void reset(){*this=PauseControls{};}
 // tick is monotonic milliseconds. Returning ExitToTitle/ExitToGameSelect is
 // only a REQUEST; the host must safely close the emulator before navigating.
 PauseAction press(GameButton b,std::uint64_t now);
 PauseAction release(GameButton b,std::uint64_t now);
 PauseAction tick(std::uint64_t now);
 PauseAction continueGame(){if(screen_==PauseScreen::Gameplay)return PauseAction::None;screen_=PauseScreen::Gameplay;return PauseAction::Resume;}
 PauseAction chooseExit(){if(screen_!=PauseScreen::Options)return PauseAction::None;screen_=PauseScreen::ConfirmExit;return PauseAction::RequestExit;}
 PauseAction chooseYes(){if(screen_!=PauseScreen::ConfirmExit)return PauseAction::None;screen_=PauseScreen::Gameplay;return PauseAction::ExitToGameSelect;}
 PauseAction chooseNo(){if(screen_!=PauseScreen::ConfirmExit)return PauseAction::None;screen_=PauseScreen::Gameplay;return PauseAction::Resume;}
};
}
