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
struct Poke { std::uint32_t addr; std::uint8_t width; std::uint32_t value; }; // width 1/2/4 bytes
struct CheatDef {
 const char* id;      // stable key for saved settings
 const char* label;   // shown in menus
 std::vector<Poke> pokes;
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
public:
 explicit CheatSettings(std::filesystem::path file):file_(std::move(file)){}
 bool load();        // missing file = nothing enabled
 bool save() const;
 bool enabled(const std::string& game,const std::string& id) const;
 void set(const std::string& game,const std::string& id,bool on);
 bool anyEnabled(const std::string& game) const;
 std::vector<Poke> activePokes(const std::string& game) const;
};

// CHEAT EX switches (Health/Power/Time for player 1) -> pokes for sfexp / sfex2p.
// "Unlock all secret characters" has no verified memory cheat yet and is ignored.
std::vector<Poke> exCheatPokes(const std::string& game,bool health,bool power,bool time);

// MAME autoboot Lua that applies the pokes to ":maincpu" every frame.
std::string buildCheatLua(const std::string& title,const std::vector<Poke>& pokes);
}
