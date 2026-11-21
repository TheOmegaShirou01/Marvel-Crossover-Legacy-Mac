"""Record each game's own coin-insert sound from the user's ROMs for achievement unlocks.

    py -3 tools/extract_unlock_sounds.py [--mame <mame.exe>] [--rom-list bin/rom_directory.txt]
                                         [--out bin/Audio/Unlock]

For every MAME game in the collection this runs MAME headless (no window, no audio
output) with -wavwrite and a tiny Lua script that presses COIN 1 once the game has
booted, then cuts the coin sound out of the recording (from its first sample until
it falls silent) and writes <out>/<game>.wav. The sound comes straight from the game's
sound hardware emulation, so it is ROM-sourced and generated per machine; the files are
never committed. MvC2 runs in Flycast and has no recorder hook, so it is not extracted:
the front end uses Marvel vs. Capcom's sound for MvC2 and collection trophies.
"""
import argparse, os, struct, subprocess, sys, tempfile, wave

SETS = [  # (collection id, MAME set, frame to press coin, seconds to record)
    ("xmcota", "xmcotau", 1200, 25), ("msh", "mshud", 1200, 25), ("xmvsf", "xmvsfur2", 1200, 25),
    ("mshvsf", "mshvsfu1d", 1200, 25), ("mvsc", "mvscu", 1200, 25),
    ("sfexp", "sfexp", 1800, 36), ("sfex2p", "sfex2p", 1800, 36),
]
LUA = r'''
local m = manager.machine
local fld
for _, port in pairs(m.ioport.ports) do
  for _, f in pairs(port.fields) do
    if f.type == m.ioport:token_to_input_type("COIN1") and f.player == 0 then fld = f end
  end
end
local fr = 0
mcl_coin = emu.add_machine_frame_notifier(function()
  fr = fr + 1
  if fld then fld:set_value((fr >= COIN_FRAME and fr < COIN_FRAME + 6) and 1 or 0) end
  if fr == COIN_FRAME then print(string.format("MCL_COIN_TIME %.4f", emu.time())) end
end)
'''


def read_roms(rom_list):
    dirs = []
    base = os.path.dirname(os.path.abspath(rom_list))
    for line in open(rom_list, encoding="utf-8", errors="replace"):
        line = line.strip()
        if line and not line.startswith("#"):
            dirs.append(line if os.path.isabs(line) else os.path.join(base, line))
    return ";".join(dirs)


def cut_coin(wav_in, coin_time, wav_out):
    w = wave.open(wav_in)
    sr, ch, n = w.getframerate(), w.getnchannels(), w.getnframes()
    data = struct.unpack("<%dh" % (n * ch), w.readframes(n))
    w.close()
    frames = [data[i:i + ch] for i in range(0, len(data), ch)]
    level = lambda f: max(abs(x) for x in f)
    start_search = int(max(0.0, coin_time - 0.05) * sr)
    # background before the coin must be quiet, or the cut would include music
    before = frames[max(0, start_search - sr // 2):start_search]
    if before and max(level(f) for f in before) > 1500:
        return "too noisy before the coin (attract music?)"
    onset = next((i for i in range(start_search, min(len(frames), start_search + 3 * sr)) if level(frames[i]) > 400), None)
    if onset is None:
        return "no coin sound found"
    onset = max(0, onset - int(0.01 * sr))
    end, quiet = onset, 0
    for i in range(onset, min(len(frames), onset + 3 * sr)):
        if level(frames[i]) > 60:
            end, quiet = i, 0
        else:
            quiet += 1
            if quiet > int(0.25 * sr):
                break
    clip = frames[onset:end + int(0.05 * sr)]
    # Some games run straight from the coin chime into a jingle: keep notifications short,
    # ending in a 0.3 s fade rather than a hard cut.
    limit, fade = int(1.6 * sr), int(0.3 * sr)
    faded = len(clip) > limit
    clip = clip[:limit]
    peak = max(level(f) for f in clip) or 1
    gain = min(4.0, 26000.0 / peak)  # even loudness across games
    def scale(i):
        g = gain
        if faded and i >= len(clip) - fade:
            g *= (len(clip) - i) / fade
        return g
    out = wave.open(wav_out, "wb")
    out.setnchannels(ch); out.setsampwidth(2); out.setframerate(sr)
    out.writeframes(b"".join(struct.pack("<%dh" % ch, *[max(-32768, min(32767, int(x * scale(i)))) for x in f]) for i, f in enumerate(clip)))
    out.close()
    return "%.2f s" % (len(clip) / sr)


def main(argv=None):
    ap = argparse.ArgumentParser(description="Extract ROM coin sounds for unlock notifications")
    ap.add_argument("--mame", help="mame.exe (default: first line of bin/mame_executable.txt)")
    ap.add_argument("--rom-list", default=os.path.join("bin", "rom_directory.txt"))
    ap.add_argument("--out", default=os.path.join("bin", "Audio", "Unlock"))
    ap.add_argument("--only", help="comma-separated collection ids")
    a = ap.parse_args(argv)
    mame = a.mame
    if not mame:
        cfg = os.path.join("bin", "mame_executable.txt")
        mame = next((l.strip() for l in open(cfg, encoding="utf-8") if l.strip() and not l.startswith("#")), None)
    if not mame or not os.path.isfile(mame):
        raise SystemExit("mame.exe not found; pass --mame")
    roms = read_roms(a.rom_list)
    os.makedirs(a.out, exist_ok=True)
    only = set(a.only.split(",")) if a.only else None
    failures = 0
    with tempfile.TemporaryDirectory() as tmp:
        for game, mset, frame, secs in SETS:
            if only and game not in only:
                continue
            lua = os.path.join(tmp, "coin.lua")
            open(lua, "w").write("COIN_FRAME = %d\n" % frame + LUA)
            rec = os.path.join(tmp, mset + ".wav")
            r = subprocess.run([mame, mset, "-rompath", roms, "-video", "none", "-sound", "none", "-nothrottle",
                                "-seconds_to_run", str(secs), "-skip_gameinfo", "-wavwrite", rec,
                                "-autoboot_script", lua], cwd=os.path.dirname(mame), capture_output=True, text=True)
            t = next((float(l.split()[1]) for l in r.stdout.splitlines() if l.startswith("MCL_COIN_TIME")), None)
            if t is None or not os.path.isfile(rec):
                print(f"{game}: MAME did not run ({r.returncode})"); failures += 1; continue
            result = cut_coin(rec, t, os.path.join(a.out, game + ".wav"))
            ok = result.endswith(" s")
            failures += 0 if ok else 1
            print(f"{game}: {result}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
