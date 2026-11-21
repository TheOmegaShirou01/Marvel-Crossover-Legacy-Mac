#pragma once
#include <filesystem>
#include <string>
namespace mcl {
// Auto-save safety net for bin\MCL_UserData. Settings, trophies and stats are already
// written the moment they change; these helpers keep dated copies and repair a lost save.
//
//   backupUserData   copies the save files into MCL_UserData\Backups\<stamp>\ and keeps
//                    only the newest `keep` backups. Called at startup and after each game.
//   restoreIfNeeded  if progress.tsv is missing, empty or damaged, puts back the newest
//                    good copy from Backups. Called before the save is loaded.
// Saved files: progress.tsv (trophies + stats), settings.txt, marvel_cheats.txt,
// cheat_ex_settings.txt.
int backupUserData(const std::filesystem::path& userDir,const std::string& stamp,int keep=10);
bool restoreIfNeeded(const std::filesystem::path& userDir);
// True when the text is a readable progress file (header line present).
bool progressLooksValid(const std::string& text);
// Local time as "YYYYMMDD-HHMMSS" for backup folder names.
std::string backupStamp();

// MAME high scores: MAME's bundled hiscore plugin reads "hiscore.dat" from MAME's working
// folder before its own copy. This returns the plugin's data with the collection's exact
// ROM sets added where the plugin only lists a sister revision (xmvsfur2 shares
// xmvsfur1's table at ff256e, checked 2026-11-21). Empty if nothing needs adding.
std::string hiscoreDatWithAliases(const std::string& pluginDat);
// Writes <mame folder>\hiscore.dat from plugins\hiscore\hiscore.dat when needed.
bool ensureHiscoreDat(const std::filesystem::path& mameExecutable);
}
