"""Extract Marvel vs. Capcom's own 8x8 font from the user's mvsc.zip (read-only).

Writes an RGBA atlas: 16 columns x 4 rows of 8x8 glyphs for ASCII 0x20-0x5F
(space, punctuation, 0-9, A-Z), coloured with MvC's boot-screen palette: a dark blue
outline and a pale-pink-to-deep-red gradient. Lower-case text is drawn with the capitals.

How it was found (2026-11-19): the MvC boot screen writes "MARVEL VS CAPCOM" to the
scroll1 layer with tile codes equal to ASCII (M=0x4D, A=0x41...); scroll1 tile code c
lives at CPS-2 graphics offset (0x20000 + c) * 64 bytes. The graphics ROMs mvc.13m/
15m/17m/19m are word-interleaved and stored shuffled in 2 MB banks (MAME's
cps2_gfx_decode "unshuffle"). Pen 15 is transparent. Colours were measured from a MAME
snapshot of the same screen (exact match on all 1024 pixels).

The atlas is derived from the ROM, so it is generated per machine and never committed.

    py -3 tools/extract_mvc_font.py --roms <folder with mvsc.zip> [--roms <folder> ...] --out bin/Fonts/mvc_font.png
    py -3 tools/extract_mvc_font.py --rom-list bin/rom_directory.txt --out bin/Fonts/mvc_font.png
"""
import argparse, os, struct, sys, zipfile, zlib

BANK = 0x200000
FONT_BASE = 0x20000
PALETTE = {  # pen -> RGB (measured); pen 15 transparent, pen 1 unused by the glyphs
    0: (0x00, 0x22, 0x88), 1: (0x00, 0x22, 0x88), 2: (0xFF, 0xAA, 0xCC), 3: (0xFF, 0x88, 0xAA),
    4: (0xFF, 0x66, 0x88), 5: (0xFF, 0x22, 0x22), 6: (0xDD, 0x11, 0x11), 7: (0xCC, 0x00, 0x00),
}
GFX = ("mvc.13m", "mvc.15m", "mvc.17m", "mvc.19m")


def unshuffle(units, lo, n):
    if n == 2:
        return
    half = n // 2
    unshuffle(units, lo, half)
    unshuffle(units, lo + half, half)
    q = half // 2
    for i in range(q):
        a, b = lo + q + i, lo + half + i
        units[a], units[b] = units[b], units[a]


def load_bank(roms, index):
    part = slice(index * (BANK // 4), (index + 1) * (BANK // 4))
    raw = bytearray(BANK)
    for r in range(4):
        src = roms[r][part]
        raw[r * 2::8] = src[0::2]
        raw[r * 2 + 1::8] = src[1::2]
    units = [bytes(raw[i:i + 8]) for i in range(0, BANK, 8)]
    unshuffle(units, 0, len(units))
    return b"".join(units)


def glyph(bank, offset_in_bank):
    rows = []
    for y in range(8):
        row = bank[offset_in_bank + y * 8: offset_in_bank + y * 8 + 4]
        line = []
        for x in range(8):
            v = 0
            for p, off in enumerate((24, 16, 8, 0)):
                bit = off + x
                if row[bit // 8] & (0x80 >> (bit % 8)):
                    v |= 1 << (3 - p)
            line.append(v)
        rows.append(line)
    return rows


def write_png(path, w, h, rgba):
    raw = b"".join(b"\x00" + bytes(rgba[y * w * 4:(y + 1) * w * 4]) for y in range(h))
    def chunk(t, d):
        return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)
    png = (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
           + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))
    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
    with open(path, "wb") as f:
        f.write(png)


def find_zip(dirs):
    for d in dirs:
        p = os.path.join(d, "mvsc.zip")
        if os.path.isfile(p):
            return p
    return None


def extract(zip_path, out):
    with zipfile.ZipFile(zip_path) as z:
        names = set(z.namelist())
        missing = [n for n in GFX if n not in names]
        if missing:
            raise SystemExit(f"{zip_path} lacks graphics ROMs: {', '.join(missing)}")
        roms = [z.read(n) for n in GFX]
    first = (FONT_BASE + 0x20) * 64
    bank_index = first // BANK
    bank = load_bank(roms, bank_index)
    W, H = 16 * 8, 4 * 8
    rgba = bytearray(W * H * 4)
    for i in range(64):  # ASCII 0x20..0x5F
        off = (FONT_BASE + 0x20 + i) * 64 - bank_index * BANK
        g = glyph(bank, off)
        gx, gy = (i % 16) * 8, (i // 16) * 8
        for y in range(8):
            for x in range(8):
                v = g[y][x]
                if v == 15:
                    continue
                r, gg, b = PALETTE.get(v, (0xFF, 0xFF, 0xFF))
                k = ((gy + y) * W + gx + x) * 4
                rgba[k:k + 4] = bytes((r, gg, b, 255))
    write_png(out, W, H, rgba)
    # sanity: 'A' must have pixels, space must be empty
    a_off = ((0x41 - 0x20) // 16 * 8 * W + (0x41 - 0x20) % 16 * 8) * 4
    if not any(rgba[a_off + 3 + 4 * x] for x in range(8)) and not any(
            rgba[a_off + 3 + 4 * (W * y + x)] for y in range(8) for x in range(8)):
        raise SystemExit("Font glyphs look empty; is this the expected MvC ROM set?")
    print(f"MvC font extracted from {zip_path} -> {out}")


def main(argv=None):
    ap = argparse.ArgumentParser(description="Extract the Marvel vs. Capcom font from mvsc.zip")
    ap.add_argument("--roms", action="append", default=[], help="folder that may contain mvsc.zip (repeatable)")
    ap.add_argument("--rom-list", help="text file with one ROM folder per line (bin/rom_directory.txt)")
    ap.add_argument("--out", required=True)
    a = ap.parse_args(argv)
    dirs = list(a.roms)
    if a.rom_list and os.path.isfile(a.rom_list):
        base = os.path.dirname(os.path.abspath(a.rom_list))
        for line in open(a.rom_list, encoding="utf-8", errors="replace"):
            line = line.strip()
            if line and not line.startswith("#"):
                dirs.append(line if os.path.isabs(line) else os.path.join(base, line))
    zip_path = find_zip(dirs)
    if not zip_path:
        raise SystemExit("mvsc.zip not found in: " + "; ".join(dirs))
    extract(zip_path, a.out)


if __name__ == "__main__":
    sys.exit(main())
