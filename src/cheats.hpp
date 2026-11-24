#pragma once
#include <cstdint>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>
namespace mcl {
// Game cheats applied by MAME through an -autoboot_script Lua file that rewrites
// memory every frame (same effect as MAME's own cheat engine "run" scripts).
// Addresses come from the MAME cheat database (cheat.zip found on the user's PC,
// 2010 XML files) and were checked against the exact sets we launch on MAME 0.289
// by sampling them during attract mode (timer counts down, full-health values match).
// width 1/2/4 bytes; with ifAddr set the poke is only written while that byte equals ifValue
// (e.g. only while a human is playing, so the attract-mode demo never sees it), and with
// ifNonZero set only while that byte is not 0 (e.g. once the select screen has been confirmed).
struct Poke { std::uint32_t addr; std::uint8_t width; std::uint32_t value; std::uint32_t ifAddr=0; std::uint8_t ifValue=0; std::uint32_t ifNonZero=0; };
struct CheatChoice { const char* label; std::vector<Poke> pokes; };
struct CheatDef {
 const char* id;      // stable key for saved settings
 const char* label;   // shown in menus
 std::vector<Poke> pokes;
 // A choice cheat (e.g. "Secret Character P1": OFF / Akuma / Juggernaut ...) instead of ON/OFF.
 std::vector<CheatChoice> choices={};
 bool isChoice() const {return !choices.empty();}
};
// Cheats for a collection game id (xmcota, msh, ...). Empty for games without cheats.
const std::vector<CheatDef>& cheatsFor(const std::string& game);
// The five Marvel CPS-2 games that have cheats on the CHEATS screen, in menu order.
const std::vector<std::string>& marvelCheatGames();
// The two CHEAT EX games (their cheats live in the same CheatSettings file).
const std::vector<std::string>& exCheatGames();

// Which cheats are switched on, per game. Plain text file, one "game cheat_id" per line.
class CheatSettings {
 std::filesystem::path file_;
 std::map<std::string,std::set<std::string>> on_;
 std::map<std::string,std::map<std::string,int>> choice_; // game -> cheat id -> 1-based option (0 = off)
public:
 explicit CheatSettings(std::filesystem::path file):file_(std::move(file)){}
 bool load();        // missing file = nothing enabled
 bool save() const;
 bool enabled(const std::string& game,const std::string& id) const;
 void set(const std::string& game,const std::string& id,bool on);
 bool anyEnabled(const std::string& game) const;
 // Choice cheats: 0 = OFF, 1..n = that option. Saved as "game id=n".
 int choice(const std::string& game,const std::string& id) const;
 void setChoice(const std::string& game,const std::string& id,int option);
 std::vector<Poke> activePokes(const std::string& game) const;
};

// CHEAT EX switches (Health/Power/Time for player 1) -> pokes for sfexp / sfex2p.
// "Unlock all secret characters" has no verified memory cheat yet and is ignored.
std::vector<Poke> exCheatPokes(const std::string& game,bool health,bool power,bool time);

// MAME autoboot Lua that applies the pokes to ":maincpu" every frame.
std::string buildCheatLua(const std::string& title,const std::vector<Poke>& pokes);
}
