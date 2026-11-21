#pragma once
#include <string>
#include <vector>
#include <set>
#include <map>
#include <filesystem>
#include <functional>

namespace mcl {
namespace fs = std::filesystem;
struct Game { std::string id,title,year,hardware,description; };
struct Achievement {std::string id,game,title,condition,tier;};
struct Character {std::string game,name,notes,role,portraitAsset;};
// Character Encyclopedia entry (gallery package v2): one per character name.
struct CharacterProfile {std::string name,universe,bio,story;};
struct GalleryGame {std::string id,title,year,hardware,developer,history,mechanics,sources;};
struct Media {std::string game,category,title; fs::path path;};
struct Stats {int wins=0, losses=0, credits=0, launches=0, clears=0; long long seconds=0;};
struct AchievementProgress {bool unlocked=false; std::string timestamp;};
// Send only confirmed emulator events here. No user-facing UI calls this API.
// For a win, arcadeMode and humanControlled must be true to count arcade trophies.
struct VerifiedEvent {
    std::string game;
    // coin, launch, match_win, match_loss, arcade_clear, two_player_match, master_code,
    // perfect_round (round won with no damage), perfect_match (match won with no damage),
    // win_streak (fifth consecutive match win in one run)
    std::string type; // boss_defeated: detail = boss id
    bool arcadeMode=false;
    bool humanControlled=false;
    bool cheatsEnabled=false;
    bool continued=false;
    bool allCharactersAlive=false;
    bool allMarvelTeam=false;
    bool allCapcomTeam=false;
    int count=1;
    std::string detail;
    std::vector<std::string> characters; // P1's fighters (match_win / arcade_clear)
};
class Collection {
public:
    explicit Collection(fs::path root);
    const std::vector<Game>& games() const { return games_; }
    const std::vector<Achievement>& achievements() const {return achievements_;}
    const std::vector<Character>& characters() const {return characters_;}
    const std::vector<CharacterProfile>& characterProfiles() const {return profiles_;}
    const CharacterProfile* profile(const std::string& name) const;
    const std::vector<GalleryGame>& galleryGames() const {return galleryGames_;}
    bool galleryUnlocked() const; // first VERIFIED arcade clear, from trusted emulator event only
    bool consumeGalleryUnlockNotice(); // one-shot display request for the host GUI
    bool galleryDataLoaded() const {return galleryDataLoaded_;}
    std::vector<Media> media(const std::string& game,const std::string& type) const;
    Stats stats(const std::string& game) const;
    AchievementProgress progress(const std::string& id) const;
    bool recordEvent(const std::string& game,const std::string& event,int amount=1); // called only by verified emulation hooks
    bool unlockAchievement(const std::string& id); // trusted engine only, not exposed in frontend
    bool processVerifiedEvent(const VerifiedEvent& event); // award supported triggers, persist once
    std::vector<std::string> consumeAchievementNotices(); // host UI popup notifications // called only by validated achievements engine
    bool saveState(const std::string& game,int slot,const std::vector<unsigned char>& bytes); // receives actual emulator-serialized state
    bool loadState(const std::string& game,int slot,std::vector<unsigned char>& bytes) const;
    bool save() const;
    bool load();
    const fs::path& root() const {return root_;}
private:
    fs::path root_;
    std::vector<Game> games_;
    std::vector<Achievement> achievements_;
    std::vector<Character> characters_;
    std::vector<GalleryGame> galleryGames_;
    std::vector<CharacterProfile> profiles_;
    bool galleryDataLoaded_=false;
    bool galleryNotice_=false;
    std::vector<std::string> achievementNotices_;
    std::map<std::string,int> noContinueClears_;
    std::set<std::string> flags_; // saved progress markers ("F" lines): secret wins, cheats used, no-continue clears
    bool setFlag(const std::string& f);
    bool loadGalleryPackage(const fs::path& path);
    std::map<std::string,Stats> stats_;
    std::map<std::string,AchievementProgress> progress_;
};
}
