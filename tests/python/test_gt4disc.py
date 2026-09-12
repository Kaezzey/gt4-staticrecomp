"""Original synthetic fixtures only; no GT4 data needed."""

import contextlib
import copy
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
import gt4disc
import pycdlib


CONFIG = b"BOOT2 = cdrom0:\\SCUS_973.28;1\r\nVER = 2.00\r\nVMODE = NTSC\r\n"


def make_iso(path, config=CONFIG, core=True):
    iso = pycdlib.PyCdlib()
    iso.new()
    payloads = {"/SYSTEM.CNF;1": config, "/SCUS_973.28;1": b"synthetic boot"}
    if core:
        payloads["/CORE.GT4;1"] = b"synthetic core"
    # Keep streams alive until pycdlib has written the image.
    with contextlib.ExitStack() as stack:
        for name, data in payloads.items():
            stream = stack.enter_context(io.BytesIO(data))
            iso.add_fp(stream, len(data), iso_path=name)
        iso.write(str(path))
    iso.close()


class FingerprintTests(unittest.TestCase):
    def test_known_sha256_vector(self):
        self.assertEqual(gt4disc.fingerprint(io.BytesIO(b"abc")), {
            "size_bytes": 3,
            "sha256": "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
        })

    def test_chunk_boundaries_do_not_change_digest(self):
        data = b"abc" * 400000
        original = gt4disc.CHUNK_SIZE
        try:
            gt4disc.CHUNK_SIZE = 31
            small = gt4disc.fingerprint(io.BytesIO(data))
            gt4disc.CHUNK_SIZE = original
            self.assertEqual(small, gt4disc.fingerprint(io.BytesIO(data)))
        finally:
            gt4disc.CHUNK_SIZE = original

    def test_boot_metadata(self):
        self.assertEqual(gt4disc.read_config(CONFIG), {
            "boot_path": "/SCUS_973.28;1", "serial": "SCUS-97328",
            "version": "2.00", "video_mode": "NTSC",
        })

    def test_malformed_configs_rejected(self):
        for data in (b"VER = 2.00", CONFIG + b"VER=1.00", CONFIG.replace(b"cdrom0:", b"host:")):
            with self.subTest(data=data), self.assertRaises(ValueError):
                gt4disc.read_config(data)


class DiscTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.iso = self.root / "original.iso"
        self.manifest = self.root / "baseline.json"
        make_iso(self.iso)
        self.observed = gt4disc.inspect_disc(self.iso)
        self.manifest.write_text(json.dumps(self.observed), encoding="utf-8")

    def verify(self, path=None):
        return subprocess.run([
            sys.executable, str(Path(gt4disc.__file__)), "verify", str(path or self.iso),
            "--manifest", str(self.manifest),
        ], capture_output=True, text=True)

    def test_repeated_inspection_identical(self):
        self.assertEqual(self.observed, gt4disc.inspect_disc(self.iso))
        self.assertEqual(self.observed["files"]["/CORE.GT4;1"]["size_bytes"], 14)
        self.assertEqual(self.verify().returncode, 0)

    def test_rename_does_not_change_identity(self):
        renamed = self.iso.rename(self.root / "renamed.iso")
        self.assertEqual(self.verify(renamed).returncode, 0)

    def test_same_size_changed_byte_rejected_without_repinning(self):
        baseline = self.manifest.read_bytes()
        with self.iso.open("r+b") as stream:
            stream.write(b"X")  # Same length, different bytes in the system area.
        result = self.verify()
        self.assertEqual(result.returncode, 1)
        self.assertIn("disc.sha256", result.stderr)
        self.assertEqual(baseline, self.manifest.read_bytes())

    def test_truncated_iso_rejected(self):
        with self.iso.open("r+b") as stream:
            stream.truncate(4096)
        result = self.verify()
        self.assertEqual(result.returncode, 1)
        self.assertIn("disc.size_bytes", result.stderr)

    def test_wrong_contained_hash_rejected(self):
        changed = copy.deepcopy(self.observed)
        changed["files"]["/CORE.GT4;1"]["sha256"] = "0" * 64
        self.manifest.write_text(json.dumps(changed), encoding="utf-8")
        result = self.verify()
        self.assertEqual(result.returncode, 1)
        self.assertIn("/CORE.GT4;1.sha256", result.stderr)

    def test_missing_file_is_an_error(self):
        missing = self.root / "missing.iso"
        make_iso(missing, core=False)
        with self.assertRaises(pycdlib.pycdlibexception.PyCdlibException):
            gt4disc.inspect_disc(missing)

    def test_bad_schema_or_missing_baseline_entry_rejected(self):
        for change in ("schema", "file"):
            changed = copy.deepcopy(self.observed)
            if change == "schema":
                changed["schema_version"] = 999
            else:
                del changed["files"]["/CORE.GT4;1"]
            self.manifest.write_text(json.dumps(changed), encoding="utf-8")
            with self.subTest(change=change):
                self.assertEqual(self.verify().returncode, 2)

    def test_missing_iso_is_an_error(self):
        self.assertEqual(self.verify(self.root / "absent.iso").returncode, 2)


if __name__ == "__main__":
    unittest.main()
