// Background / preview video where no decoder is wired in yet (macOS first test builds):
// the video reports failure at once, so the menus draw their gradient background and the
// Arcade Select cards stay on their logos. earth_video.cpp is the Media Foundation version.
#ifndef _WIN32
#include "earth_video.hpp"
namespace mcl {
void EarthVideo::start(const std::filesystem::path&,std::uint32_t,std::uint32_t){failed_=true;}
void EarthVideo::stop(){}
bool EarthVideo::latest(std::vector<std::uint32_t>&,std::uint32_t&,std::uint32_t&,std::uint64_t&){return false;}
void EarthVideo::run(std::filesystem::path){}
}
#endif
