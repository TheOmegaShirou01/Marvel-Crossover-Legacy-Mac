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
 // Mame: MAME games (pad = MAME JOYCODE sequences). Mvc2: Flycast's MvC2 (pad = a named
 // controller button: PAD_A, PAD_B, PAD_X, PAD_Y, PAD_LB, PAD_RB, PAD_LT, PAD_RT, PAD_START...).
 enum class Layout { Mame, Mvc2 };
 explicit ControlsConfig(Layout layout=Layout::Mame);
 Layout layout() const {return layout_;}
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
 Layout layout_=Layout::Mame;
};
// Marvel vs. Capcom 2 runs in Flycast and has its own keyboard layout (OPTIONS > CONTROLS
// SETUP > GAME: MARVEL VS. CAPCOM 2, saved in MCL_UserData\controls_mvc2.txt). Its six
// buttons follow the NAOMI panel: 1 LP, 2 HP, 3 assist 1, 4 LK, 5 HK, 6 assist 2.
const char* mvc2Label(const std::string& port);
// SDL scancode (USB HID usage) for a MAME keycode ("KEYCODE_LCONTROL" -> 224); -1 if unknown.
int sdlScancode(const std::string& mameKey);
// Flycast keyboard mapping for arcade games (mappings\SDL_Keyboard_arcade.cfg): the player's
// MvC2 keys, plus Tab for the pause screen.
std::string flycastKeyboardCfg(const ControlsConfig& c,unsigned takeover=0); // takeover: ACCESSIBILITY buttons (mvc2_onebutton)
// MvC2 default controller button for a game input ("PAD_X"...); directions are always D-pad + left stick.
const char* mvc2PadDefault(const std::string& port);
// Named controller buttons the MvC2 page can assign, in order.
const std::vector<std::string>& mvc2PadButtons();
// Flycast controller mapping for arcade games (mappings\SDL_<name>_arcade.cfg) for one pad,
// from how SDL exposes it: backend 'r' (Raw Input) or 'x' (XInput) for Xbox-style pads,
// 'h' (HIDAPI) for PlayStation / Switch pads. "" for an unknown backend.
std::string flycastPadCfg(const ControlsConfig& c,char backend,unsigned takeover=0);
// Pads Flycast opened, read from its flycast.log: (backend, SDL joystick name).
std::vector<std::pair<char,std::string>> flycastPadsFromLog(const std::string& log);
// MAME keycode for a Windows virtual-key (VK_*) value, "" if unsupported.
std::string mameKeyName(int vk);
// MAME key name for an SDL scancode (the SDL front end's CONTROLS SETUP); "" if MAME has none.
std::string mameKeyFromSdlScancode(int scancode);
// MAME's own per-game file (cfg/<set>.cfg) with every key/button remap removed (ports that
// carry a <newseq>), so CONTROLS SETUP always decides the controls. Everything else is kept.
std::string stripMameInputRemaps(const std::string& xml);
// Short label for showing a MAME code: "KEYCODE_LCONTROL" -> "L-CTRL", "JOYCODE_1_BUTTON3" -> "PAD 1 BTN 3".
std::string inputLabel(const std::string& code);
}
