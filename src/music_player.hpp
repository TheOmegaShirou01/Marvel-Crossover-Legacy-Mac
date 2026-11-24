#pragma once
#include <filesystem>
#include <string>
namespace mcl {
// Minimal MP3 player. Plays one stretch of a file (from startMs to endMs) with a 0..1000
// volume for fades. Call from one thread only.
// Two builds: music_player.cpp (Windows MCI "mpegvideo") and music_player_sdl.cpp (SDL3_mixer,
// Windows + macOS).
class MusicPlayer {
public:
 MusicPlayer()=default;
 MusicPlayer(const MusicPlayer&)=delete;
 MusicPlayer& operator=(const MusicPlayer&)=delete;
 ~MusicPlayer(){stop();}
 bool play(const std::filesystem::path& file,int startMs,int endMs,int volume);
 void stop();
 void setVolume(int volume);
 int positionMs() const;
 bool playing() const;
private:
 bool open_=false;
 void* track_=nullptr; // SDL build: MIX_Track*
 void* audio_=nullptr; // SDL build: MIX_Audio*
 int endMs_=0;
};
// Short sounds (title voice, achievement unlock). `slot` names a channel: a new sound on the
// same slot replaces the one playing. gain 0..1. SDL build only; the MCI build plays them itself.
bool audioInit();
void audioShutdown();
bool playSound(const std::string& slot,const std::filesystem::path& file,float gain);
void stopSound(const std::string& slot);
}
