#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
namespace mcl {
// OPTIONS > ACCESSIBILITY for Marvel vs. Capcom 2 (Flycast has no scripting, so the front end
// does it). Flycast's control files leave out the buttons we take over (LP/LK for specials,
// HP/HK for supers); the front end reads those buttons itself and types into Flycast through
// spare keys no keyboard has (F13-F24 and international keys, bound in the keyboard mapping):
//   outside a fight it just passes each press/release on;
//   in a fight:  LP = QCF+LP, LK = QCB+LK, HP = QCF+LP+HP (hyper), HK = QCF+LK+HK (hyper),
//                with "forward" from the on-screen fighter's facing in the game's memory.
// MvC2 RAM (NAOMI, verified with Flycast): fighter slots at 0x2D7088 + n*0x5A4 in the order
// P1-1, P2-1, P1-2, P2-2, P1-3, P2-3; +0x000 = 1 for the fighter on screen, +0x110 bit 0 set =
// facing right, +0x420 = health (max 144); round timer at 0x2F8378.
enum Mvc2Button { kLP=1, kHP=2, kLK=4, kHK=8 };
struct InjectKey { unsigned short vk, scancode; int sdl; };
// The spare key for player (0/1) and function: 0 up, 1 down, 2 left, 3 right, 4 LP, 5 HP, 6 LK, 7 HK.
const InjectKey& mvc2InjectKey(int player,int function);
// Flycast option names for those functions ("btn_dpad1_up", "btn_a", ...).
const char* mvc2InjectOption(int function);
// Buttons the front end takes over for the chosen features.
unsigned mvc2Takeover(bool specials,bool supers);

class Mvc2OneButton {
public:
 using ReadRam=std::function<bool(std::uint32_t offset,void* out,std::size_t size)>;
 using PostKey=std::function<void(const InjectKey&,bool down)>;
 void configure(bool specials,bool supers){takeover_=mvc2Takeover(specials,supers);reset();}
 unsigned takeover() const {return takeover_;}
 // held[player] = the player's real LP/HP/LK/HK buttons (Mvc2Button bits). nowMs = a clock.
 void tick(double nowMs,const unsigned held[2],const ReadRam& read,const PostKey& post);
 void reset();
 // Releases every spare key it holds down (pause screen, focus lost, game end).
 void releaseAll(const PostKey& post);
 // Exposed for tests.
 bool inFight(double nowMs,const ReadRam& read);
 bool facingRight(int player,const ReadRam& read);
private:
 struct Step { unsigned keys; double until; }; // keys: bit per function (0..7)
 struct Player { unsigned prev=0, down=0; std::vector<Step> queue; std::size_t at=0; };
 unsigned takeover_=0;
 Player p_[2];
 int lastTimer_=-1; double timerMoved_=-1e9;
 void apply(int player,unsigned keys,const PostKey& post);
};
}
