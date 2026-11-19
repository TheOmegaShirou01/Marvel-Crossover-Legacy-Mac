#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <filesystem>
namespace mcl {class Collection;}
namespace mcl_select {int show(HWND owner,const std::filesystem::path& root,bool cheatExUnlocked,mcl::Collection& model,bool startInEx=false,bool cheatsUnlocked=false);}
