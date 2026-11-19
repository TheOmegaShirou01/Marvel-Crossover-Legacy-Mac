"""Build a repaired NAOMI game set for Flycast without touching the original archive.

The user's mvsc2.zip has 17 good chips and one corrupt one (mpr-23084, CRC e228cdfd;
the good dump is CRC 4ebbbdd9). Put a good replacement chip in a fix folder, e.g.

    Runtime\\ROMs_extra\\mvsc2\\mpr-23084.ic32s

then run (Windows, from the project folder):

    py -3 tools\\build_flycast_set.py --game mvsc2 ^
        --original "...\\02_ROMs_and_BIOS" --fixes "Runtime\\ROMs_extra\\mvsc2" ^
        --out "Runtime\\ROMs_extra"

It writes <out>\\mvsc2.zip (original chips + verified fixes, superseded chips removed)
and copies naomi.zip beside it, which is where Flycast looks for the BIOS. The app
launches the copy because Flycast uses the LAST ROM folder that holds the game zip.

Every replacement chip is checked against the CRC32 that MAME records for it; a chip
that does not match is refused. The original folder is only ever read.
"""
import argparse
import os
import shutil
import sys
import zipfile
import zlib

# name -> (expected CRC32, size, original chip names this file replaces)
KNOWN_FIXES = {
    "mvsc2": {
        "mpr-23084.ic32s": (0x4EBBBDD9, 4194304, ["mpr-23084.ic32"]),
    },
}
BIOS = {"mvsc2": "naomi.zip"}


def crc32(data):
    return zlib.crc32(data) & 0xFFFFFFFF


def build(game, original_dir, fixes_dir, out_dir):
    if game not in KNOWN_FIXES:
        raise SystemExit(f"Unknown game '{game}'. Known: {', '.join(KNOWN_FIXES)}")
    src_zip = os.path.join(original_dir, game + ".zip")
    bios_src = os.path.join(original_dir, BIOS[game])
    out_zip = os.path.join(out_dir, game + ".zip")
    if not os.path.isfile(src_zip):
        raise SystemExit(f"Original set not found: {src_zip}")
    if not os.path.isfile(bios_src):
        raise SystemExit(f"BIOS not found: {bios_src}")
    if os.path.abspath(out_zip) == os.path.abspath(src_zip):
        raise SystemExit("Refusing to overwrite the original archive; choose a different --out folder.")

    fixes = {}
    for name, (want_crc, want_size, replaces) in KNOWN_FIXES[game].items():
        path = os.path.join(fixes_dir, name)
        if not os.path.isfile(path):
            raise SystemExit(f"Missing replacement chip: {path}")
        data = open(path, "rb").read()
        got = crc32(data)
        if len(data) != want_size or got != want_crc:
            raise SystemExit(f"{name}: CRC {got:08x} / {len(data)} bytes does not match the good dump "
                             f"{want_crc:08x} / {want_size} bytes. Not using it.")
        fixes[name] = (data, replaces)
        print(f"OK  {name}  CRC {got:08x}")

    superseded = {old for _, replaces in fixes.values() for old in replaces}
    os.makedirs(out_dir, exist_ok=True)
    tmp = out_zip + ".tmp"
    with zipfile.ZipFile(src_zip) as zin, zipfile.ZipFile(tmp, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as zout:
        for info in zin.infolist():
            if info.filename in superseded:
                print(f"--  {info.filename}  (replaced)")
                continue
            if info.filename in fixes:
                continue
            zout.writestr(info.filename, zin.read(info.filename))
        for name, (data, _) in fixes.items():
            zout.writestr(name, data)
    with zipfile.ZipFile(tmp) as check:
        bad = check.testzip()
        if bad:
            raise SystemExit(f"Written archive failed its own CRC check at {bad}")
    os.replace(tmp, out_zip)
    shutil.copy2(bios_src, os.path.join(out_dir, BIOS[game]))
    print(f"Built {out_zip} and copied {BIOS[game]} beside it.")
    return out_zip


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--game", default="mvsc2")
    ap.add_argument("--original", required=True, help="folder holding the original <game>.zip and BIOS")
    ap.add_argument("--fixes", required=True, help="folder holding verified replacement chip files")
    ap.add_argument("--out", required=True, help="folder to write the repaired <game>.zip into")
    a = ap.parse_args(argv)
    build(a.game, a.original, a.fixes, a.out)


if __name__ == "__main__":
    sys.exit(main())
