#!/usr/bin/env python3
"""Read-only arcade ROM archive diagnostics for Marvel: Crossover Legacy.

No ROM/BIOS data are embedded, copied, patched or redistributed. A ZIP CRC pass
is not proof that a particular MAME version can boot a game.
"""
import argparse
import json
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path

SETS = {
    'xmcota': ('xmcota.zip',),
    'xmcotau': ('xmcotau.zip', 'xmcota.zip'),
    'msh': ('msh.zip',),
    'xmvsf': ('xmvsf.zip',),
    'mshvsf': ('mshvsf.zip',),
    'mvsc': ('mvsc.zip',),
    'mvsc2': ('mvsc2.zip', 'naomi.zip'),
    'sfexp': ('sfexp.zip', 'coh1000c.zip'),
    'sfex2p': ('sfex2p.zip', 'coh3002c.zip'),
}


def inspect_file(p, crc):
    if not p.is_file():
        return {'name': p.name, 'status': 'missing'}
    try:
        with zipfile.ZipFile(p) as z:
            names = z.namelist()
            bad = z.testzip() if crc else None
            return {'name': p.name, 'status': 'bad_crc' if bad else 'archive_readable',
                    'entries': len(names), 'security_keys': sorted(n for n in names if n.lower().endswith('.key')),
                    'crc_checked': crc, 'bad_entry': bad}
    except (zipfile.BadZipFile, OSError, RuntimeError) as exc:
        return {'name': p.name, 'status': 'invalid_zip', 'error': str(exc)}


def audit(romdir, game, crc=False, mame=None, run_mame=False, timeout=90):
    if game not in SETS:
        raise ValueError('Unknown game set: ' + game)
    files = [inspect_file(romdir / name, crc) for name in SETS[game]]
    ok = all(f['status'] == 'archive_readable' for f in files)
    notes = []
    if game == 'xmcotau' and not any(name.lower().endswith('xmcotau.key') for f in files for name in f.get('security_keys', [])):
        notes.append('xmcotau.key not found inside named game/parent archives; a separate differently named ZIP is not automatically a recognized MAME set')
    result = {'game': game, 'rom_directory': str(romdir), 'files': files, 'notes': notes,
              'archive_preflight': 'pass' if ok else 'fail',
              'mame_audit': 'not_run', 'gameplay': 'not_tested'}
    if run_mame:
        if not ok:
            result['mame_audit'] = 'skipped_archive_preflight_failed'
        elif not mame or not Path(mame).is_file():
            result['mame_audit'] = 'mame_executable_missing'
        else:
            try:
                cmd = [str(mame), '-verifyroms', game, '-rompath', str(romdir)]
                proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                      text=True, timeout=timeout, check=False)
                result['mame_audit'] = 'pass' if proc.returncode == 0 else 'fail'
                result['mame_exit_code'] = proc.returncode
                result['mame_output_tail'] = proc.stdout[-6000:]
            except subprocess.TimeoutExpired:
                result['mame_audit'] = 'timeout'
            except OSError as exc:
                result['mame_audit'] = 'launch_error'
                result['mame_error'] = str(exc)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom-dir', type=Path, required=True)
    parser.add_argument('--game', choices=sorted(SETS), default='xmcotau')
    parser.add_argument('--crc', action='store_true', help='Run full ZIP member CRC validation')
    parser.add_argument('--mame', type=Path)
    parser.add_argument('--verify-mame', action='store_true', help='Run read-only MAME ROM audit; does not start gameplay')
    parser.add_argument('--report', type=Path, help='Optional output JSON path (outside ROM directory)')
    args = parser.parse_args()
    result = audit(args.rom_dir, args.game, args.crc, args.mame, args.verify_mame)
    data = json.dumps(result, indent=2)
    print(data)
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(data + '\n', encoding='utf-8')
    return 0 if result['archive_preflight'] == 'pass' and (not args.verify_mame or result['mame_audit'] == 'pass') else 1

if __name__ == '__main__':
    sys.exit(main())
