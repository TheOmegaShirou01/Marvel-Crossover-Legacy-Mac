#pragma once
#include <cstdint>
#include <filesystem>
#include <map>
#include <random>
#include <string>
#include <vector>
namespace mcl {
// One stretch of music inside a soundtrack file (see tools/music_scan.cpp, Music/catalog.tsv).
struct Track {
 std::string game;   // folder name: xmcota, msh, xmvsf, mshvsf, mvsc, mvsc2, sfexp, sfex2p, sfex3
 std::string path;   // relative to the Music folder, forward slashes
 std::string name;   // file stem (+ part number)
 int startMs=0,endMs=0;
};
std::vector<Track> loadMusicCatalog(const std::filesystem::path& file);
// False for short stingers that should never play in the menus (continue, game over...).
bool isMenuMusic(const Track& t);
// Pools for the menus: Marvel menus use the six Marvel soundtracks (endings included);
// CHEAT EX uses only SF EX / EX2. SF EX3 is jukebox-only and never in a pool.
const std::vector<std::string>& marvelMusicGames();
const std::vector<std::string>& exMusicGames();
// Friendly caption, e.g. "SPIDER-MAN THEME" or "TRACK 7" for unnamed rips.
std::string trackCaption(const Track& t);

// Shuffled rotation: balances plays across the pool's games (the least-played game goes
// next, never the same game twice in a row when another is available), shuffles within
// each game, and never repeats a track until that game's other tracks have played.
class MusicRotation {
public:
 MusicRotation(const std::vector<Track>& all,const std::vector<std::string>& games,std::uint32_t seed);
 bool empty() const {return tracks_.empty();}
 const Track* next();
 const Track* pick(const std::string& path); // play this one next (e.g. MvC1 Character Select)
 std::size_t size() const {return tracks_.size();}
private:
 std::vector<Track> tracks_;
 std::map<std::string,std::vector<std::size_t>> bags_; // game -> remaining shuffled indices
 std::map<std::string,int> plays_;
 std::string lastGame_;
 std::size_t lastTrack_=static_cast<std::size_t>(-1);
 std::mt19937 rng_;
 void refill(const std::string& game);
};

// ---------- Arcade Jukebox ----------
// All nine soundtracks, one page per Music\<game> folder (SF EX3 is jukebox-only).
const std::vector<std::string>& jukeboxGames();
// Every piece from that game's folder, in disc order (path, then start time).
std::vector<Track> jukeboxTracks(const std::vector<Track>& all,const std::string& game);
// Optional Music\track_titles.tsv: "<path relative to Music>\t<title>" per line, so the
// player can name raw rips. Lines starting with # are comments.
std::map<std::string,std::string> loadTrackTitles(const std::filesystem::path& file);
// Jukebox label: the user's title if given, else the song name, else "Track NN" (number
// = position in that game's list) for rips that carry no title.
std::string jukeboxTitle(const Track& t,int number,const std::map<std::string,std::string>& titles);
}
