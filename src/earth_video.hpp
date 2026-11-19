#pragma once
#ifdef _WIN32
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <thread>
#include <vector>
namespace mcl {
// Looping background video decoded on a worker thread with Media Foundation.
// Frames are scaled by MF's video processor (e.g. 4K -> 1280x720) and delivered as
// 32-bit BGRX pixels, paced to the video's own timestamps. If anything fails the
// caller simply never receives a frame and draws its fallback background.
class EarthVideo {
public:
 EarthVideo()=default;
 EarthVideo(const EarthVideo&)=delete;
 EarthVideo& operator=(const EarthVideo&)=delete;
 ~EarthVideo(){stop();}
 void start(const std::filesystem::path& file,std::uint32_t width,std::uint32_t height);
 void stop();
 // Copies the newest frame if it is newer than `serial`. Returns true when copied.
 bool latest(std::vector<std::uint32_t>& out,std::uint32_t& w,std::uint32_t& h,std::uint64_t& serial);
 bool failed() const {return failed_;}
 bool hardware() const {return hardware_;}
 // Pause decoding entirely (e.g. while a game or another window is in front).
 void setPaused(bool paused){paused_=paused;}
private:
 void run(std::filesystem::path file);
 std::thread worker_;
 std::atomic<bool> quit_{false},failed_{false},paused_{false},hardware_{false};
 std::mutex m_;
 std::vector<std::uint32_t> frame_;
 std::uint32_t w_=0,h_=0,reqW_=1280,reqH_=720;
 std::uint64_t serial_=0;
};
}
#endif
