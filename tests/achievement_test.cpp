#include "../src/core.hpp"
#include <cassert>
#include <filesystem>
#include <iostream>
int main(){
 namespace fs=std::filesystem;auto root=fs::temp_directory_path()/"mcl_achievements_031";fs::remove_all(root);
 {mcl::Collection c(root);assert(c.achievements().size()==108);
  mcl::VerifiedEvent e;e.game="mvsc";e.type="match_win";e.arcadeMode=true;e.humanControlled=true;
  assert(c.processVerifiedEvent(e));assert(c.progress("mvsc_01").unlocked);
  assert(c.progress("collection_03").unlocked);assert(c.consumeAchievementNotices().size()==2);
  assert(c.consumeAchievementNotices().empty());
  e.type="arcade_clear";e.cheatsEnabled=true;assert(c.processVerifiedEvent(e));
  assert(!c.progress("mvsc_12").unlocked);assert(c.galleryUnlocked());
  assert(c.consumeGalleryUnlockNotice());assert(!c.consumeGalleryUnlockNotice());
  e.cheatsEnabled=false;e.continued=true;assert(c.processVerifiedEvent(e));assert(!c.progress("mvsc_12").unlocked);
  e.continued=false;assert(c.processVerifiedEvent(e));assert(c.progress("mvsc_12").unlocked);
  mcl::VerifiedEvent bogus=e;bogus.game="no_game";assert(!c.processVerifiedEvent(bogus));
  bogus=e;bogus.type="unknown";assert(!c.processVerifiedEvent(bogus));
  bogus=e;bogus.type="match_win";bogus.humanControlled=false;assert(!c.processVerifiedEvent(bogus));
  bogus=e;bogus.count=5;assert(!c.processVerifiedEvent(bogus));
  mcl::VerifiedEvent ex;ex.game="sfexp";ex.type="match_win";ex.arcadeMode=true;ex.humanControlled=true;
  assert(c.processVerifiedEvent(ex));assert(c.progress("sfexp_01").unlocked);
  ex.game="sfex2p";assert(c.processVerifiedEvent(ex));assert(c.progress("sfex2p_01").unlocked);
  ex.type="arcade_clear";ex.cheatsEnabled=false;assert(c.processVerifiedEvent(ex));assert(c.progress("sfex2p_12").unlocked);
 }
 {mcl::Collection c(root);
  // New mastery events: need real play, refused with cheats.
  mcl::VerifiedEvent p;p.game="xmcota";p.type="perfect_round";p.arcadeMode=true;p.humanControlled=true;
  p.cheatsEnabled=true;assert(!c.processVerifiedEvent(p));assert(!c.progress("xmcota_04").unlocked);
  p.cheatsEnabled=false;p.humanControlled=false;assert(!c.processVerifiedEvent(p));
  p.humanControlled=true;assert(c.processVerifiedEvent(p));assert(c.progress("xmcota_04").unlocked);
  p.type="perfect_match";p.game="mvsc";assert(c.processVerifiedEvent(p));assert(c.progress("mvsc_06").unlocked);
  p.type="win_streak";p.game="sfexp";assert(c.processVerifiedEvent(p));assert(c.progress("sfexp_06").unlocked);
  p.type="boss_defeated";p.game="mvsc";p.detail="thanos";assert(!c.processVerifiedEvent(p)); // wrong game
  p.detail="onslaught";p.cheatsEnabled=true;assert(!c.processVerifiedEvent(p));assert(!c.progress("mvsc_10").unlocked);
  p.cheatsEnabled=false;assert(c.processVerifiedEvent(p));assert(c.progress("mvsc_10").unlocked);
  p.game="sfex2p";p.detail="bison2";assert(c.processVerifiedEvent(p));assert(c.progress("sfex2p_11").unlocked);
  mcl::VerifiedEvent m;m.type="master_code";assert(c.processVerifiedEvent(m));assert(c.progress("collection_05").unlocked);
 }
 {mcl::Collection c(root);assert(c.achievements().size()==108);assert(c.progress("sfexp_01").unlocked);assert(c.progress("xmcota_04").unlocked);
  assert(c.progress("mvsc_12").unlocked);assert(c.galleryUnlocked());}
 fs::remove_all(root);std::cout<<"PASS: 108 achievements, verified events, guards, unlocks, notifications, persistence\\n";
}
