#pragma once
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
namespace mcl_pause_ui {
// Prototype ONLY. No game is paused or stopped by this UI.
// Returns true if Exit Game > Yes was chosen; false for Continue, No, or close.
bool showPreview(HWND owner);
}
#endif
