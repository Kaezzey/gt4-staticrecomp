"""Optional local M4 integration tests. Game fixtures stay under private/."""

from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
from verify_native_image import verify_native_image

EXECUTABLE = ROOT / "build/gt4core.exe"
CORE = ROOT / "private/fingerprint-check/CORE.GT4"
REFERENCE = ROOT / "private/reference-elf/SCUS_973.28.reference.elf"


@unittest.skipUnless(all(path.exists() for path in (EXECUTABLE, CORE, REFERENCE)),
                     "Requires the native build and private M2/M3 fixtures")
class NativeCliTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.directory = Path(self.temporary.name)
        self.output = self.directory / "native.elf"

    def run_tool(self, core=CORE, output=None):
        return subprocess.run([str(EXECUTABLE), "--reference-analysis", str(core),
                               str(output or self.output)], capture_output=True, text=True)

    def build_output(self):
        result = self.run_tool()
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_real_image_and_repeatability(self):
        self.build_output()
        report = verify_native_image(CORE, REFERENCE, self.output)
        self.assertEqual(report["payload_records_matched"], 3)
        self.assertTrue(report["native_alignment_valid"])
        self.assertEqual(report["zero_fill_ranges"][0]["start"], 0x6d5dfc)
        self.assertEqual(report["zero_fill_ranges"][0]["end_exclusive"], 0xed5dfc)
        second = self.directory / "second.elf"
        self.assertEqual(self.run_tool(output=second).returncode, 0)
        self.assertEqual(self.output.read_bytes(), second.read_bytes())

    def test_existing_output_preserved(self):
        self.output.write_bytes(b"keep this file")
        self.assertEqual(self.run_tool().returncode, 1)
        self.assertEqual(self.output.read_bytes(), b"keep this file")

    def test_same_length_changed_input_rejected(self):
        changed = bytearray(CORE.read_bytes())
        changed[-1] ^= 1
        input_path = self.directory / "changed-core"
        input_path.write_bytes(changed)
        result = self.run_tool(core=input_path)
        self.assertEqual(result.returncode, 1)
        self.assertIn("SHA-256 differs", result.stderr)
        self.assertFalse(self.output.exists())

    def test_missing_input_rejected(self):
        self.assertEqual(self.run_tool(core=self.directory / "missing").returncode, 1)
        self.assertFalse(self.output.exists())

    def test_explicit_analysis_policy_required(self):
        result = subprocess.run([str(EXECUTABLE), str(CORE), str(self.output)], capture_output=True)
        self.assertEqual(result.returncode, 2)
        self.assertFalse(self.output.exists())

    def test_different_zero_fill_extent_rejected(self):
        self.build_output()
        changed = bytearray(self.output.read_bytes())
        # ELF32: third program header's p_memsz field.
        changed[52 + 2 * 32 + 20] ^= 1
        self.output.write_bytes(changed)
        with self.assertRaisesRegex(ValueError, "memory_size differs"):
            verify_native_image(CORE, REFERENCE, self.output)


if __name__ == "__main__":
    unittest.main()
