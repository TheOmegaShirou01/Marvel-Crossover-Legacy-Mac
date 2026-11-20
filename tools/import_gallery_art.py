"""Install the user's PS1 Marvel vs. Capcom gallery images for the in-app Gallery.

    py -3 tools/import_gallery_art.py [--zip <Marvel_vs_Capcom_PS1_Gallery.zip>] [--out bin/Gallery/Art]

The zip (ripped by the user from their own PS1 disc) holds one folder per character with
  Gallery_Artwork_Cropped.png         -> Art/<slug>/portrait.png
  Original_Gallery_Image_640x480.png  -> Art/<slug>/full.png
  Original_Gallery_Thumbnail_80x60.png-> Art/<slug>/thumb.png
plus _Bonus_Group_Artwork (-> Art/_bonus/...). <slug> is the character name in lower case
with non-letters removed (e.g. "Captain America" -> captainamerica), matching what the
front end looks up for every game the character appears in.

The images are copied unchanged (read-only access to the zip). They are the user's own
files and are never committed to the source repository. Without --zip the tool looks
for the zip under the project's parent folders.
"""
import argparse, os, re, sys, zipfile

NAME = "Marvel_vs_Capcom_PS1_Gallery.zip"
FILES = {"Gallery_Artwork_Cropped.png": "portrait.png",
         "Original_Gallery_Image_640x480.png": "full.png",
         "Original_Gallery_Thumbnail_80x60.png": "thumb.png"}


def slug(name):
    return re.sub(r"[^a-z0-9]", "", name.lower())


def find_zip(start):
    here = os.path.abspath(start)
    for _ in range(4):  # project, MCL_Work, MLC SAVE, Desktop
        for root, dirs, files in os.walk(here):
            depth = root[len(here):].count(os.sep)
            if NAME in files:
                return os.path.join(root, NAME)
            if depth >= 3:
                dirs[:] = []
            dirs[:] = [d for d in dirs if d not in ("bin", "obj", "Music", "Previews", ".git", "node_modules")]
        here = os.path.dirname(here)
    return None


def main(argv=None):
    ap = argparse.ArgumentParser(description="Install PS1 gallery art for the front end")
    ap.add_argument("--zip")
    ap.add_argument("--out", default=os.path.join("bin", "Gallery", "Art"))
    a = ap.parse_args(argv)
    zp = a.zip or find_zip(os.getcwd())
    if not zp or not os.path.isfile(zp):
        raise SystemExit(f"{NAME} not found; pass --zip <path>")
    n = 0
    with zipfile.ZipFile(zp) as z:
        for info in z.infolist():
            parts = info.filename.split("/")
            if len(parts) != 2 or parts[1] not in FILES:
                continue
            folder = parts[0]
            name = "_bonus" if folder == "_Bonus_Group_Artwork" else slug(folder.replace("_", " "))
            if folder.startswith("_") and name != "_bonus":
                continue
            dest = os.path.join(a.out, name, FILES[parts[1]])
            os.makedirs(os.path.dirname(dest), exist_ok=True)
            with z.open(info) as src, open(dest, "wb") as dst:
                dst.write(src.read())
            n += 1
    print(f"Gallery art: {n} images from {zp} -> {a.out}")


if __name__ == "__main__":
    sys.exit(main())
