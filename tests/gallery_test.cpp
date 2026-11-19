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
 if(c.galleryUnlocked())return 5;
 if(!c.recordEvent("mvsc","clear",1))return 6;
 if(!c.galleryUnlocked()||!c.consumeGalleryUnlockNotice()||c.consumeGalleryUnlockNotice())return 7;
 Collection again(root);
 if(!again.galleryUnlocked()||again.consumeGalleryUnlockNotice())return 8;
 if(again.characters().size()!=192)return 9;
 if(!again.recordEvent("mvsc","clear",1)||again.consumeGalleryUnlockNotice())return 10;
 std::cout<<"Gallery package: 8 games, 192 character records; first clear unlocks once and persists. PASS\n";
 std::filesystem::remove_all(root);
 return 0;
}
