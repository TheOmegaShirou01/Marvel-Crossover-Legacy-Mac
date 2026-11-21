#include "controls.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
namespace fs=std::filesystem;
int main(int argc,char** argv){
 if(argc==3&&std::string(argv[1])=="--write"){mcl::ControlsConfig m(mcl::ControlsConfig::Layout::Mvc2);std::ofstream(std::string(argv[2])+"/SDL_Xbox 360 Controller_arcade.cfg")<<mcl::flycastPadCfg(m,'r');std::ofstream(std::string(argv[2])+"/SDL_Keyboard_arcade.cfg")<<mcl::flycastKeyboardCfg(m);return 0;}
 mcl::ControlsConfig c;
 assert(c.isDefault()&&mcl::gameInputs().size()==24);
 assert(c.binding("P1_BUTTON1").key=="KEYCODE_LCONTROL");
 c.binding("P1_BUTTON1").key="KEYCODE_A";c.binding("P1_BUTTON6").pad="JOYCODE_2_ZAXIS_POS_SWITCH";c.menuConfirm=1;
 assert(!c.isDefault());
 const auto xml=c.mameCtrlrXml();
 assert(xml.find("<port type=\"P1_BUTTON1\">")!=std::string::npos&&xml.find("<port type=\"P2_BUTTON1\">")!=std::string::npos&&xml.find("KEYCODE_T OR JOYCODE_2_BUTTON6")!=std::string::npos);
 assert(xml.find("KEYCODE_A OR JOYCODE_1_BUTTON1")!=std::string::npos);
 assert(xml.find("KEYCODE_X OR JOYCODE_2_ZAXIS_POS_SWITCH")!=std::string::npos);
 c.binding("P1_BUTTON2").key.clear();c.binding("P1_BUTTON2").pad.clear();
 assert(c.mameCtrlrXml().find("<newseq type=\"standard\">NONE</newseq>")!=std::string::npos);
 const fs::path f=fs::temp_directory_path()/"mcl_controls_test.txt";
 assert(c.save(f));
 mcl::ControlsConfig d;assert(d.load(f));
 assert(d.binding("P1_BUTTON1").key=="KEYCODE_A"&&d.binding("P1_BUTTON6").pad=="JOYCODE_2_ZAXIS_POS_SWITCH"&&d.menuConfirm==1);
 assert(d.binding("P1_BUTTON2").key.empty());
 fs::remove(f);
 d.resetDefaults();assert(d.isDefault());
 assert(mcl::mameKeyName('Q')=="KEYCODE_Q"&&mcl::mameKeyName(0xA2)=="KEYCODE_LCONTROL"&&mcl::mameKeyName(0x61)=="KEYCODE_1_PAD");
 assert(mcl::mameKeyName(0x13).empty());
 assert(mcl::inputLabel("KEYCODE_LCONTROL")=="L-CTRL");
 assert(mcl::inputLabel("JOYCODE_1_BUTTON3")=="PAD 1 BTN 3");
 assert(mcl::inputLabel("JOYCODE_1_YAXIS_UP_SWITCH OR JOYCODE_1_HAT1UP")=="PAD 1 STICK UP");
 // MvC2 (Flycast) layout
 {
  mcl::ControlsConfig m(mcl::ControlsConfig::Layout::Mvc2);
  assert(m.binding("P1_BUTTON1").pad=="PAD_X"&&m.binding("P1_BUTTON6").pad=="PAD_RB"&&m.isDefault());
  assert(mcl::inputLabel("PAD_RB")=="RB"&&mcl::inputLabel("PAD_DPAD")=="D-PAD / STICK");
  assert(mcl::sdlScancode("KEYCODE_LCONTROL")==224&&mcl::sdlScancode("KEYCODE_1")==30&&mcl::sdlScancode("KEYCODE_A")==4&&mcl::sdlScancode("KEYCODE_F12")==69);
  const std::string kb=mcl::flycastKeyboardCfg(m);
  assert(kb.find("43:btn_menu")!=std::string::npos&&kb.find("224:btn_a")!=std::string::npos&&kb.find("30:btn_start")!=std::string::npos);
  assert(kb.find("82:btn_dpad1_up")!=std::string::npos&&kb.find("version = 4")!=std::string::npos);
  assert(kb.find("31:btn_start1")!=std::string::npos&&kb.find("4:btn_a1")!=std::string::npos&&kb.find("21:btn_dpad1_up1")!=std::string::npos); // player 2: 2, A, R
  assert(mcl::mvc2Label("P2_BUTTON3")==std::string("ASSIST 1"));
  // Xbox pad through Raw Input: X(2)=LP, Back(6)=pause, hat = D-pad, RT axis 5
  m.binding("P1_BUTTON6").pad="PAD_RT";
  const std::string r=mcl::flycastPadCfg(m,'r');
  assert(r.find("2:btn_a")!=std::string::npos&&r.find("6:btn_menu")!=std::string::npos&&r.find("256:btn_dpad1_up")!=std::string::npos);
  assert(r.find("5+:btn_z")!=std::string::npos&&r.find("triggers = 4,5")!=std::string::npos&&r.find("0-:btn_dpad1_left")!=std::string::npos);
  // PlayStation through HIDAPI: LB = button 9, Back = 4, D-pad buttons 11-14
  const std::string h=mcl::flycastPadCfg(m,'h');
  assert(h.find("9:btn_c")!=std::string::npos&&h.find("4:btn_menu")!=std::string::npos&&h.find("11:btn_dpad1_up")!=std::string::npos);
  assert(mcl::flycastPadCfg(m,'?').empty());
  const auto pads=mcl::flycastPadsFromLog(
   "00:00:00:346 sdl/dreamlink.cpp:52 N[INPUT]: GUID: 0300b9695e0400008e02000000007200 VID:045e PID:028e\n"
   "00:00:00:347 sdl/sdl_gamepad.cpp:221 N[INPUT]: SDL: Opened joystick 0 on port 0: 'Xbox 360 Controller' unique_id=sdl_joystick_0\n"
   "00:00:01:000 x N[INPUT]: GUID: 050000004c050000e60c000000006800 VID:054c PID:0ce6\n"
   "00:00:01:001 x N[INPUT]: SDL: Opened joystick 1 on port 1: 'PS5 Controller' unique_id=sdl_joystick_1\n");
  assert(pads.size()==2&&pads[0].first=='r'&&pads[0].second=="Xbox 360 Controller"&&pads[1].first=='h'&&pads[1].second=="PS5 Controller");
 }
 std::cout<<"PASS: controls config, MAME controller file, key names, labels, MvC2 Flycast mappings\n";
}
