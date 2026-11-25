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
#include <functional>
#include <mutex>
#include <string>
#include <thread>
namespace mcl {
// The version of this build. Publish GitHub releases with tags like "v0.4.3" so the
// updater can compare them; a release with a non-numeric tag (e.g. "BETA") counts as new
// only when it was published after this exe was built and has not been installed yet.
inline constexpr const char* kAppVersion="0.4.5";
inline constexpr const wchar_t* kUpdateHost=L"api.github.com";
#ifdef _WIN32
inline constexpr const wchar_t* kUpdatePath=L"/repos/TheOmegaShirou01/Marvel-Crossover-Legacy/releases/latest";
#else
// Mac version: its own repository (never the Windows one).
inline constexpr const wchar_t* kUpdatePath=L"/repos/TheOmegaShirou01/Marvel-Crossover-Legacy-Mac/releases/latest";
#endif

struct ReleaseInfo {
 long long id=0;
 std::string tag,name,publishedAt,assetName,assetUrl;
 std::string body; // release notes (Markdown)
 unsigned long long assetSize=0;
};
// GitHub "latest release" JSON -> release + the archive to download (.zip, .rar or .7z; one whose
// name has "update" in it is preferred, otherwise the largest). False if there is none.
bool parseLatestRelease(const std::string& json,ReleaseInfo& out);
// Release notes as plain text for the update pop-up (Markdown/HTML removed, at most maxLines).
std::string releaseNotesText(const std::string& markdown,size_t maxLines=14);
// Numbers in a tag/version ("v0.4.3" -> 0,4,3). False when the text has no digits.
bool versionNumbers(const std::string& text,int out[4]);
// -1 / 0 / 1 like strcmp, on the numbers of both versions.
int compareVersions(const std::string& a,const std::string& b);
// "2026-11-21T18:25:43Z" -> seconds since 1970 (UTC); 0 if it can't be read.
long long parseIsoUtc(const std::string& iso);
// Is this release an update for us?
bool releaseIsNewer(const ReleaseInfo& r,const std::string& appVersion,long long buildUtc,long long installedId);
// Files an update leaves alone (path relative to the game folder): the player's progress,
// settings, controls and backups (MCL_UserData), logs, ROM/emulator path files, MAME's
// per-game settings, saves and high scores, Flycast's settings, saves and mappings.
bool keptOnUpdate(const std::filesystem::path& relative);

#ifdef _WIN32
// Installs an unpacked update (`source` = the folder holding the new exe) into the game folder
// `app`, from inside the running game. Each file that will be replaced is first moved to
// app/Update/backup (a running exe can be renamed, just not overwritten), then the new file is
// moved in. Any failure puts every file back as it was. On success it records the release
// (MCL_UserData/installed_release.txt) and leaves "ok" in MCL_UserData/update_result.txt so the
// restarted game can say so. `progress` gets 0..1.
bool installUpdate(const std::filesystem::path& source,const std::filesystem::path& app,long long releaseId,
 const std::string& version,std::wstring& error,const std::function<void(double)>& progress={});
// After the restart: the result installUpdate left (true once), and removal of app/Update.
struct UpdateResult { bool ok=false; std::string version; };
bool takeUpdateResult(const std::filesystem::path& app,UpdateResult& out);
void cleanupUpdateFolder(const std::filesystem::path& app); // retries for a while: the old exe may still be closing
// Only the game and README.txt stay visible: everything else at the top of the game folder
// (including folders a future update adds) gets Hidden + System. Run at every start and after an update.
void hideGameFolder(const std::filesystem::path& app);

class Updater {
public:
 enum class State { Idle, Checking, UpToDate, Available, Failed, Downloading, Extracting, Ready, Installing, Installed };
 explicit Updater(std::filesystem::path appDir):dir_(std::move(appDir)){}
 ~Updater();
 Updater(const Updater&)=delete;
 Updater& operator=(const Updater&)=delete;
 void startCheck();                 // background; see state()
 void startDownload();              // after Available: download + unpack in the background
 void cancel();                     // stop a download
 void startInstall();               // after Ready (menu music/videos stopped): install in the background
 bool restartGame();                // after Installed: start the new exe; the caller then quits
 State state() const {return state_;}
 double progress() const {return progress_;}           // 0..1 while downloading / installing
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
#else
// macOS / Linux test builds: no self-update yet (the check reports "up to date").
struct UpdateResult { bool ok=false; std::string version; };
inline bool takeUpdateResult(const std::filesystem::path&,UpdateResult&){return false;}
inline void cleanupUpdateFolder(const std::filesystem::path&){}
inline void hideGameFolder(const std::filesystem::path&){}
class Updater {
public:
 enum class State { Idle, Checking, UpToDate, Available, Failed, Downloading, Extracting, Ready, Installing, Installed };
 explicit Updater(std::filesystem::path){}
 void startCheck(){state_=State::UpToDate;}
 void startDownload(){}
 void cancel(){}
 void startInstall(){}
 bool restartGame(){return false;}
 State state() const {return state_;}
 double progress() const {return 0;}
 ReleaseInfo release() const {return {};}
 std::wstring error() const {return {};}
private:
 State state_=State::Idle;
};
#endif
}
