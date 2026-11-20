#include "../src/core.hpp"
#include <iostream>
#include <filesystem>
#include <vector>
using namespace mcl;
int main(int argc,char**argv){
 if(argc<2)return 2;
 auto exe=std::filesystem::path(argv[1]);
 auto root=exe/"MCL_UserData";std::filesystem::remove_all(root);
 Collection c(root);
 if(!c.galleryDataLoaded()){std::cerr<<"failed reading gallery package\n";return 3;}
 if(c.galleryGames().size()!=8||c.characters().size()!=192)return 4;
 // v2: every roster character has an encyclopedia entry with a bio and a story.
 if(c.characterProfiles().size()!=95)return 11;
 for(const auto& ch:c.characters()){const auto* p=c.profile(ch.name);if(!p||p->bio.empty()||p->story.empty())return 12;}
 if(c.galleryGames()[0].history.size()<300)return 13;
 if(c.galleryUnlocked())return 5;
 if(!c.recordEvent("mvsc","clear",1))return 6;
 if(!c.galleryUnlocked()||!c.consumeGalleryUnlockNotice()||c.consumeGalleryUnlockNotice())return 7;
 Collection again(root);
 if(!again.galleryUnlocked()||again.consumeGalleryUnlockNotice())return 8;
 if(again.characters().size()!=192)return 9;
 if(!again.recordEvent("mvsc","clear",1)||again.consumeGalleryUnlockNotice())return 10;
 // A first clear of a different game does not announce the Gallery again.
 if(!again.recordEvent("xmcota","clear",1)||again.consumeGalleryUnlockNotice())return 14;
 std::cout<<"Gallery package: 8 games, 192 roster records, 95 encyclopedia entries; first clear unlocks once and persists. PASS\n";
 std::filesystem::remove_all(root);
 return 0;
}
