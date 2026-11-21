#include "controls.hpp"
#include <cctype>
#include <fstream>
#include <sstream>
#include <system_error>
namespace mcl {
const std::vector<GameInput>& gameInputs(){
 // Keyboard defaults are MAME's own; pad defaults are MAME's standard joystick items.
 // Player 1 (12 inputs) then Player 2 (12 inputs, same order). Keys are MAME's defaults.
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
  {"P2_JOYSTICK_UP","UP","KEYCODE_R","JOYCODE_2_YAXIS_UP_SWITCH OR JOYCODE_2_HAT1UP"},
  {"P2_JOYSTICK_DOWN","DOWN","KEYCODE_F","JOYCODE_2_YAXIS_DOWN_SWITCH OR JOYCODE_2_HAT1DOWN"},
  {"P2_JOYSTICK_LEFT","LEFT","KEYCODE_D","JOYCODE_2_XAXIS_LEFT_SWITCH OR JOYCODE_2_HAT1LEFT"},
  {"P2_JOYSTICK_RIGHT","RIGHT","KEYCODE_G","JOYCODE_2_XAXIS_RIGHT_SWITCH OR JOYCODE_2_HAT1RIGHT"},
  {"P2_BUTTON1","LIGHT PUNCH","KEYCODE_A","JOYCODE_2_BUTTON1"},
  {"P2_BUTTON2","MEDIUM PUNCH","KEYCODE_S","JOYCODE_2_BUTTON2"},
  {"P2_BUTTON3","HEAVY PUNCH","KEYCODE_Q","JOYCODE_2_BUTTON3"},
  {"P2_BUTTON4","LIGHT KICK","KEYCODE_W","JOYCODE_2_BUTTON4"},
  {"P2_BUTTON5","MEDIUM KICK","KEYCODE_E","JOYCODE_2_BUTTON5"},
  {"P2_BUTTON6","HEAVY KICK","KEYCODE_T","JOYCODE_2_BUTTON6"},
  {"START2","START","KEYCODE_2","JOYCODE_2_START OR JOYCODE_2_BUTTON10"},
  {"COIN2","COIN (SELECT)","KEYCODE_6","JOYCODE_2_SELECT OR JOYCODE_2_BUTTON9"},
 };
 return v;
}
ControlsConfig::ControlsConfig(Layout layout):layout_(layout){resetDefaults();}
const char* mvc2PadDefault(const std::string& port){
 // Like the home ports of MvC2: X/Y punches, A/B kicks, LB/RB assists.
 if(port=="P1_BUTTON1")return "PAD_X";if(port=="P1_BUTTON2")return "PAD_Y";if(port=="P1_BUTTON3")return "PAD_LB";
 if(port=="P1_BUTTON4")return "PAD_A";if(port=="P1_BUTTON5")return "PAD_B";if(port=="P1_BUTTON6")return "PAD_RB";
 if(port=="START1")return "PAD_START";
 if(port.rfind("P1_JOYSTICK_",0)==0)return "PAD_DPAD";
 return "";
}
const std::vector<std::string>& mvc2PadButtons(){
 static const std::vector<std::string> v={"PAD_A","PAD_B","PAD_X","PAD_Y","PAD_LB","PAD_RB","PAD_LT","PAD_RT","PAD_START","PAD_LS","PAD_RS"};
 return v;
}
static const char* defaultPadFor(ControlsConfig::Layout l,const GameInput& g){return l==ControlsConfig::Layout::Mvc2?mvc2PadDefault(g.port):g.defaultPad;}
void ControlsConfig::resetDefaults(){
 b_.clear();
 for(const auto& g:gameInputs())b_[g.port]={g.defaultKey,defaultPadFor(layout_,g)};
 menuConfirm=menuBack=menuSelect=menuStart=-1;
}
bool ControlsConfig::isDefault() const{
 for(const auto& g:gameInputs()){const auto& b=binding(g.port);if(b.key!=g.defaultKey||b.pad!=defaultPadFor(layout_,g))return false;}
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
const char* mvc2Label(const std::string& full){
 // same labels for both players
 std::string port=full;
 if(port.rfind("P2_",0)==0)port="P1_"+port.substr(3);
 if(port=="START2")port="START1";
 if(port=="COIN2")port="COIN1";
 if(port=="P1_BUTTON1")return "LIGHT PUNCH";if(port=="P1_BUTTON2")return "HEAVY PUNCH";if(port=="P1_BUTTON3")return "ASSIST 1";
 if(port=="P1_BUTTON4")return "LIGHT KICK";if(port=="P1_BUTTON5")return "HEAVY KICK";if(port=="P1_BUTTON6")return "ASSIST 2";
 for(const auto& g:gameInputs())if(port==g.port)return g.label;
 return "";
}
int sdlScancode(const std::string& code){
 if(code.rfind("KEYCODE_",0)!=0)return -1;
 const std::string k=code.substr(8);
 if(k.size()==1&&k[0]>='A'&&k[0]<='Z')return 4+(k[0]-'A');
 if(k.size()==1&&k[0]>='1'&&k[0]<='9')return 30+(k[0]-'1');
 if(k=="0")return 39;
 if(k.size()==6&&k.compare(1,5,"_PAD")==0&&k[0]>='0'&&k[0]<='9')return k[0]=='0'?98:89+(k[0]-'1');
 if(k.size()>=2&&k[0]=='F'&&k.find_first_not_of("0123456789",1)==std::string::npos){const int n=std::stoi(k.substr(1));if(n>=1&&n<=12)return 57+n;}
 static const std::map<std::string,int> named={
  {"ENTER",40},{"ESC",41},{"BACKSPACE",42},{"TAB",43},{"SPACE",44},{"MINUS",45},{"EQUALS",46},{"OPENBRACE",47},{"CLOSEBRACE",48},
  {"BACKSLASH",49},{"COLON",51},{"QUOTE",52},{"TILDE",53},{"COMMA",54},{"STOP",55},{"SLASH",56},
  {"INSERT",73},{"HOME",74},{"PGUP",75},{"DEL",76},{"END",77},{"PGDN",78},{"RIGHT",79},{"LEFT",80},{"DOWN",81},{"UP",82},
  {"SLASH_PAD",84},{"ASTERISK",85},{"MINUS_PAD",86},{"PLUS_PAD",87},{"ENTER_PAD",88},{"DEL_PAD",99},
  {"LCONTROL",224},{"LSHIFT",225},{"LALT",226},{"RCONTROL",228},{"RSHIFT",229},{"RALT",230}};
 const auto it=named.find(k);return it==named.end()?-1:it->second;
}
std::string flycastKeyboardCfg(const ControlsConfig& c){
 // The keyboard runs in Flycast's "all players" mode (launched with input:maple_sdl_keyboard=4):
 // unsuffixed names drive player 1, names ending in 1 drive player 2.
 static const std::pair<const char*,const char*> map[]={
  {"JOYSTICK_UP","btn_dpad1_up"},{"JOYSTICK_DOWN","btn_dpad1_down"},{"JOYSTICK_LEFT","btn_dpad1_left"},{"JOYSTICK_RIGHT","btn_dpad1_right"},
  {"BUTTON1","btn_a"},{"BUTTON2","btn_b"},{"BUTTON3","btn_c"},{"BUTTON4","btn_x"},{"BUTTON5","btn_y"},{"BUTTON6","btn_z"},
  {"START","btn_start"},{"COIN","btn_d"}};
 std::map<int,std::string> binds[2]; // per player: scancode -> Flycast button (one action per key)
 binds[0][43]="btn_menu";             // Tab: the front end's MvC2 pause screen
 for(int player=0;player<2;++player){
  for(const auto& m:map){
   const std::string what=m.first;
   const std::string port=what=="START"||what=="COIN"?what+std::to_string(player+1):"P"+std::to_string(player+1)+"_"+what;
   std::string seq=c.binding(port).key;
   for(size_t at=0;;){
    const size_t end=seq.find(" OR ",at);
    const int sc=sdlScancode(seq.substr(at,end==std::string::npos?std::string::npos:end-at));
    if(sc>=0&&sc!=43)binds[player][sc]=std::string(m.second)+(player?"1":"");
    if(end==std::string::npos)break;at=end+4;
   }
  }
 }
 std::ostringstream o;
 o<<"[digital]\n";int n=0;
 for(const auto& table:binds)for(const auto& b:table)o<<"bind"<<n++<<" = "<<b.first<<":"<<b.second<<"\n";
 o<<"\n[emulator]\ndead_zone = 10\nmapping_name = Keyboard\nrumble_power = 100\nsaturation = 100\nversion = 4\n";
 return o.str();
}
// Raw SDL joystick numbers of the named buttons, per backend (Flycast stores raw numbers).
//  r/x: Xbox layout from Raw Input / XInput: A0 B1 X2 Y3 LB4 RB5 BACK6 START7 LS8 RS9, D-pad = hat 0,
//       triggers = axes 4,5 (Raw Input) or 2,5 (XInput).
//  h:   HIDAPI pads use SDL's controller order: A0 B1 X2 Y3 BACK4 GUIDE5 START6 LS7 RS8 LB9 RB10,
//       D-pad = buttons 11-14, triggers = axes 4,5.
std::string flycastPadCfg(const ControlsConfig& c,char backend){
 if(backend!='r'&&backend!='x'&&backend!='h')return {};
 const bool hid=backend=='h';
 const std::map<std::string,int> btn=hid?std::map<std::string,int>{{"PAD_A",0},{"PAD_B",1},{"PAD_X",2},{"PAD_Y",3},{"PAD_BACK",4},{"PAD_START",6},{"PAD_LS",7},{"PAD_RS",8},{"PAD_LB",9},{"PAD_RB",10}}
                                    :std::map<std::string,int>{{"PAD_A",0},{"PAD_B",1},{"PAD_X",2},{"PAD_Y",3},{"PAD_LB",4},{"PAD_RB",5},{"PAD_BACK",6},{"PAD_START",7},{"PAD_LS",8},{"PAD_RS",9}};
 const int lt=backend=='x'?2:4,rt=5;
 static const std::pair<const char*,const char*> act[]={
  {"P1_BUTTON1","btn_a"},{"P1_BUTTON2","btn_b"},{"P1_BUTTON3","btn_c"},{"P1_BUTTON4","btn_x"},{"P1_BUTTON5","btn_y"},{"P1_BUTTON6","btn_z"},{"START1","btn_start"}};
 std::map<int,std::string> digital;std::map<std::string,std::string> analog; // "4+" -> action
 digital[btn.at("PAD_BACK")]="btn_menu"; // Back/Select: the front end's MvC2 pause screen
 if(hid){digital[11]="btn_dpad1_up";digital[12]="btn_dpad1_down";digital[13]="btn_dpad1_left";digital[14]="btn_dpad1_right";}
 else{digital[256]="btn_dpad1_up";digital[257]="btn_dpad1_down";digital[258]="btn_dpad1_left";digital[259]="btn_dpad1_right";}
 analog["0-"]="btn_dpad1_left";analog["0+"]="btn_dpad1_right";analog["1-"]="btn_dpad1_up";analog["1+"]="btn_dpad1_down";
 for(const auto& a:act){
  const std::string pad=c.binding(a.first).pad;
  if(pad=="PAD_LT")analog[std::to_string(lt)+"+"]=a.second;
  else if(pad=="PAD_RT")analog[std::to_string(rt)+"+"]=a.second;
  else{const auto it=btn.find(pad);if(it!=btn.end()&&pad!="PAD_BACK")digital[it->second]=a.second;}
 }
 std::ostringstream o;int n=0;
 o<<"[analog]\n";for(const auto& a:analog)o<<"bind"<<n++<<" = "<<a.first<<":"<<a.second<<"\n";
 n=0;o<<"\n[digital]\n";for(const auto& d:digital)o<<"bind"<<n++<<" = "<<d.first<<":"<<d.second<<"\n";
 o<<"\n[emulator]\ndead_zone = 10\nmapping_name = Marvel: Crossover Legacy (MvC2)\nrumble_power = 100\nsaturation = 100\ntriggers = "<<lt<<","<<rt<<"\nversion = 4\n";
 return o.str();
}
std::vector<std::pair<char,std::string>> flycastPadsFromLog(const std::string& log){
 // "...GUID: 0300b9695e0400008e02000000007200 VID:045e PID:028e"
 // "...SDL: Opened joystick 0 on port 0: 'Xbox 360 Controller' unique_id=sdl_joystick_0"
 std::vector<std::pair<char,std::string>> out;char backend=0;
 std::istringstream in(log);std::string line;
 while(std::getline(in,line)){
  const auto g=line.find("GUID: ");
  if(g!=std::string::npos&&line.size()>=g+6+32){
   const std::string hex=line.substr(g+6+28,2); // byte 14 = driver: 'r' Raw Input, 'x' XInput, 'h' HIDAPI
   try{backend=static_cast<char>(std::stoi(hex,nullptr,16));}catch(...){backend=0;}
   continue;
  }
  const auto o=line.find("Opened joystick ");
  if(o==std::string::npos)continue;
  const auto q1=line.find('\'',o),q2=q1==std::string::npos?q1:line.find('\'',q1+1);
  if(q2==std::string::npos)continue;
  const std::string name=line.substr(q1+1,q2-q1-1);
  bool seen=false;for(const auto& p:out)seen=seen||p.second==name;
  if(!name.empty()&&!seen)out.push_back({backend,name});
  backend=0;
 }
 return out;
}
std::string inputLabel(const std::string& code){
 if(code.empty())return "-";
 if(code=="PAD_DPAD")return "D-PAD / STICK";
 if(code.rfind("PAD_",0)==0)return code.substr(4);
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
