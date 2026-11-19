#pragma once
#include "external_roms.hpp"
#include <string>
namespace mcl {
// Collection game id -> the MAME romset that is actually launched.
// Collection ids (achievements, stats, saves, cards) stay fixed; only the MAME
// set changes. Chosen from real -verifyroms results on the user's archives
// (2026-11-19), re-verified and boot-tested on MAME 0.289: the USA releases, or the
// decrypted "Phoenix" builds where the user's archive has no CPS-2 security key.
// MvC2 runs in Flycast (NAOMI specialist): it needs only naomi.zip beside the game
// zip, not MAME's mie/jvs13551 device ROMs. The user's mvsc2.zip has a corrupt
// mpr-23084 chip; tools/build_flycast_set.py builds a repaired copy once a good chip
// is supplied. sfexp/sfex2p still miss small chips in the user's archives.
enum class Emulator { Mame, Flycast };
struct MameSet {
 const char* game;   // collection id
 const char* set;    // romset to launch / audit
 const char* parent; // parent set required by a clone ("" if none)
 const char* bios;   // hardware BIOS set ("" if none)
 Emulator emulator=Emulator::Mame;
};
inline constexpr MameSet kMameSets[]={
 {"xmcota","xmcotau","xmcota",""},
 {"msh","mshud","msh",""},
 {"xmvsf","xmvsfur2","xmvsf",""},
 {"mshvsf","mshvsfu1d","mshvsf",""},
 {"mvsc","mvscu","mvsc",""},
 {"mvsc2","mvsc2","","naomi",Emulator::Flycast},
 {"sfexp","sfexp","","coh1000c"},
 {"sfex2p","sfex2p","","coh3002c"},
};
inline const MameSet* findMameSet(const std::string& game){
 for(const auto& s:kMameSets)if(game==s.game)return &s;
 return nullptr;
}
inline bool usesFlycast(const std::string& g){const auto* s=findMameSet(g);return s&&s->emulator==Emulator::Flycast;}
inline bool isCps2Game(const std::string& g){return g=="xmcota"||g=="msh"||g=="xmvsf"||g=="mshvsf"||g=="mvsc";}
// Presence only (zip file or folder). MAME -verifyroms remains the real check.
// Returns an empty string when the set, its parent and its BIOS are all present.
inline std::string missingRomFor(const RomSearchPath& roms,const std::string& game){
 const auto* s=findMameSet(game);
 if(!s)return "Unknown game";
 if(!roms.hasSet(s->set))return std::string("ROM set missing: ")+s->set+".zip";
 if(*s->parent&&!roms.hasSet(s->parent))return std::string("Parent ROM set missing: ")+s->parent+".zip";
 if(*s->bios&&!roms.hasSet(s->bios))return std::string("BIOS missing: ")+s->bios+".zip";
 return {};
}
}
