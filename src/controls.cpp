#include "controls.hpp"
#include <cctype>
#include <fstream>
#include <sstream>
#include <system_error>
namespace mcl {
const std::vector<GameInput>& gameInputs(){
 // Keyboard defaults are MAME's own; pad defaults are MAME's standard joystick items.
 static const std::vector<GameInput> v={
  {"P1_JOYSTICK_UP","UP","KEYCODE_UP","JOYCODE_1_YAXIS_UP_SWITCH OR JOYCODE_1_HAT1UP"},
  {"P1_JOYSTICK_DOWN","DOWN","KEYCODE_DOWN","JOYCODE_1_YAXIS_DOWN_SWITCH OR JOYCODE_1_HAT1DOWN"},
  {"P1_JOYSTICK_LEFT","LEFT","KEYCODE_LEFT","JOYCODE_1_XAXIS_LEFT_SWITCH OR JOYCODE_1_HAT1LEFT"},
  {"P1_JOYSTICK_RIGHT","RIGHT","KEYCODE_RIGHT","JOYCODE_1_XAXIS_RIGHT_SWITCH OR JOYCODE_1_HAT1RIGHT"},
  {"P1_BUTTON1","LIGHT PUNCH","KEYCODE_LCONTROL","JOYCODE_1_BUTTON1"},
  {"P1_BUTTON2","MEDIUM PUNCH","KEYCODE_LALT","JOYCODE_1_BUTTON2"},
  {"P1_BUTTON3","HEAVY PUNCH","KEYCODE_SPACE","JOYCODE_1_BUTTON3"},
  {"P1_BUTTON4","LIGHT KICK","KEYCODE_LSHIFT","JOYCODE_1_BUTTON4"},
  {"P1_BUTTON5","MEDIUM KICK","KEYCODE_Z","JOYCODE_1_BUTTON5"},
  {"P1_BUTTON6","HEAVY KICK","KEYCODE_X","JOYCODE_1_BUTTON6"},
  {"START1","START","KEYCODE_1","JOYCODE_1_START OR JOYCODE_1_BUTTON10"},
  {"COIN1","COIN (SELECT)","KEYCODE_5","JOYCODE_1_SELECT OR JOYCODE_1_BUTTON9"},
 };
 return v;
}
ControlsConfig::ControlsConfig(){resetDefaults();}
void ControlsConfig::resetDefaults(){
 b_.clear();
 for(const auto& g:gameInputs())b_[g.port]={g.defaultKey,g.defaultPad};
 menuConfirm=menuBack=menuSelect=menuStart=-1;
}
bool ControlsConfig::isDefault() const{
 for(const auto& g:gameInputs()){const auto& b=binding(g.port);if(b.key!=g.defaultKey||b.pad!=g.defaultPad)return false;}
 return menuConfirm<0&&menuBack<0&&menuSelect<0&&menuStart<0;
}
ControlsConfig::Binding& ControlsConfig::binding(const std::string& port){return b_[port];}
const ControlsConfig::Binding& ControlsConfig::binding(const std::string& port) const{
 static const Binding none;const auto it=b_.find(port);return it==b_.end()?none:it->second;
}
static bool validCode(const std::string& s){
 if(s.size()>200)return false;
 for(char c:s)if(!(std::isalnum(static_cast<unsigned char>(c))||c=='_'||c==' '))return false;
 return true;
}
bool ControlsConfig::load(const std::filesystem::path& file){
 std::ifstream in(file);if(!in)return false;
 std::string line;
 while(std::getline(in,line)){
  if(!line.empty()&&line.back()=='\r')line.pop_back();
  const auto eq=line.find('=');if(eq==std::string::npos)continue;
  const std::string k=line.substr(0,eq),v=line.substr(eq+1);
  try{
   if(k=="menu_confirm")menuConfirm=std::stoi(v);
   else if(k=="menu_back")menuBack=std::stoi(v);
   else if(k=="menu_select")menuSelect=std::stoi(v);
   else if(k=="menu_start")menuStart=std::stoi(v);
  }catch(...){}
  const auto dot=k.rfind('.');if(dot==std::string::npos||!validCode(v))continue;
  const std::string port=k.substr(0,dot),which=k.substr(dot+1);
  bool known=false;for(const auto& g:gameInputs())known=known||port==g.port;
  if(!known)continue;
  if(which=="key")b_[port].key=v;else if(which=="pad")b_[port].pad=v;
 }
 return true;
}
bool ControlsConfig::save(const std::filesystem::path& file) const{
 std::error_code ec;if(file.has_parent_path())std::filesystem::create_directories(file.parent_path(),ec);
 std::ofstream out(file,std::ios::trunc);if(!out)return false;
 out<<"# Marvel: Crossover Legacy controls (OPTIONS > CONTROLS SETUP)\n";
 for(const auto& g:gameInputs()){const auto& b=binding(g.port);out<<g.port<<".key="<<b.key<<"\n"<<g.port<<".pad="<<b.pad<<"\n";}
 out<<"menu_confirm="<<menuConfirm<<"\nmenu_back="<<menuBack<<"\nmenu_select="<<menuSelect<<"\nmenu_start="<<menuStart<<"\n";
 return static_cast<bool>(out);
}
std::string ControlsConfig::mameCtrlrXml() const{
 std::ostringstream o;
 o<<"<?xml version=\"1.0\"?>\n<!-- Written by Marvel: Crossover Legacy (OPTIONS > CONTROLS SETUP) -->\n"
    "<mameconfig version=\"10\">\n    <system name=\"default\">\n        <input>\n";
 for(const auto& g:gameInputs()){
  const auto& b=binding(g.port);
  std::string seq=b.key;
  if(!b.pad.empty())seq+=(seq.empty()?"":" OR ")+b.pad;
  if(seq.empty())seq="NONE";
  o<<"            <port type=\""<<g.port<<"\">\n                <newseq type=\"standard\">"<<seq<<"</newseq>\n            </port>\n";
 }
 o<<"        </input>\n    </system>\n</mameconfig>\n";
 return o.str();
}
std::string mameKeyName(int vk){
 if(vk>='A'&&vk<='Z')return std::string("KEYCODE_")+static_cast<char>(vk);
 if(vk>='0'&&vk<='9')return std::string("KEYCODE_")+static_cast<char>(vk);
 if(vk>=0x60&&vk<=0x69)return "KEYCODE_"+std::to_string(vk-0x60)+"_PAD";
 if(vk>=0x70&&vk<=0x7B)return "KEYCODE_F"+std::to_string(vk-0x6F);
 switch(vk){
  case 0x26:return "KEYCODE_UP";case 0x28:return "KEYCODE_DOWN";case 0x25:return "KEYCODE_LEFT";case 0x27:return "KEYCODE_RIGHT";
  case 0x20:return "KEYCODE_SPACE";case 0x0D:return "KEYCODE_ENTER";case 0x09:return "KEYCODE_TAB";case 0x08:return "KEYCODE_BACKSPACE";
  case 0xA0:return "KEYCODE_LSHIFT";case 0xA1:return "KEYCODE_RSHIFT";case 0xA2:return "KEYCODE_LCONTROL";case 0xA3:return "KEYCODE_RCONTROL";
  case 0xA4:return "KEYCODE_LALT";case 0xA5:return "KEYCODE_RALT";
  case 0x10:return "KEYCODE_LSHIFT";case 0x11:return "KEYCODE_LCONTROL";case 0x12:return "KEYCODE_LALT";
  case 0x2D:return "KEYCODE_INSERT";case 0x2E:return "KEYCODE_DEL";case 0x24:return "KEYCODE_HOME";case 0x23:return "KEYCODE_END";
  case 0x21:return "KEYCODE_PGUP";case 0x22:return "KEYCODE_PGDN";
  case 0xBA:return "KEYCODE_COLON";case 0xBB:return "KEYCODE_EQUALS";case 0xBC:return "KEYCODE_COMMA";case 0xBD:return "KEYCODE_MINUS";
  case 0xBE:return "KEYCODE_STOP";case 0xBF:return "KEYCODE_SLASH";case 0xC0:return "KEYCODE_TILDE";case 0xDB:return "KEYCODE_OPENBRACE";
  case 0xDC:return "KEYCODE_BACKSLASH";case 0xDD:return "KEYCODE_CLOSEBRACE";case 0xDE:return "KEYCODE_QUOTE";
  case 0x6A:return "KEYCODE_ASTERISK";case 0x6B:return "KEYCODE_PLUS_PAD";case 0x6D:return "KEYCODE_MINUS_PAD";case 0x6E:return "KEYCODE_DEL_PAD";
  case 0x6F:return "KEYCODE_SLASH_PAD";
 }
 return {};
}
std::string inputLabel(const std::string& code){
 if(code.empty())return "-";
 std::string first=code.substr(0,code.find(" OR "));
 if(first.rfind("KEYCODE_",0)==0){
  std::string k=first.substr(8);
  if(k=="LCONTROL")return "L-CTRL";if(k=="RCONTROL")return "R-CTRL";if(k=="LALT")return "L-ALT";if(k=="RALT")return "R-ALT";
  if(k=="LSHIFT")return "L-SHIFT";if(k=="RSHIFT")return "R-SHIFT";
  return k;
 }
 if(first.rfind("JOYCODE_",0)==0){
  const auto us=first.find('_',8);
  const std::string pad=first.substr(8,us-8),item=us==std::string::npos?"":first.substr(us+1);
  std::string name=item;
  if(item.rfind("BUTTON",0)==0)name="BTN "+item.substr(6);
  else if(item.rfind("HAT1",0)==0)name="D-PAD "+item.substr(4);
  else if(item=="YAXIS_UP_SWITCH")name="STICK UP";else if(item=="YAXIS_DOWN_SWITCH")name="STICK DOWN";
  else if(item=="XAXIS_LEFT_SWITCH")name="STICK LEFT";else if(item=="XAXIS_RIGHT_SWITCH")name="STICK RIGHT";
  else if(item=="ZAXIS_POS_SWITCH")name="TRIGGER +";else if(item=="ZAXIS_NEG_SWITCH")name="TRIGGER -";
  else if(item=="RZAXIS_POS_SWITCH")name="R-TRIGGER";else if(item=="RZAXIS_NEG_SWITCH")name="R-TRIGGER -";
  return "PAD "+pad+" "+name;
 }
 return first;
}
}
