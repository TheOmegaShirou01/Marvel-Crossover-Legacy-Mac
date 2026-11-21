"""Extract character portraits from the user's own ROMs for the Gallery.

    py -3 tools/extract_portraits.py [--mame <mame.exe>] [--rom-list bin/rom_directory.txt]
                                     [--out bin/Gallery/Art] [--only xmcota,msh,...] [--jobs 6]

How it works (found 2026-11-21): every game shows a large portrait of Player 1's fighter
on its character select screen (EX Plus: on its VS screen). MAME runs headless, inserts
coins, presses Start, and writes P1's character byte (addresses from MAME's cheat
database "Select Character") so the screen shows each fighter in turn; a snapshot of
that screen is cropped to the portrait. Two methods:
  cycle  one run; the byte is stepped every 40 frames while the select screen is up
         (games whose portrait follows the byte: COTA, XvSF, MSHvSF, MvC, EX2 Plus)
  hold   one run per fighter; the byte is held from the coin onward and one snapshot is
         taken at a fixed frame (MSH select screen, EX Plus VS screen)
Output: <out>/<slug>/rom_<game>.png at the game's own resolution (pixel art; the front
end scales it with nearest-neighbour). Generated per machine from the user's ROMs and
never committed. MvC2 runs in Flycast and is not covered.
"""
import argparse, os, re, struct, subprocess, sys, tempfile, zlib
from concurrent.futures import ThreadPoolExecutor

LUA = r'''
local m = manager.machine
local mem = m.devices[":maincpu"].spaces["program"]
local fields = {}
for _, port in pairs(m.ioport.ports) do
  for _, f in pairs(port.fields) do
    for _, tok in ipairs({"COIN1", "START1"}) do
      if f.type == m.ioport:token_to_input_type(tok) and f.player == 0 then fields[tok] = f end
    end
  end
end
local P = PORT
local fr = 0
mcl_portraits = emu.add_machine_frame_notifier(function()
  fr = fr + 1
  local c = P.coin
  if fields.COIN1 then fields.COIN1:set_value(((fr >= c and fr < c + 8) or (fr >= c + 30 and fr < c + 38)) and 1 or 0) end
  if fields.START1 then fields.START1:set_value((fr >= c + 90 and fr < c + 98) and 1 or 0) end
  if P.hold and fr >= c then mem:write_u8(P.addr, P.hold) end
  if P.snapat then
    if fr == P.snapat then m.video:snapshot(); print("MCL_SNAP " .. P.hold) end
    return
  end
  if fr >= P.start then
    local k = math.floor((fr - P.start) / P.period) + 1
    if k <= #P.ids then
      mem:write_u8(P.addr, P.ids[k])
      if (fr - P.start) % P.period == P.period - 3 then m.video:snapshot(); print("MCL_SNAP " .. P.ids[k]) end
    end
  end
end)
'''

# id -> character name (gallery roster names); None = no usable portrait (skipped)
GAMES = {
 "xmcota": dict(set="xmcotau", method="cycle", addr=0xFF4051, coin=600, start=900, crop=(0, 8, 80, 80), secs=35,
   ids={0x00:"Wolverine",0x02:"Psylocke",0x04:"Colossus",0x06:"Cyclops",0x08:"Storm",0x0A:"Iceman",0x0C:"Spiral",
        0x0E:"Silver Samurai",0x10:"Omega Red",0x12:"Sentinel",0x14:"Juggernaut",0x16:"Magneto"}),
 "msh": dict(set="mshud", method="hold", addr=0xFF4051, coin=2400, snapat=3300, crop=(12, 0, 184, 100), secs=57,
   ids={0x00:"Spider-Man",0x02:"Captain America",0x04:"Hulk",0x06:"Iron Man",0x08:"Wolverine",0x0A:"Psylocke",
        0x0C:"Blackheart",0x0E:"Shuma-Gorath",0x10:"Juggernaut",0x12:"Magneto",0x14:"Doctor Doom",0x16:"Thanos"}),
 "xmvsf": dict(set="xmvsfur2", method="cycle", addr=0xFF4053, coin=2400, start=2700, crop=(0, 0, 176, 104), secs=75,
   ids={0x00:"Wolverine",0x02:"Cyclops",0x04:"Storm",0x06:"Rogue",0x08:"Gambit",0x0A:"Sabretooth",0x0C:"Juggernaut",
        0x0E:"Magneto",0x10:"Apocalypse",0x12:"Ryu",0x14:"Ken",0x16:"Chun-Li",0x18:"Dhalsim",0x1A:"Zangief",
        0x1C:"M. Bison",0x1E:"Akuma",0x20:"Charlie",0x22:"Cammy"}),
 "mshvsf": dict(set="mshvsfu1d", method="cycle", addr=0xFF3853, coin=2400, start=2700, crop=(0, 0, 124, 118), secs=80,
   ids={0x02:"Cyclops",0x04:"Captain America",0x06:"Hulk",0x08:"Wolverine",0x0A:"Shuma-Gorath",0x0C:"Omega Red",
        0x0E:"Spider-Man",0x10:"Blackheart",0x12:"Ryu",0x14:"Ken",0x16:"Chun-Li",0x18:"Dhalsim",0x1A:"Zangief",
        0x1C:"M. Bison",0x1E:"Akuma",0x20:"Sakura",0x22:"Norimaro",0x24:"Dan",0x26:"Cyber-Akuma",0x28:"Mech-Zangief",
        0x2A:"Dark Sakura",0x2E:"U.S. Agent"}),
 "mvsc": dict(set="mvscu", method="cycle", addr=0xFF3053, coin=2400, start=2700, crop=(0, 0, 118, 206), secs=80,
   ids={0x02:"War Machine",0x04:"Captain America",0x06:"Hulk",0x08:"Wolverine",0x0A:"Gambit",0x0C:"Venom",
        0x0E:"Spider-Man",0x10:"Roll",0x12:"Ryu",0x14:"Captain Commando",0x16:"Chun-Li",0x18:"Jin Saotome",
        0x1A:"Zangief",0x1C:"Strider Hiryu",0x1E:"Morrigan",0x20:"Mega Man",0x22:"Onslaught",0x24:"Hyper Venom",
        0x26:"Orange Hulk",0x28:"Gold War Machine",0x2A:"Shadow Lady",0x2C:"Lilith-Mode Morrigan"}),
 "sfexp": dict(set="sfexp", method="hold", addr=0x32358A, coin=2400, snapat=5370, crop=(0, 14, 220, 132), secs=92,
   ids={0x00:"Kairi",0x01:"Hokuto",0x02:"Darun Mister",0x03:"Doctrine Dark",0x04:"Pullum Purna",0x05:"Blair Dame",
        0x06:"Skullomania",0x07:"Allen Snider",0x08:"Cracker Jack",0x09:"Guile",0x0C:"Ryu",0x0D:"Ken",0x0E:"Chun-Li",
        0x0F:"Zangief",0x10:"Akuma",0x12:"Evil Ryu",0x13:"Bloody Hokuto",0x14:"M. Bison",0x15:"Garuda",
        0x16:"Cycloid Gamma",0x17:"Cycloid Beta"}),
 "sfex2p": dict(set="sfex2p", method="cycle", addr=0x01F010, coin=2400, start=2700, crop=(4, 0, 190, 132), secs=75,
   ids={0x01:"Doctrine Dark",0x02:"Hokuto",0x03:"Nanase",0x04:"Cracker Jack",0x05:"Rosso",0x06:"Area",
        0x07:"Pullum Purna",0x08:"Darun Mister",0x09:"Skullomania",0x0A:"Sharon",0x0B:"Guile",0x0C:"Vega",
        0x0D:"M. Bison",0x0E:"Sagat",0x0F:"Ryu",0x10:"Ken",0x11:"Chun-Li",0x12:"Zangief",0x13:"Dhalsim",0x14:"Blanka"}),
}


def slug(name):
    name = name.split(" (")[0]
    return re.sub(r"[^a-z0-9]", "", name.lower())


def read_png(path):
    d = open(path, "rb").read(); pos = 8; idat = b""; w = h = ct = 0
    while pos < len(d):
        ln = struct.unpack(">I", d[pos:pos + 4])[0]; t = d[pos + 4:pos + 8]; body = d[pos + 8:pos + 8 + ln]; pos += 12 + ln
        if t == b"IHDR": w, h, bd, ct = struct.unpack(">IIBB", body[:10])
        elif t == b"IDAT": idat += body
    bpp = 4 if ct == 6 else 3
    raw = zlib.decompress(idat); st = w * bpp; rows = []; prev = bytearray(st); i = 0
    for _ in range(h):
        f = raw[i]; line = bytearray(raw[i + 1:i + 1 + st]); i += 1 + st
        for x in range(st):
            a = line[x - bpp] if x >= bpp else 0; b = prev[x]; c = prev[x - bpp] if x >= bpp else 0
            if f == 1: line[x] = (line[x] + a) & 255
            elif f == 2: line[x] = (line[x] + b) & 255
            elif f == 3: line[x] = (line[x] + (a + b) // 2) & 255
            elif f == 4:
                p = a + b - c; pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                line[x] = (line[x] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 255
        rows.append(bytes(line[k] for k in range(st) if k % bpp < 3)); prev = line
    return w, h, rows


def write_crop(src, dst, crop):
    w, h, rows = read_png(src)
    x, y, cw, ch = crop
    cw, ch = min(cw, w - x), min(ch, h - y)
    data = b"".join(b"\x00" + rows[y + r][x * 3:(x + cw) * 3] for r in range(ch))
    chunk = lambda t, d: struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    with open(dst, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", cw, ch, 8, 2, 0, 0, 0))
                + chunk(b"IDAT", zlib.compress(data, 9)) + chunk(b"IEND", b""))


def run_mame(mame, roms, cfg, port_fields, workdir):
    os.makedirs(workdir, exist_ok=True)
    lua = os.path.join(workdir, "run.lua")
    open(lua, "w").write("PORT = {%s}\n" % port_fields + LUA)
    r = subprocess.run([mame, cfg["set"], "-rompath", roms, "-video", "none", "-sound", "none", "-nothrottle",
                        "-seconds_to_run", str(cfg["secs"]), "-skip_gameinfo", "-snapshot_directory", workdir,
                        "-autoboot_script", lua], cwd=os.path.dirname(mame), capture_output=True, text=True)
    snaps = [int(l.split()[1]) for l in r.stdout.splitlines() if l.startswith("MCL_SNAP")]
    pngs = sorted(os.path.join(workdir, cfg["set"], f) for f in os.listdir(os.path.join(workdir, cfg["set"]))) \
        if os.path.isdir(os.path.join(workdir, cfg["set"])) else []
    return list(zip(snaps, pngs))


def extract(game, cfg, mame, roms, out, tmp, jobs):
    done = 0
    if cfg["method"] == "cycle":
        ids = sorted(cfg["ids"])
        fields = "addr=%d, coin=%d, start=%d, period=40, ids={%s}" % (cfg["addr"], cfg["coin"], cfg["start"], ",".join(map(str, ids)))
        pairs = run_mame(mame, roms, cfg, fields, os.path.join(tmp, game))
    else:
        def one(i):
            fields = "addr=%d, coin=%d, snapat=%d, hold=%d" % (cfg["addr"], cfg["coin"], cfg["snapat"], i)
            return run_mame(mame, roms, cfg, fields, os.path.join(tmp, "%s_%02X" % (game, i)))
        with ThreadPoolExecutor(max_workers=jobs) as pool:
            pairs = [p for res in pool.map(one, sorted(cfg["ids"])) for p in res]
    for cid, png in pairs:
        name = cfg["ids"].get(cid)
        if not name:
            continue
        write_crop(png, os.path.join(out, slug(name), "rom_%s.png" % game), cfg["crop"])
        done += 1
    return done


def main(argv=None):
    ap = argparse.ArgumentParser(description="Extract ROM character portraits for the Gallery")
    ap.add_argument("--mame")
    ap.add_argument("--rom-list", default=os.path.join("bin", "rom_directory.txt"))
    ap.add_argument("--out", default=os.path.join("bin", "Gallery", "Art"))
    ap.add_argument("--only")
    ap.add_argument("--jobs", type=int, default=6)
    a = ap.parse_args(argv)
    mame = a.mame
    if not mame:
        cfg = os.path.join("bin", "mame_executable.txt")
        mame = next((l.strip() for l in open(cfg, encoding="utf-8") if l.strip() and not l.startswith("#")), None)
    if not mame or not os.path.isfile(mame):
        raise SystemExit("mame.exe not found; pass --mame")
    base = os.path.dirname(os.path.abspath(a.rom_list))
    roms = ";".join(l.strip() if os.path.isabs(l.strip()) else os.path.join(base, l.strip())
                    for l in open(a.rom_list, encoding="utf-8", errors="replace") if l.strip() and not l.startswith("#"))
    only = set(a.only.split(",")) if a.only else None
    total = 0
    with tempfile.TemporaryDirectory() as tmp:
        with ThreadPoolExecutor(max_workers=3) as pool:
            futs = {g: pool.submit(extract, g, c, mame, roms, a.out, tmp, a.jobs) for g, c in GAMES.items() if not only or g in only}
            for g, f in futs.items():
                n = f.result(); total += n
                print("%s: %d portraits (of %d)" % (g, n, len(GAMES[g]["ids"])))
    print("total: %d portraits -> %s" % (total, a.out))
    return 0 if total else 1


if __name__ == "__main__":
    sys.exit(main())
