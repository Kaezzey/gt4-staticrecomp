"""Synthetic container/ELF comparisons; never distribute real game fixtures."""

import hashlib
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'scripts'))
import inspect_reference


class ReferenceInspectionTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        directory = Path(temporary.name)
        self.core = directory / 'core'
        self.elf = directory / 'reference'
        self.manifest = directory / 'manifest.json'
        records = [(0x2000, b'REG!'), (0x1000, b'TEXTCODE'), (0x3000, b'DATA')]
        body = bytearray(struct.pack('<HHII', 0, 0, 3, 0x1000))
        for address, data in records:
            body.extend(struct.pack('<II', address, len(data)))
            body.extend(data)
        compressor = zlib.compressobj(wbits=-15)
        core = b'\x01\x01' + struct.pack('<I', len(body))
        core += compressor.compress(body) + compressor.flush()
        self.core.write_bytes(core)
        self.manifest.write_text(json.dumps({'files': {'/CORE.GT4;1': {
            'size_bytes': len(core), 'sha256': hashlib.sha256(core).hexdigest()
        }}}))
        elf = bytearray(512)
        elf[:7] = b'\x7fELF\x01\x01\x01'
        struct.pack_into('<II', elf, 24, 0x1000, 52)
        struct.pack_into('<HH', elf, 42, 32, 3)
        for index, record_index in enumerate((1, 0, 2)):
            address, data = records[record_index]
            offset = 256 + index * 32
            kind = 0x70000000 if record_index == 0 else 1
            struct.pack_into('<8I', elf, 52 + index * 32,
                             kind, offset, address, address, len(data), len(data), 4, 4)
            elf[offset:offset + len(data)] = data
        self.elf.write_bytes(elf)

    def inspect(self):
        with patch.object(inspect_reference, 'MANIFEST', self.manifest):
            return inspect_reference.inspect_reference(self.core, self.elf)

    def test_payload_order_and_entry(self):
        report = self.inspect()
        self.assertEqual(report['entry_address'], 0x1000)
        self.assertEqual([item['size_bytes'] for item in report['records']], [4, 8, 4])
        self.assertTrue(all(item['all_payload_bytes_match'] for item in report['records']))

    def test_changed_payload_rejected(self):
        elf = bytearray(self.elf.read_bytes())
        elf[256] ^= 1
        self.elf.write_bytes(elf)
        with self.assertRaisesRegex(ValueError, 'Payload mismatch'):
            self.inspect()

    def test_wrong_entry_rejected(self):
        elf = bytearray(self.elf.read_bytes())
        struct.pack_into('<I', elf, 24, 0x1004)
        self.elf.write_bytes(elf)
        with self.assertRaisesRegex(ValueError, 'entry differs'):
            self.inspect()

    def test_wrong_core_rejected(self):
        self.core.write_bytes(self.core.read_bytes() + b'x')
        with self.assertRaisesRegex(ValueError, 'CORE size'):
            self.inspect()

    def test_truncated_payload_rejected(self):
        self.elf.write_bytes(self.elf.read_bytes()[:258])
        with self.assertRaisesRegex(ValueError, 'bounds'):
            self.inspect()

    def test_reginfo_address_shift_reported(self):
        elf = bytearray(self.elf.read_bytes())
        struct.pack_into('<I', elf, 52 + 32 + 8, 0x2018)
        self.elf.write_bytes(elf)
        report = self.inspect()
        self.assertEqual(report['records'][0]['address_delta'], 24)


if __name__ == '__main__':
    unittest.main()
