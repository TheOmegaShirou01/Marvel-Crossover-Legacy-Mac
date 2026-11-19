import importlib.util
import tempfile
import unittest
import zipfile
from pathlib import Path

SCRIPT = Path(__file__).resolve().parents[1] / 'rom_preflight.py'
spec = importlib.util.spec_from_file_location('rom_preflight', SCRIPT)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

class PreflightTests(unittest.TestCase):
    def test_us_clone_requires_parent(self):
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp)
            with zipfile.ZipFile(p/'xmcotau.zip', 'w') as z:
                z.writestr('xmcotau.key', b'A'*20)
            a=module.audit(p,'xmcotau',True)
            self.assertEqual(a['archive_preflight'],'fail')
            self.assertEqual(a['files'][1]['status'],'missing')
            with zipfile.ZipFile(p/'xmcota.zip','w') as z:z.writestr('dummy',b'test')
            a=module.audit(p,'xmcotau',True)
            self.assertEqual(a['archive_preflight'],'pass')
            self.assertEqual(a['files'][0]['security_keys'],['xmcotau.key'])
            self.assertEqual(a['gameplay'],'not_tested')
    def test_bios_missing(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)
            with zipfile.ZipFile(p/'mvsc2.zip','w') as z:z.writestr('data',b'x')
            self.assertEqual(module.audit(p,'mvsc2')['archive_preflight'],'fail')
            self.assertEqual(module.audit(p,'sfexp')['archive_preflight'],'fail')
    def test_invalid_zip(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp);(p/'msh.zip').write_bytes(b'not a zip')
            self.assertEqual(module.audit(p,'msh')['files'][0]['status'],'invalid_zip')
    def test_mame_not_run_by_default(self):
        with tempfile.TemporaryDirectory() as tmp:
            a=module.audit(Path(tmp),'msh',run_mame=False)
            self.assertEqual(a['mame_audit'],'not_run')
    def test_reject_ex3(self):
        with self.assertRaises(ValueError):module.audit(Path('.'),'sfex3')

if __name__=='__main__':unittest.main()
