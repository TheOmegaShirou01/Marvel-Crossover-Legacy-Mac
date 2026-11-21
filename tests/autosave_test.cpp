#include "autosave.hpp"
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
namespace fs=std::filesystem;
static void put(const fs::path& p,const std::string& s){fs::create_directories(p.parent_path());std::ofstream(p,std::ios::binary)<<s;}
static std::string get(const fs::path& p){std::ifstream in(p,std::ios::binary);return std::string(std::istreambuf_iterator<char>(in),{});}
int main(){
 const fs::path dir=fs::temp_directory_path()/"mcl_autosave_test";
 fs::remove_all(dir);
 const std::string good="MCL-STATE-1\nA\tcollection_01\t2026-11-20T18:30:47Z\n";
 put(dir/"progress.tsv",good);put(dir/"settings.txt","music_volume=7\n");
 // backups rotate, newest kept
 for(int i=0;i<12;++i){char s[32];std::snprintf(s,sizeof s,"20261121-1200%02d",i);assert(mcl::backupUserData(dir,s,10)==2);}
 int n=0;for(auto& e:fs::directory_iterator(dir/"Backups")){(void)e;++n;}
 assert(n==10&&!fs::exists(dir/"Backups"/"20261121-120000")&&fs::exists(dir/"Backups"/"20261121-120011"/"progress.tsv"));
 // a good save is left alone
 assert(!mcl::restoreIfNeeded(dir));
 // damaged save: not backed up, and repaired from the newest good copy
 put(dir/"progress.tsv","garbage");
 assert(mcl::backupUserData(dir,"20261121-130000",10)==0);
 assert(mcl::restoreIfNeeded(dir)&&get(dir/"progress.tsv")==good&&fs::exists(dir/"progress.damaged.tsv"));
 // missing save is restored too
 fs::remove(dir/"progress.tsv");
 assert(mcl::restoreIfNeeded(dir)&&mcl::progressLooksValid(get(dir/"progress.tsv")));
 assert(mcl::backupStamp().size()==15);
 // hiscore alias: xmvsfur2 joins the xmvsfur1 group once
 const std::string dat="xmvsfu:\nxmvsfur1:\n@:maincpu,program,ff256e,63,00,05\n";
 const std::string p=mcl::hiscoreDatWithAliases(dat);
 assert(p=="xmvsfu:\nxmvsfur2:\nxmvsfur1:\n@:maincpu,program,ff256e,63,00,05\n");
 assert(mcl::hiscoreDatWithAliases(p).empty());
 fs::remove_all(dir);
 std::cout<<"PASS: rotating backups, damaged/missing save restore, hiscore aliases\n";
}
