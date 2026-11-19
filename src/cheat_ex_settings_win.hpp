#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <filesystem>
namespace mcl_exui {void showSettings(HWND owner,const std::filesystem::path& assetRoot,const char* gameId);}
