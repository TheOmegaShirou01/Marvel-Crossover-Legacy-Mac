"""Tests for tools/build_flycast_set.py using small synthetic archives (no real ROM data)."""
import os
import sys
import tempfile
import unittest
import zipfile
import zlib

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "tools"))
import build_flycast_set as b  # noqa: E402


class BuildFlycastSetTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        root = self.tmp.name
        self.orig = os.path.join(root, "orig")
        self.fixes = os.path.join(root, "fixes")
        self.out = os.path.join(root, "out")
        os.makedirs(self.orig)
        os.makedirs(self.fixes)
        with zipfile.ZipFile(os.path.join(self.orig, "mvsc2.zip"), "w") as z:
            z.writestr("epr-23085a.ic11", b"program")
            z.writestr("mpr-23084.ic32", b"corrupt chip")
            z.writestr("mvsc2-key.bin", b"KEY!")
        with zipfile.ZipFile(os.path.join(self.orig, "naomi.zip"), "w") as z:
            z.writestr("epr-21576h.ic27", b"bios")
        self.good = b"\x11" * 64
        b.KNOWN_FIXES["mvsc2"] = {"mpr-23084.ic32s": (zlib.crc32(self.good) & 0xFFFFFFFF, 64, ["mpr-23084.ic32"])}
        self.orig_bytes = open(os.path.join(self.orig, "mvsc2.zip"), "rb").read()

    def tearDown(self):
        self.tmp.cleanup()

    def test_builds_repaired_copy_and_keeps_original(self):
        open(os.path.join(self.fixes, "mpr-23084.ic32s"), "wb").write(self.good)
        out = b.build("mvsc2", self.orig, self.fixes, self.out)
        with zipfile.ZipFile(out) as z:
            names = set(z.namelist())
            self.assertEqual(names, {"epr-23085a.ic11", "mvsc2-key.bin", "mpr-23084.ic32s"})
            self.assertEqual(z.read("mpr-23084.ic32s"), self.good)
        self.assertTrue(os.path.isfile(os.path.join(self.out, "naomi.zip")))
        self.assertEqual(open(os.path.join(self.orig, "mvsc2.zip"), "rb").read(), self.orig_bytes)

    def test_rejects_wrong_chip(self):
        open(os.path.join(self.fixes, "mpr-23084.ic32s"), "wb").write(b"\x22" * 64)
        with self.assertRaises(SystemExit):
            b.build("mvsc2", self.orig, self.fixes, self.out)
        self.assertFalse(os.path.exists(os.path.join(self.out, "mvsc2.zip")))

    def test_missing_chip_and_overwrite_guard(self):
        with self.assertRaises(SystemExit):
            b.build("mvsc2", self.orig, self.fixes, self.out)
        open(os.path.join(self.fixes, "mpr-23084.ic32s"), "wb").write(self.good)
        with self.assertRaises(SystemExit):
            b.build("mvsc2", self.orig, self.fixes, self.orig)


if __name__ == "__main__":
    unittest.main()
