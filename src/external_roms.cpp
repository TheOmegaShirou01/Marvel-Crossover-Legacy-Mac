#include "external_roms.hpp"
#include <array>
#include <fstream>
#include <stdexcept>
#include <system_error>
namespace mcl {
namespace fs=std::filesystem;
bool RomSearchPath::anyDirectoryExists() const {
 std::error_code ec;
 for(const auto& d:dirs)if(fs::is_directory(d,ec))return true;
 return false;
}
bool RomSearchPath::hasSet(const std::string& set) const {
 if(set.empty())return false;
 std::error_code ec;
 for(const auto& d:dirs){
  if(fs::is_regular_file(d/(set+".zip"),ec)||fs::is_directory(d/set,ec))return true;
 }
 return false;
}
fs::path RomSearchPath::findFile(const std::string& name) const {
 std::error_code ec;
 for(const auto& d:dirs){auto p=d/name;if(fs::is_regular_file(p,ec))return p;}
 return {};
}
std::wstring RomSearchPath::mameRompath() const {
 std::wstring out;
 for(const auto& d:dirs){
  if(!out.empty())out+=L';';
  out+=fs::absolute(d).lexically_normal().wstring();
 }
 return out;
}
bool RomReadiness::readyForFileLevelCheck() const {
 if(!configured||files.empty()) return false;
 for(const auto& entry:files)if(!entry.present||!entry.looksLikeZip)return false;
 return true;
}
static fs::path parseLine(std::string raw,const fs::path& configFile){
 if(!raw.empty()&&raw.back()=='\r')raw.pop_back();
 const auto first=raw.find_first_not_of(" \t");
 if(first==std::string::npos)return {};
 raw.erase(0,first);
 while(!raw.empty()&&(raw.back()==' '||raw.back()=='\t'))raw.pop_back();
 if(raw.empty()||raw[0]=='#'||raw.size()>4096||raw.find('\0')!=std::string::npos)return {};
 // UTF-8 is supported on Windows by std::filesystem::u8path.
 fs::path folder=fs::u8path(raw);
 if(folder.is_relative())folder=configFile.parent_path()/folder;
 return folder.lexically_normal();
}
RomSearchPath readRomSearchPath(const fs::path& configFile){
 RomSearchPath result;
 std::ifstream in(configFile,std::ios::binary);
 if(!in)return result;
 std::string raw;
 while(result.dirs.size()<16&&std::getline(in,raw)){
  auto p=parseLine(raw,configFile);
  if(!p.empty())result.dirs.push_back(p);
 }
 return result;
}
fs::path readRomDirectory(const fs::path& configFile) {
 auto all=readRomSearchPath(configFile);
 return all.empty()?fs::path{}:all.dirs.front();
}
RomSearchPath resolveRuntimeRomSearchPath(const fs::path& applicationDirectory){
 auto explicitPaths=readRomSearchPath(applicationDirectory/"rom_directory.txt");
 if(!explicitPaths.empty())return explicitPaths;
 std::error_code ec;auto local=applicationDirectory/"Runtime"/"ROMs";
 if(fs::is_directory(local,ec))return RomSearchPath(local);
 return {};
}
fs::path resolveRuntimeRomDirectory(const fs::path& applicationDirectory){
 auto all=resolveRuntimeRomSearchPath(applicationDirectory);
 return all.empty()?fs::path{}:all.dirs.front();
}
RomReadiness checkExternalRomFiles(const RomSearchPath& search,const std::string& gameId){
 RomReadiness result; result.directory=search.empty()?fs::path{}:search.dirs.front();
 result.configured=search.anyDirectoryExists();
 // Minimal file-level dependencies: NOT proof that ROM revision, security keys,
 // program chips, sample data or emulator BIOS compatibility are correct.
 std::vector<std::string> required;
 if(gameId=="sfexp")required={"sfexp.zip","coh1000c.zip"};
 else if(gameId=="sfex2p")required={"sfex2p.zip","coh3002c.zip"};
 else throw std::invalid_argument("Unknown CHEAT EX game");
 for(const auto& name:required){
  RomCheck check;check.filename=name;
  if(result.configured){
   auto file=search.findFile(name);
   check.present=!file.empty();
   if(check.present){
    std::ifstream data(file,std::ios::binary);
    std::array<unsigned char,4> sig{};
    data.read(reinterpret_cast<char*>(sig.data()),4);
    check.looksLikeZip=(data.gcount()==4&&sig[0]=='P'&&sig[1]=='K'&&
      ((sig[2]==3&&sig[3]==4)||(sig[2]==5&&sig[3]==6)||(sig[2]==7&&sig[3]==8)));
   }
  }
  result.files.push_back(check);
 }
 return result;
}
}
