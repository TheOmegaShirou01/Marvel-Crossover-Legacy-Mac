#pragma once
#include <string>
#include <vector>
namespace mcl {
// Gameplay detector that runs inside MAME (appended to the autoboot Lua) and reports
// real arcade results for achievements. It reads each game's RAM: both teams' health,
// the round timer and the arcade stage counter. Addresses were taken from MAME's cheat
// database and confirmed on 2026-11-19 by automated arcade runs that reached each
// game's ending (stage counters found by RAM diffs across rounds).
//
// It writes one line per event to eventsFile:
//   <type> <stage> <continued 0/1> <allAlive 0/1> [detail]
// types: coin, start, continue, match_win, match_loss, perfect_round, perfect_match,
//        win_streak, boss_defeated (detail = boss id), arcade_clear, two_player_match.
// Results only count after a human presses START (attract-mode demos never count), and
// a player who loses must press START to continue or the run ends.
struct DetectorProfile {
 std::string game;            // collection id (xmcota, msh, ...)
 std::vector<unsigned> p1,p2; // health byte per character slot
 unsigned maxHealth=0x90;     // anything above this is a KO underflow
 bool zeroIsDead=true;        // EX games: 0 health is still alive until the next hit
 unsigned timer=0, timerFull=0x99;
 unsigned stage=0;            // arcade stage index (0 = first fight)
 int roundsToWin=2;           // tag games end the match on the first team KO
 int finalStage=7;
 std::vector<std::pair<int,std::string>> bosses; // stage index -> boss id
 // Verified final bosses: xmcota Magneto (Juggernaut 1 stage before), msh Thanos, xmvsf
 // Apocalypse, mshvsf Cyber-Akuma (Apocalypse before), mvsc Onslaught, sfexp Garuda,
 // sfex2p M. Bison II (snapshots showed the ending right after each).
};
// Profile for a collection game; empty game id when the game has no detector (MvC2).
DetectorProfile detectorProfile(const std::string& game);
std::string buildAchievementLua(const std::string& game,const std::string& eventsFileUtf8);
// One parsed line of the events file.
struct DetectedEvent { std::string type; int stage=-1; bool continued=false; bool allAlive=false; std::string detail; };
std::vector<DetectedEvent> parseDetectedEvents(const std::string& text);
}
