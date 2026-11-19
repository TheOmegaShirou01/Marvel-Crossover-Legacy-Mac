#!/usr/bin/env python3
import sys,hashlib,json,zipfile
from pathlib import Path
r=Path(__file__).resolve().parent
m=json.loads((r/'rom_manifest_v0.3.6.json').read_text())
failed=[]
for item in m['files']:
 p=r/'ROMs'/item['filename']
 if not p.is_file():failed.append(f"MISSING: {p}");continue
 if p.stat().st_size!=item['size']:failed.append(f"SIZE: {p.name}");continue
 h=hashlib.sha256()
 with p.open('rb') as f:
  for block in iter(lambda:f.read(4*1024*1024),b''):h.update(block)
 if h.hexdigest()!=item['sha256']:failed.append(f"SHA256: {p.name}");continue
 try:
  with zipfile.ZipFile(p) as z:
   if z.testzip() is not None:failed.append(f"CRC: {p.name}")
 except Exception as e:failed.append(f"ZIP: {p.name}: {e}")
for item in failed:print(item)
print(f"Verified {len(m['files'])-len(failed)}/{len(m['files'])} ROM/BIOS ZIP archives")
sys.exit(1 if failed else 0)
