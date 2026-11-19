#pragma once
#include "core.hpp"
#include "external_roms.hpp"
#include <string>
#include <filesystem>
namespace mcl {
// This is a contract for future trusted emulator backends, not an emulator.
// A renderer/UI must not fabricate VerifiedEvent calls.
enum class RuntimeState { Idle, Prepared, Running, Paused };
struct RuntimeResult {bool ok=false;std::string message;};
class EmulatorBackend {
public:
 virtual ~EmulatorBackend()=default;
 virtual bool available(const std::string& gameId) const=0;
 virtual bool start(const std::string& gameId,const std::filesystem::path& romDirectory)=0;
 virtual bool stop()=0;
 virtual bool pause(bool paused)=0;
};
class EmulatorBridge {
 Collection& model_;
 EmulatorBackend& backend_;
 std::filesystem::path romDirectory_;
 std::string game_;
 RuntimeState state_=RuntimeState::Idle;
 bool cheats_=false;
 bool continued_=false;
public:
 EmulatorBridge(Collection& model,EmulatorBackend& backend,std::filesystem::path romDirectory)
  :model_(model),backend_(backend),romDirectory_(std::move(romDirectory)){}
 RuntimeState state() const {return state_;}
 const std::string& game()const{return game_;}
 RuntimeResult prepare(const std::string& gameId);
 RuntimeResult start();
 RuntimeResult pause();
 RuntimeResult resume();
 RuntimeResult stop();
 // Only the trusted backend integration should call these hooks.
 bool onVerifiedGameplayEvent(VerifiedEvent event);
 bool onCheatApplicationStatus(bool actualCheatActive);
 void onContinueUsed();
};
}
