#pragma once
#include <filesystem>
#include <map>
#include <string>
#include <vector>
namespace mcl {
// CONTROLS SETUP (OPTIONS): the player's own keys and controller buttons.
//
// Game controls are written as MAME input sequences ("KEYCODE_LCONTROL OR JOYCODE_1_BUTTON1")
// into MAME\ctrlr\mcl_controls.cfg, which every MAME launch loads with -ctrlr (MAME's
// controller-file mechanism), so the player's own MAME settings are never modified.
// Controllers are addressed through DirectInput (-joystickprovider dinput), which covers
// Xbox, PlayStation 3/4/5, Switch Pro and generic USB/Bluetooth pads alike.
//
// Menu buttons (confirm/back/select/start) are DirectInput button numbers for non-Xbox
// pads; -1 = automatic layout for the detected controller.
struct GameInput { const char* port; const char* label; const char* defaultKey; const char* defaultPad; };
const std::vector<GameInput>& gameInputs(); // P1 directions, 6 attack buttons, start, coin
class ControlsConfig {
public:
 struct Binding { std::string key,pad; };
 ControlsConfig();
 Binding& binding(const std::string& port);
 const Binding& binding(const std::string& port) const;
 void resetDefaults();
 bool isDefault() const;
 int menuConfirm=-1,menuBack=-1,menuSelect=-1,menuStart=-1;
 bool load(const std::filesystem::path& file);
 bool save(const std::filesystem::path& file) const;
 // MAME controller file (<mameconfig> with one <port> per game input).
 std::string mameCtrlrXml() const;
private:
 std::map<std::string,Binding> b_;
};
// MAME keycode for a Windows virtual-key (VK_*) value, "" if unsupported.
std::string mameKeyName(int vk);
// Short label for showing a MAME code: "KEYCODE_LCONTROL" -> "L-CTRL", "JOYCODE_1_BUTTON3" -> "PAD 1 BTN 3".
std::string inputLabel(const std::string& code);
}
