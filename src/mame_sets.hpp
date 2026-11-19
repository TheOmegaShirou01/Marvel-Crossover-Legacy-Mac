#pragma once
#include "external_roms.hpp"
#include <string>
namespace mcl {
// Collection game id -> the MAME romset that is actually launched.
// Collection ids (achievements, stats, saves, cards) stay fixed; only the MAME
// set changes. Chosen from real -verifyroms results on the user's archives
// (2026-11-19), re-verified and boot-tested on MAME 0.289: the USA releases, or the
// decrypted "Phoenix" builds where the user's archive has no CPS-2 security key.
// Not yet runnable with the user's files: mvsc2 (mpr-23084 chip not recognised by
// MAME, mie/jvs13551 device ROMs missing) and sfexp/sfex2p (small chips missing).
struct MameSet {
 const char* game;   // collection id
 const char* set;    // MAME romset to launch / audit
 const char* parent; // parent set required by a clone ("" if none)
 const char* bios;   // hardware BIOS set ("" if none)
};
inline constexpr MameSet kMameSets[]={
 {"xmcota","xmcotau","xmcota",""},
 {"msh","mshud","msh",""},
 {"xmvsf","xmvsfur2","xmvsf",""},
 {"mshvsf","mshvsfu1d","mshvsf",""},
 {"mvsc","mvscu","mvsc",""},
 {"mvsc2","mvsc2","","naomi"},
 {"sfexp","sfexp","","coh1000c"},
 {"sfex2p","sfex2p","","coh3002c"},
};
inline const MameSet* findMameSet(const std::string& game){
 for(const auto& s:kMameSets)if(game==s.game)return &s;
 return nullptr;
}
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
