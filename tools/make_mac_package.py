#!/usr/bin/env python3
"""Builds the Mac test package on Windows.

Takes the app GitHub built (MCL-mac-arm64.tar.gz: front end + MAME + Flycast, no content)
and adds the game content from this PC inside the app (Contents/Resources): Audio, Fonts,
Gallery, Logos, Music and the ROM/BIOS folders listed in bin/rom_directory.txt (copied
unchanged, repaired sets taking precedence, like tools/make_package.ps1).

The tar is read directly into the zip (never unpacked on Windows), so the Mac file
permissions and symbolic links inside Flycast.app survive. The zip stores Unix modes, which
macOS Archive Utility and `unzip` restore.

  py -3 tools/make_mac_package.py --app-tar <MCL-mac-arm64.tar.gz> --out <folder>
"""
import argparse, os, stat, sys, tarfile, time, zipfile

APP = "Marvel Crossover Legacy.app"
CONTENT = ["Audio", "Fonts", "Gallery", "Logos"]          # from bin/
STORE = {".zip", ".mp3", ".png", ".jpg", ".jpeg", ".webp", ".mp4", ".7z", ".gz", ".icns", ".chd"}

README = """MARVEL: CROSSOVER LEGACY - MAC TEST BUILD {version} (Apple Silicon: M1/M2/M3/M4)

1. Unzip this file (double-click it) and drag "Marvel Crossover Legacy" into Applications.

2. This test build is not signed by Apple, so macOS will refuse to open it at first.
   Open Terminal (Applications > Utilities > Terminal), paste this line and press Return:

     xattr -cr "/Applications/Marvel Crossover Legacy.app"

   Then open the game normally (double-click). You only need to do this once per new build.

3. Keyboard: arrows move, Enter selects, Esc goes back, Cmd+Enter switches window/fullscreen.
   Controllers (Xbox, PlayStation, Switch Pro) work in the menus and games.

Saves, settings and logs are kept in:
  ~/Library/Application Support/Marvel Crossover Legacy
(Logs/ there is the folder to send back if something goes wrong.)

Not in this test build yet: the moving Earth background and card preview videos, in-game
updates, and the Marvel vs. Capcom 2 pause screen / cheats / 1-button moves.
"""

def zinfo(name, mode, mtime):
    zi = zipfile.ZipInfo(name, time.localtime(max(mtime, 315532800))[:6])
    zi.create_system = 3                      # Unix: macOS reads the mode from here
    zi.external_attr = (mode & 0xFFFF) << 16
    ext = os.path.splitext(name)[1].lower()
    zi.compress_type = zipfile.ZIP_STORED if ext in STORE or stat.S_ISLNK(mode) else zipfile.ZIP_DEFLATED
    return zi

def add_tree(z, src, arc, count):
    for root, dirs, files in os.walk(src):
        dirs.sort(); files.sort()
        rel = os.path.relpath(root, src)
        base = arc if rel == "." else arc + "/" + rel.replace(os.sep, "/")
        z.writestr(zinfo(base + "/", stat.S_IFDIR | 0o755, os.path.getmtime(root)), b"")
        for f in files:
            p = os.path.join(root, f)
            zi = zinfo(base + "/" + f, stat.S_IFREG | 0o644, os.path.getmtime(p))
            with open(p, "rb") as fin, z.open(zi, "w", force_zip64=True) as fout:
                while True:
                    b = fin.read(1 << 20)
                    if not b: break
                    fout.write(b)
            count[0] += 1

def rom_dirs(bin_dir):
    out = []
    with open(os.path.join(bin_dir, "rom_directory.txt"), encoding="utf-8-sig") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"): continue
            p = line if os.path.isabs(line) else os.path.join(bin_dir, line)
            if os.path.isdir(p): out.append(os.path.normpath(p))
    return out

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--app-tar", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--project", default=os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    ap.add_argument("--version", default="0.4.5-mac-test1")
    ap.add_argument("--no-roms", action="store_true", help="leave the ROMs out (for a quick structure test)")
    a = ap.parse_args()
    proj = a.project; bin_dir = os.path.join(proj, "bin")
    os.makedirs(a.out, exist_ok=True)
    zpath = os.path.join(a.out, f"Marvel_Crossover_Legacy_Mac_{a.version}_arm64.zip")
    res = APP + "/Contents/Resources"
    count = [0]
    with zipfile.ZipFile(zpath, "w", allowZip64=True) as z:
        # 1. the app from GitHub, exactly as built (modes + symlinks)
        with tarfile.open(a.app_tar, "r:gz") as t:
            for m in t.getmembers():
                name = m.name[2:] if m.name.startswith("./") else m.name
                if not name.startswith(APP): continue
                if m.isdir():
                    z.writestr(zinfo(name.rstrip("/") + "/", stat.S_IFDIR | (m.mode & 0o777 or 0o755), m.mtime), b"")
                elif m.issym():
                    z.writestr(zinfo(name, stat.S_IFLNK | 0o755, m.mtime), m.linkname.encode("utf-8"))
                elif m.isfile():
                    zi = zinfo(name, stat.S_IFREG | (m.mode & 0o777), m.mtime)
                    with t.extractfile(m) as fin, z.open(zi, "w", force_zip64=True) as fout:
                        while True:
                            b = fin.read(1 << 20)
                            if not b: break
                            fout.write(b)
                count[0] += 1
        # 2. content
        for d in CONTENT:
            src = os.path.join(bin_dir, d)
            if os.path.isdir(src): add_tree(z, src, res + "/" + d, count)
            else: print("WARNING: missing", src)
        music = os.path.join(proj, "Music")
        if os.path.isdir(music): add_tree(z, music, res + "/Music", count)
        # 3. ROMs: first folder = ROMs, others = ROMs/extra, ROMs/extra2 ... (repaired sets win)
        targets = []
        if not a.no_roms:
            dirs = rom_dirs(bin_dir)
            later = set()
            for d in dirs[1:]:
                later |= {f for f in os.listdir(d) if f.lower().endswith(".zip")}
            for i, d in enumerate(dirs):
                name = "ROMs" if i == 0 else "ROMs/extra" + (str(i) if i > 1 else "")
                targets.append(name)
                for root, ds, files in os.walk(d):
                    ds.sort(); files.sort()
                    rel = os.path.relpath(root, d)
                    base = res + "/" + name + ("" if rel == "." else "/" + rel.replace(os.sep, "/"))
                    z.writestr(zinfo(base + "/", stat.S_IFDIR | 0o755, os.path.getmtime(root)), b"")
                    for f in files:
                        if i == 0 and rel == "." and (f in later or f.endswith(").zip")): continue  # replaced / duplicate copies
                        p = os.path.join(root, f)
                        zi =zinfo(base + "/" + f, stat.S_IFREG | 0o644, os.path.getmtime(p))
                        with open(p, "rb") as fin, z.open(zi, "w", force_zip64=True) as fout:
                            while True:
                                b = fin.read(1 << 20)
                                if not b: break
                                fout.write(b)
                        count[0] += 1
        rom_txt = "# ROM/BIOS folders, relative to this file\n" + "".join(t + "\n" for t in targets)
        z.writestr(zinfo(res + "/rom_directory.txt", stat.S_IFREG | 0o644, time.time()), rom_txt)
        z.writestr(zinfo("READ ME FIRST.txt", stat.S_IFREG | 0o644, time.time()), README.format(version=a.version))
    print(f"{count[0]} files -> {zpath} ({os.path.getsize(zpath)/2**30:.2f} GB)")

if __name__ == "__main__":
    sys.exit(main())
