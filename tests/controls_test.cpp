#include "controls.hpp"
#include <cassert>
#include <filesystem>
#include <iostream>
namespace fs=std::filesystem;
int main(){
 mcl::ControlsConfig c;
 assert(c.isDefault()&&mcl::gameInputs().size()==12);
 assert(c.binding("P1_BUTTON1").key=="KEYCODE_LCONTROL");
 c.binding("P1_BUTTON1").key="KEYCODE_A";c.binding("P1_BUTTON6").pad="JOYCODE_2_ZAXIS_POS_SWITCH";c.menuConfirm=1;
 assert(!c.isDefault());
 const auto xml=c.mameCtrlrXml();
 assert(xml.find("<port type=\"P1_BUTTON1\">")!=std::string::npos);
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
 std::cout<<"PASS: controls config, MAME controller file, key names, labels\n";
}
