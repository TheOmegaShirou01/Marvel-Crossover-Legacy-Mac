#pragma once
#include <array>
#include <filesystem>
#include <string>
namespace mcl {
// Configuration ONLY. Emulators must not treat an enabled flag as an applied patch.
enum class ExCheat : std::size_t { Health=0, Power=1, Time=2, SecretCharacters=3 };
struct ExCheatOptions {std::array<bool,4> flags{}; bool get(ExCheat c) const {return flags.at(static_cast<std::size_t>(c));} void set(ExCheat c,bool on){flags.at(static_cast<std::size_t>(c))=on;}};
class ExCheatSettings {
 std::filesystem::path file_;
 std::array<ExCheatOptions,2> games_{};
public:
 explicit ExCheatSettings(std::filesystem::path file):file_(std::move(file)){}
 bool load(); // safe defaults on missing/invalid data
 bool save() const;
 ExCheatOptions& options(const std::string& id);
 const ExCheatOptions& options(const std::string& id) const;
 static bool validGame(const std::string& id){return id=="sfexp"||id=="sfex2p";}
};
}
