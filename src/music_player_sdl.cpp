// SDL3_mixer build of music_player.hpp (Windows + macOS).
#include "music_player.hpp"
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <algorithm>
#include <map>
namespace mcl {
namespace {
MIX_Mixer* gMixer=nullptr;
struct Sound { MIX_Audio* audio=nullptr; MIX_Track* track=nullptr; };
std::map<std::string,Sound> gSounds;
void freeSound(Sound& s){
 if(s.track){MIX_StopTrack(s.track,0);MIX_DestroyTrack(s.track);s.track=nullptr;}
 if(s.audio){MIX_DestroyAudio(s.audio);s.audio=nullptr;}
}
}
bool audioInit(){
 if(gMixer)return true;
 if(!SDL_WasInit(SDL_INIT_AUDIO)&&!SDL_InitSubSystem(SDL_INIT_AUDIO))return false;
 if(!MIX_Init())return false;
 gMixer=MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,nullptr);
 if(gMixer&&SDL_getenv("MCL_MUTE"))MIX_SetMixerGain(gMixer,0.f); // automated tests: silent
 return gMixer!=nullptr;
}
void audioShutdown(){
 for(auto& s:gSounds)freeSound(s.second);
 gSounds.clear();
 if(gMixer){MIX_DestroyMixer(gMixer);gMixer=nullptr;}
 MIX_Quit();
}
bool playSound(const std::string& slot,const std::filesystem::path& file,float gain){
 if(!audioInit())return false;
 Sound& s=gSounds[slot];
 freeSound(s);
 std::error_code ec;
 if(gain<=0.f||!std::filesystem::is_regular_file(file,ec))return false;
 s.audio=MIX_LoadAudio(gMixer,file.u8string().c_str(),true);
 if(!s.audio)return false;
 s.track=MIX_CreateTrack(gMixer);
 if(!s.track){freeSound(s);return false;}
 MIX_SetTrackAudio(s.track,s.audio);
 MIX_SetTrackGain(s.track,std::clamp(gain,0.f,1.f));
 return MIX_PlayTrack(s.track,0);
}
void stopSound(const std::string& slot){
 auto it=gSounds.find(slot);
 if(it!=gSounds.end())freeSound(it->second);
}

bool MusicPlayer::play(const std::filesystem::path& file,int startMs,int endMs,int volume){
 stop();
 std::error_code ec;
 if(!std::filesystem::is_regular_file(file,ec)||!audioInit())return false;
 // Streamed (not pre-decoded): pieces are long and only one plays at a time.
 MIX_Audio* a=MIX_LoadAudio(gMixer,file.u8string().c_str(),false);
 if(!a)return false;
 MIX_Track* t=MIX_CreateTrack(gMixer);
 if(!t){MIX_DestroyAudio(a);return false;}
 MIX_SetTrackAudio(t,a);
 MIX_SetTrackGain(t,static_cast<float>(std::clamp(volume,0,1000))/1000.f);
 const SDL_PropertiesID props=SDL_CreateProperties();
 SDL_SetNumberProperty(props,MIX_PROP_PLAY_START_MILLISECOND_NUMBER,std::max(0,startMs));
 const bool ok=MIX_PlayTrack(t,props);
 SDL_DestroyProperties(props);
 if(!ok){MIX_DestroyTrack(t);MIX_DestroyAudio(a);return false;}
 track_=t;audio_=a;endMs_=endMs;open_=true;
 return true;
}
void MusicPlayer::stop(){
 if(!open_)return;
 auto* t=static_cast<MIX_Track*>(track_);auto* a=static_cast<MIX_Audio*>(audio_);
 if(t){MIX_StopTrack(t,0);MIX_DestroyTrack(t);}
 if(a)MIX_DestroyAudio(a);
 track_=audio_=nullptr;open_=false;
}
void MusicPlayer::setVolume(int v){
 if(open_)MIX_SetTrackGain(static_cast<MIX_Track*>(track_),static_cast<float>(std::clamp(v,0,1000))/1000.f);
}
int MusicPlayer::positionMs() const{
 if(!open_)return 0;
 auto* t=static_cast<MIX_Track*>(track_);
 const Sint64 f=MIX_GetTrackPlaybackPosition(t);
 return f<0?0:static_cast<int>(MIX_TrackFramesToMS(t,f));
}
bool MusicPlayer::playing() const{
 if(!open_)return false;
 // The piece ends at endMs (catalog.tsv trims silence), like MCI's "play from .. to ..".
 return MIX_TrackPlaying(static_cast<MIX_Track*>(track_))&&positionMs()<endMs_;
}
}
