#pragma once
#include "mame_backend.hpp"
#include <string>
namespace mcl {
enum class AuditStatus { Pass, Failed, MissingFiles, TimedOut, CannotStart };
struct AuditResult { AuditStatus status; unsigned long exitCode; std::string message; };
// No emulator execution in this function. Validates a known game and creates a
// shell-free MAME command specification for -verifyroms.
bool createMameAuditSpec(const std::filesystem::path& emulator,
 const std::filesystem::path& romDirectory,const std::string& game,
 MameLaunchSpec& output,std::string& reason);
AuditResult interpretMameAuditExit(unsigned long exitCode);
#ifdef _WIN32
// Runs MAME audit in a separate process; never launches game or edits ROMs.
AuditResult runMameRomAudit(const MameLaunchSpec& spec,unsigned timeoutMs=15000);
#endif
}
