#include "accessibility_lua.hpp"
#include "achievement_lua.hpp"
#include <cstdio>
#include <sstream>
#include <vector>
namespace mcl {
namespace {
// One fighter slot: where its facing lives, and (tag games) its "on screen" flag.
struct Slot { unsigned active; unsigned facing; int bit; bool setMeansRight; }; // active 0 = always
struct Game { const char* id; std::vector<Slot> p1,p2; const char* superKind; };
const std::vector<Game>& games(){
 static const std::vector<Game> g={
  // CPS-2: facing byte +0xA3 of the player block, 1 = facing right
  {"xmcota",{{0,0xFF40A3,0,true}},{{0,0xFF44A3,0,true}},"three"},
  {"msh",{{0,0xFF40A3,0,true}},{{0,0xFF44A3,0,true}},"three"},
  // X-Men vs. SF swaps the team's blocks on a tag: the first block is always the active fighter
  {"xmvsf",{{0,0xFF40A3,0,true}},{{0,0xFF44A3,0,true}},"two"},
  // MSH vs. SF / MvC: blocks stay put, +0x000 = 1 for the fighter on screen
  {"mshvsf",{{0xFF3800,0xFF38A3,0,true},{0xFF4000,0xFF40A3,0,true}},{{0xFF3C00,0xFF3CA3,0,true},{0xFF4400,0xFF44A3,0,true}},"two"},
  {"mvsc",{{0xFF3000,0xFF30B3,0,true},{0xFF3800,0xFF38B3,0,true}},{{0xFF3400,0xFF34B3,0,true},{0xFF3C00,0xFF3CB3,0,true}},"two"},
  // ZN (EX): bit 7 of the facing byte is set while facing left
  {"sfexp",{{0,0x3235F8,7,false}},{{0,0x324A6C,7,false}},"double"},
  {"sfex2p",{{0,0x2B5170,7,false}},{{0,0x2B6974,7,false}},"double"},
 };
 return g;
}
}
std::string buildOneButtonLua(const std::string& id,bool specials,bool supers){
 const Game* game=nullptr;for(const auto& g:games())if(id==g.id)game=&g;
 if(!game||(!specials&&!supers))return {};
 std::ostringstream o;char buf[160];
 const auto slots=[&](const std::vector<Slot>& v){
  std::string s="{";
  for(const auto& x:v){std::snprintf(buf,sizeof buf,"{active=%u, facing=%u, bit=%d, right=%s},",x.active,x.facing,x.bit,x.setMeansRight?"true":"false");s+=buf;}
  return s+"}";
 };
 // "In a fight" (moves only then, so menus keep their normal buttons): both teams have a fighter
 // with health in range, and the round timer is full or has moved in the last 3 seconds.
 const DetectorProfile prof=detectorProfile(id);
 const auto list=[&](const std::vector<unsigned>& v){std::string s="{";for(unsigned a:v){std::snprintf(buf,sizeof buf,"%u,",a);s+=buf;}return s+"}";};
 o<<"-- Marvel: Crossover Legacy ACCESSIBILITY: 1-button specials / supers ("<<game->id<<")\n"
    "do\n"
    "local FIGHT = { p1="<<list(prof.p1)<<", p2="<<list(prof.p2)<<", max="<<prof.maxHealth<<", zeroDead="<<(prof.zeroIsDead?"true":"false")
    <<", timer="<<prof.timer<<", full="<<prof.timerFull<<" }\n";
 o<<
    "local mach = manager.machine\n"
    "local mem = mach.devices[':maincpu'].spaces['program']\n"
    "local input = mach.input\n"
    "local SPECIALS, SUPERS = "<<(specials?"true":"false")<<", "<<(supers?"true":"false")<<"\n"
    "local SUPER = '"<<game->superKind<<"'\n"
    "local SLOTS = { "<<slots(game->p1)<<", "<<slots(game->p2)<<" }\n";
 o<<R"LUA(
-- fields per player: directions and the six attack buttons
local players = {}
for pl = 1, 2 do
  local P = { f = {}, prev = {}, queue = nil, step = 0 }
  for _, port in pairs(mach.ioport.ports) do
    for _, fld in pairs(port.fields) do
      for _, n in ipairs({"JOYSTICK_UP", "JOYSTICK_DOWN", "JOYSTICK_LEFT", "JOYSTICK_RIGHT",
                          "BUTTON1", "BUTTON2", "BUTTON3", "BUTTON4", "BUTTON5", "BUTTON6"}) do
        local t, who = mach.ioport:token_to_input_type("P" .. pl .. "_" .. n)
        if fld.type == t and fld.player == who then P.f[n] = fld end
      end
    end
  end
  players[pl] = P
end
-- Real (physical) state of a field; MCL_TEST_PRESSED lets the test harness press buttons.
local function physical(pl, name)
  if MCL_TEST_PRESSED and MCL_TEST_PRESSED["P" .. pl .. "_" .. name] then return true end
  local fld = players[pl].f[name]
  if not fld then return false end
  local ok, seq = pcall(function() return fld:input_seq("standard") end)
  if not ok or not seq then return false end
  local ok2, pressed = pcall(function() return input:seq_pressed(seq) end)
  return ok2 and pressed or false
end
local function facingRight(pl)
  local pick = SLOTS[pl][1]
  for _, s in ipairs(SLOTS[pl]) do
    if s.active == 0 or (mem:read_u8(s.active) & 1) == 1 then pick = s; break end
  end
  local set = (mem:read_u8(pick.facing) >> pick.bit) & 1 == 1
  return set == pick.right
end
-- Moves as lists of {directions, buttons, frames}; "f"/"b" become left/right by facing.
local function motion(seq, buttons)
  local out = {}
  for i, d in ipairs(seq) do out[#out + 1] = { d, (i == #seq) and buttons or "", 2 } end
  out[#out][3] = 4 -- hold the last step (and the buttons) a little longer
  return out
end
local QCF, QCB, DP = {"d", "df", "f"}, {"d", "db", "b"}, {"f", "d", "df"}
local MOVES = {}
if SPECIALS then
  MOVES.BUTTON1 = motion(QCF, "1")   -- light punch
  MOVES.BUTTON2 = motion(DP, "2")    -- medium punch
  MOVES.BUTTON4 = motion(QCB, "4")   -- light kick
  MOVES.BUTTON5 = motion(QCF, "5")   -- medium kick
end
if SUPERS then
  if SUPER == "three" then MOVES.BUTTON3 = motion(QCF, "123"); MOVES.BUTTON6 = motion(QCF, "456")
  elseif SUPER == "two" then MOVES.BUTTON3 = motion(QCF, "13"); MOVES.BUTTON6 = motion(QCF, "46")
  else MOVES.BUTTON3 = motion({"d", "df", "f", "d", "df", "f"}, "3"); MOVES.BUTTON6 = motion({"d", "df", "f", "d", "df", "f"}, "6") end
end
local DIRS = {"JOYSTICK_UP", "JOYSTICK_DOWN", "JOYSTICK_LEFT", "JOYSTICK_RIGHT"}
local lastTimer, timerMoved, frame = -1, -1000, 0
local function alive(v) if v > FIGHT.max then return false end; if v == 0 then return not FIGHT.zeroDead end; return true end
local function teamAlive(slots) for _, a in ipairs(slots) do if alive(mem:read_u8(a)) then return true end end; return false end
local function inFight()
  if FIGHT.timer == 0 or #FIGHT.p1 == 0 then return true end
  local t = mem:read_u8(FIGHT.timer)
  if t ~= lastTimer then lastTimer = t; timerMoved = frame end
  return teamAlive(FIGHT.p1) and teamAlive(FIGHT.p2) and (t == FIGHT.full or frame - timerMoved < 180)
end
local function drive(P, dirs, buttons)
  P.f.JOYSTICK_UP:set_value(dirs:find("u") and 1 or 0)
  P.f.JOYSTICK_DOWN:set_value(dirs:find("d") and 1 or 0)
  P.f.JOYSTICK_LEFT:set_value(dirs:find("L") and 1 or 0)
  P.f.JOYSTICK_RIGHT:set_value(dirs:find("R") and 1 or 0)
  for b = 1, 6 do
    local fld = P.f["BUTTON" .. b]
    if fld then fld:set_value(buttons:find(tostring(b)) and 1 or 0) end
  end
end
local released = true
mcl_a11y_sub = emu.add_machine_frame_notifier(function()
  frame = frame + 1
  local fighting = inFight()
  if MCL_TEST_PRESSED then MCL_A11Y_FIGHTING = fighting end -- test harness only
  if not fighting then
    -- menus, select screens, intros: hand every input back to the player
    if not released then
      for pl = 1, 2 do
        local P = players[pl]
        for _, fld in pairs(P.f) do fld:clear_value() end
        P.queue = nil
      end
      released = true
    end
    for pl = 1, 2 do for name in pairs(MOVES) do players[pl].prev[name] = physical(pl, name) end end
    return
  end
  released = false
  for pl = 1, 2 do
    local P = players[pl]
    if P.f.JOYSTICK_UP and P.f.JOYSTICK_DOWN and P.f.JOYSTICK_LEFT and P.f.JOYSTICK_RIGHT then
      -- new presses of a 1-button move start it (one at a time)
      for name, move in pairs(MOVES) do
        local now = physical(pl, name)
        if now and not P.prev[name] and not P.queue then
          local right = facingRight(pl)
          local steps = {}
          for _, s in ipairs(move) do
            local d = s[1]:gsub("f", right and "R" or "L"):gsub("b", right and "L" or "R")
            for _ = 1, s[3] do steps[#steps + 1] = { d, s[2] } end
          end
          P.queue, P.step = steps, 1
        end
        P.prev[name] = now
      end
      if P.queue then
        local s = P.queue[P.step]
        drive(P, s[1], s[2])
        P.step = P.step + 1
        if P.step > #P.queue then P.queue = nil end
      else
        -- not in a move: the stick is the player's own; buttons taken over by a move stay up
        for _, d in ipairs(DIRS) do P.f[d]:clear_value() end
        for b = 1, 6 do
          local fld = P.f["BUTTON" .. b]
          if fld then if MOVES["BUTTON" .. b] then fld:set_value(0) else fld:clear_value() end end
        end
      end
    end
  end
end)
end
)LUA";
 return o.str();
}
}
