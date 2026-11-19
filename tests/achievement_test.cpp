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
 {mcl::Collection c(root);assert(c.achievements().size()==108);assert(c.progress("sfexp_01").unlocked);
  assert(c.progress("mvsc_12").unlocked);assert(c.galleryUnlocked());}
 fs::remove_all(root);std::cout<<"PASS: 108 achievements, verified events, guards, unlocks, notifications, persistence\\n";
}
