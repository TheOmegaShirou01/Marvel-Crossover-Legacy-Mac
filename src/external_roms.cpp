#include "external_roms.hpp"
#include <array>
#include <fstream>
#include <stdexcept>
#include <system_error>
namespace mcl {
namespace fs=std::filesystem;
bool RomReadiness::readyForFileLevelCheck() const {
 if(!configured||files.empty()) return false;
 for(const auto& entry:files)if(!entry.present||!entry.looksLikeZip)return false;
 return true;
}
fs::path readRomDirectory(const fs::path& configFile) {
 std::ifstream in(configFile,std::ios::binary);
 if(!in)return {};
 std::string raw;
 if(!std::getline(in,raw))return {};
 if(!raw.empty()&&raw.back()=='\r')raw.pop_back();
 if(raw.empty()||raw.size()>4096||raw.find('\0')!=std::string::npos)return {};
 // UTF-8 is supported on Windows by std::filesystem::u8path.
 fs::path folder=fs::u8path(raw);
 if(folder.is_relative())folder=configFile.parent_path()/folder;
 return folder.lexically_normal();
}
fs::path resolveRuntimeRomDirectory(const fs::path& applicationDirectory){
 const auto explicitPath=readRomDirectory(applicationDirectory/"rom_directory.txt");
 if(!explicitPath.empty())return explicitPath;
 std::error_code ec;auto local=applicationDirectory/"Runtime"/"ROMs";
 if(fs::is_directory(local,ec))return local;
 return {};
}
RomReadiness checkExternalRomFiles(const fs::path& directory,const std::string& gameId){
 RomReadiness result; result.directory=directory;
 std::error_code ec;
 result.configured=!directory.empty()&&fs::is_directory(directory,ec);
 // Minimal file-level dependencies: NOT proof that ROM revision, security keys,
 // program chips, sample data or emulator BIOS compatibility are correct.
 std::vector<std::string> required;
 if(gameId=="sfexp")required={"sfexp.zip","coh1000c.zip"};
 else if(gameId=="sfex2p")required={"sfex2p.zip","coh3002c.zip"};
 else throw std::invalid_argument("Unknown CHEAT EX game");
 for(const auto& name:required){
  RomCheck check;check.filename=name;
  if(result.configured){
   auto file=directory/name;
   std::error_code fileEc;
   check.present=fs::is_regular_file(file,fileEc);
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
