#pragma once
#ifdef _WIN32
#include <filesystem>
namespace mcl {
// Minimal MP3 player on Windows MCI ("mpegvideo" device). Plays one stretch of a file
// (from startMs to endMs) with a 0..1000 volume for fades. Call from one thread only.
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
};
}
#endif
