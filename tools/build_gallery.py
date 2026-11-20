"""Build Gallery/arcade_gallery.mclgallery from the editable source text in data/gallery.

    py -3 tools/build_gallery.py [--src data/gallery] [--out Gallery/arcade_gallery.mclgallery]

Sources:
  games.txt         Arcade Museum: one [id] block per game (title, year, hardware,
                    developer, history, mechanics, sources)
  roster.tsv        every character as they appear in each game (id, game, name, role,
                    one-line note, sources)
  encyclopedia.txt  Character Encyclopedia: one [Name] block per character
                    (universe, bio, story); every roster name must have one

Format MCLGAL02 (read by Collection::loadGalleryPackage in src/core.cpp):
  "MCLGAL02", u32 record count, then per record: u16 type, u32 field count, and
  u32-length-prefixed UTF-8 fields; finally the SHA-256 of everything before it.
  type 1 = game (8 fields), 2 = roster entry (7 fields), 3 = encyclopedia entry (4 fields).
Text only: no images, ROM data or audio go into the package.
"""
import argparse, hashlib, os, re, struct, sys

GAMES = ["xmcota", "msh", "xmvsf", "mshvsf", "mvsc", "mvsc2", "sfexp", "sfex2p"]


def blocks(path):
    """[key] blocks of 'field: value' lines; '#' lines are comments."""
    out, key, cur = [], None, {}
    for raw in open(path, encoding="utf-8"):
        line = raw.rstrip("\n")
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        m = re.match(r"^\[(.+)\]$", line)
        if m:
            if key is not None:
                out.append((key, cur))
            key, cur = m.group(1), {}
            continue
        if key is None or ":" not in line:
            raise SystemExit(f"{path}: unexpected line: {line[:60]}")
        f, v = line.split(":", 1)
        cur[f.strip()] = v.strip()
    if key is not None:
        out.append((key, cur))
    return out


def unescape(s):
    return s.replace("\\n", "\n").replace("\\t", "\t").replace("\\\\", "\\")


def build(src, out):
    games = blocks(os.path.join(src, "games.txt"))
    ids = [k for k, _ in games]
    if ids != GAMES:
        raise SystemExit(f"games.txt must list exactly {GAMES} in order, got {ids}")
    roster = []
    for line in open(os.path.join(src, "roster.tsv"), encoding="utf-8"):
        line = line.rstrip("\n")
        if not line or line.startswith("#"):
            continue
        cols = line.split("\t")
        if len(cols) != 6:
            raise SystemExit(f"roster.tsv: expected 6 columns: {line[:60]}")
        roster.append([unescape(c) for c in cols])
    ency = blocks(os.path.join(src, "encyclopedia.txt"))
    names = {k for k, _ in ency}
    missing = sorted({r[2] for r in roster} - names)
    if missing:
        raise SystemExit("encyclopedia.txt lacks: " + ", ".join(missing))
    for k, b in ency:
        for f in ("universe", "bio", "story"):
            if not b.get(f):
                raise SystemExit(f"encyclopedia.txt [{k}] has no {f}")

    records = []
    for k, b in games:
        records.append((1, [k, b["title"], b["year"], b["hardware"], b["developer"],
                            b["history"], b["mechanics"], b.get("sources", "")]))
    for cid, game, name, role, note, sources in roster:
        if game not in GAMES or not re.fullmatch(r"[A-Za-z0-9_-]+", cid):
            raise SystemExit(f"roster.tsv: bad id/game {cid} {game}")
        records.append((2, [cid, game, name, note, role, sources, ""]))
    for k, b in ency:
        records.append((3, [k, b["universe"], b["bio"], b["story"]]))

    body = bytearray(b"MCLGAL02")
    body += struct.pack("<I", len(records))
    for typ, fields in records:
        body += struct.pack("<HI", typ, len(fields))
        for f in fields:
            data = f.encode("utf-8")
            body += struct.pack("<I", len(data)) + data
    body += hashlib.sha256(body).digest()
    os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
    with open(out, "wb") as f:
        f.write(body)
    print(f"Gallery package: {len(games)} games, {len(roster)} roster entries, "
          f"{len(ency)} encyclopedia entries -> {out} ({len(body)} bytes)")


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--src", default="data/gallery")
    ap.add_argument("--out", default="Gallery/arcade_gallery.mclgallery")
    a = ap.parse_args(argv)
    build(a.src, a.out)


if __name__ == "__main__":
    sys.exit(main())
