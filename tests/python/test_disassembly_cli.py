"""Optional M6 CLI integration checks using the pinned private CORE."""

from pathlib import Path
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[2]
TOOL = ROOT / "build/gt4disasm.exe"
CORE = ROOT / "private/fingerprint-check/CORE.GT4"


@unittest.skipUnless(TOOL.exists() and CORE.exists(), "Requires native build and private CORE")
class DisassemblyCliTests(unittest.TestCase):
    def run_tool(self, start="0x10011c", count="1", core=CORE):
        return subprocess.run([str(TOOL), str(core), start, count], capture_output=True, text=True)

    def test_real_range_and_decimal_equivalence(self):
        hexadecimal = self.run_tool()
        decimal = self.run_tool(str(0x10011c))
        self.assertEqual(hexadecimal.returncode, 0, hexadecimal.stderr)
        self.assertEqual(decimal.returncode, 0, decimal.stderr)
        self.assertEqual(hexadecimal.stdout, decimal.stdout)
        self.assertEqual(len(hexadecimal.stdout.splitlines()), 1)
        self.assertTrue(hexadecimal.stdout.startswith("0010011c:"))
        self.assertIn("supported=1 unsupported=0", hexadecimal.stderr)

    def test_bad_numbers_and_ranges_produce_no_listing(self):
        for start, count in [("-1", "1"), ("0x", "1"), ("0x10011cgarbage", "1"),
                             ("4294967296", "1"), ("+1048860", "1"), (" 1048860", "1"),
                             ("0x10011c", "0"), ("0x10011d", "1"), ("0x10011c", "0xffffffff"),
                             ("0xffffc", "1"), ("0x617a14", "1")]:
            with self.subTest(start=start, count=count):
                result = self.run_tool(start, count)
                self.assertEqual(result.returncode, 1)
                self.assertEqual(result.stdout, "")
                self.assertIn("ERROR:", result.stderr)

    def test_unsupported_is_explicit_and_summary_counts_it(self):
        result = self.run_tool("0x100008", "2")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(len(result.stdout.splitlines()), 2)
        self.assertEqual(result.stdout.count("unsupported"), 2)
        self.assertIn("supported=0 unsupported=2", result.stderr)
        self.assertIn("opcode=0x1c function=0x28 count=2", result.stderr)

    def test_missing_input_and_missing_arguments_fail(self):
        result = self.run_tool(core=ROOT / "private/nonexistent-core")
        self.assertEqual(result.returncode, 1)
        self.assertEqual(result.stdout, "")
        result = subprocess.run([str(TOOL)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 2)
        self.assertIn("Usage:", result.stderr)


if __name__ == "__main__":
    unittest.main()
