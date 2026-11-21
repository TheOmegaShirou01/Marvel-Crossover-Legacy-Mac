#pragma once
#ifdef _WIN32
#include <windows.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
namespace mcl {
// Every game controller Windows exposes through DirectInput: PlayStation 3/4/5
// (DualShock/DualSense), Switch Pro, generic USB/Bluetooth pads, arcade sticks, and Wii
// remotes when a driver/adapter presents them as a game controller. Xbox-style XInput
// pads are listed too (isXInput) so the menus can skip them and use XInput instead.
// Devices are listed in DirectInput enumeration order, which is also the order MAME's
// "dinput" joystick provider uses, so pad N here is JOYCODE_N in MAME.
struct PadSnapshot {
 bool up=false,down=false,left=false,right=false; // D-pad or left stick
 std::uint64_t buttons=0;                           // bit k = DirectInput button k
 long lx=0,ly=0,lz=0,lrz=0;                         // axes, -1000..1000
 DWORD pov=0xFFFFFFFF;
};
struct PadInfo { std::wstring name; unsigned vid=0,pid=0; bool isXInput=false; };
class Gamepads {
public:
 Gamepads();
 ~Gamepads();
 Gamepads(const Gamepads&)=delete;
 Gamepads& operator=(const Gamepads&)=delete;
 bool init(HWND window);
 void rescan();                         // pick up controllers plugged in later
 int count() const;
 const PadInfo& info(int i) const;
 bool poll(int i,PadSnapshot& out);
 // First control newly pressed on any pad since the last call, as a MAME code
 // ("JOYCODE_2_BUTTON3", "JOYCODE_1_HAT1UP", "JOYCODE_1_ZAXIS_POS_SWITCH"); "" if none.
 std::string capturePress();
 void resetCapture();
private:
 struct Impl;
 std::unique_ptr<Impl> p_;
};
// Automatic menu layout for a controller family (DirectInput button numbers).
struct MenuButtons { int confirm,back,select,start; };
MenuButtons defaultMenuButtons(unsigned vid,unsigned pid);
}
#endif
