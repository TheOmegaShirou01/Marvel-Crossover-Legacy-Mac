#pragma once
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
namespace mcl {
// The version of this build. Publish GitHub releases with tags like "v0.4.3" so the
// updater can compare them; a release with a non-numeric tag (e.g. "BETA") counts as new
// only when it was published after this exe was built and has not been installed yet.
inline constexpr const char* kAppVersion="0.4.3";
inline constexpr const wchar_t* kUpdateHost=L"api.github.com";
inline constexpr const wchar_t* kUpdatePath=L"/repos/TheOmegaShirou01/Marvel-Crossover-Legacy/releases/latest";

struct ReleaseInfo {
 long long id=0;
 std::string tag,name,publishedAt,assetName,assetUrl;
 unsigned long long assetSize=0;
};
// GitHub "latest release" JSON -> release + the zip to download (an asset whose name has
// "update" in it is preferred, otherwise the largest .zip). False if no usable zip.
bool parseLatestRelease(const std::string& json,ReleaseInfo& out);
// Numbers in a tag/version ("v0.4.3" -> 0,4,3). False when the text has no digits.
bool versionNumbers(const std::string& text,int out[4]);
// -1 / 0 / 1 like strcmp, on the numbers of both versions.
int compareVersions(const std::string& a,const std::string& b);
// "2026-11-21T18:25:43Z" -> seconds since 1970 (UTC); 0 if it can't be read.
long long parseIsoUtc(const std::string& iso);
// Is this release an update for us?
bool releaseIsNewer(const ReleaseInfo& r,const std::string& appVersion,long long buildUtc,long long installedId);
// Batch file that waits for the game to close, copies the new files over the install
// (keeping the player's progress, settings, ROM paths and emulator saves/high scores),
// records the installed release, cleans up and starts the game again.
std::string installerScript(const std::filesystem::path& source,const std::filesystem::path& app,
 unsigned long pid,long long releaseId,const std::filesystem::path& cleanup);

#ifdef _WIN32
class Updater {
public:
 enum class State { Idle, Checking, UpToDate, Available, Failed, Downloading, Extracting, Ready };
 explicit Updater(std::filesystem::path appDir):dir_(std::move(appDir)){}
 ~Updater();
 Updater(const Updater&)=delete;
 Updater& operator=(const Updater&)=delete;
 void startCheck();                 // background; see state()
 void startDownload();              // after Available: download + unzip in the background
 void cancel();                     // stop a download
 bool launchInstaller();            // after Ready: start the installer; the caller then quits
 State state() const {return state_;}
 double progress() const {return progress_;}           // 0..1 while downloading
 ReleaseInfo release() const {std::lock_guard<std::mutex> l(m_);return rel_;}
 std::wstring error() const {std::lock_guard<std::mutex> l(m_);return error_;}
 long long buildTimeUtc() const;    // from this exe's PE header
private:
 std::filesystem::path dir_,source_;
 std::thread worker_;
 std::atomic<State> state_{State::Idle};
 std::atomic<double> progress_{0};
 std::atomic<bool> cancel_{false};
 mutable std::mutex m_;
 ReleaseInfo rel_;std::wstring error_;
 void join();
 void fail(const std::wstring& why);
};
#endif
}
