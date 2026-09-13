"""Compare this revision's decompressed CORE records to a reference ELF.

This is a read-only M3 experiment, not an executable reconstruction tool.
The record interpretation was informed by the tagged reference builder source.
"""

import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
import zlib


MANIFEST = Path(__file__).resolve().parents[1] / "docs/inputs/usa-v2.00.json"


def read_u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


def inspect_reference(core_path, elf_path):
    baseline = json.loads(MANIFEST.read_text(encoding="utf-8"))
    core = core_path.read_bytes()
    expected_core = baseline["files"]["/CORE.GT4;1"]
    if len(core) != expected_core["size_bytes"]:
        raise ValueError("CORE size differs from the pinned USA v2.00 input")
    if hashlib.sha256(core).hexdigest() != expected_core["sha256"]:
        raise ValueError("CORE hash differs from the pinned USA v2.00 input")

    if core[:2] != b"\x01\x01":
        raise ValueError("Unsupported CORE header")
    expected_inflated_size = read_u32(core, 2)
    decompressor = zlib.decompressobj(wbits=-15)  # Raw DEFLATE, without a zlib header.
    inflated = decompressor.decompress(core[6:])
    if not decompressor.eof or decompressor.unused_data:
        raise ValueError("Incomplete DEFLATE stream or trailing compressed data")
    if len(inflated) != expected_inflated_size:
        raise ValueError("Inflated size does not match the CORE header")

    # Skip the two length-prefixed authentication values. We do not authenticate
    # them ourselves: the external tool's authentication result is separate evidence.
    cursor = 0
    for authentication_value in range(2):
        value_size = struct.unpack_from("<H", inflated, cursor)[0]
        cursor += 2 + value_size
    record_count = read_u32(inflated, cursor)
    entry_address = read_u32(inflated, cursor + 4)
    cursor += 8
    if record_count != 3:
        raise ValueError("Expected this revision's three CORE records")

    elf = elf_path.read_bytes()
    if elf[:7] != b"\x7fELF\x01\x01\x01" or len(elf) < 52:
        raise ValueError("Expected an ELF32 little-endian reference")
    if read_u32(elf, 24) != entry_address:
        raise ValueError("ELF entry differs from CORE entry")
    program_table_offset = read_u32(elf, 28)
    header_size, header_count = struct.unpack_from("<HH", elf, 42)
    if header_size != 32 or header_count != 3:
        raise ValueError("Expected three 32-byte program headers")

    program_headers = []
    for header_index in range(header_count):
        header_offset = program_table_offset + header_index * header_size
        values = struct.unpack_from("<8I", elf, header_offset)
        field_names = ("type", "file_offset", "virtual_address", "physical_address",
                       "file_size", "memory_size", "flags", "alignment")
        program_headers.append(dict(zip(field_names, values)))

    # CORE record order is reginfo/text/data; the builder writes text/reginfo/data.
    reference_header_indices = (1, 0, 2)
    comparisons = []
    for record_index in range(record_count):
        target_address = read_u32(inflated, cursor)
        payload_size = read_u32(inflated, cursor + 4)
        payload_offset = cursor + 8
        payload_end = payload_offset + payload_size
        if payload_end > len(inflated):
            raise ValueError("CORE record exceeds the inflated body")
        header = program_headers[reference_header_indices[record_index]]
        elf_start = header["file_offset"]
        elf_end = elf_start + header["file_size"]
        if elf_end > len(elf) or header["file_size"] != payload_size:
            raise ValueError("Reference record bounds or size mismatch")
        payload = inflated[payload_offset:payload_end]
        if payload != elf[elf_start:elf_end]:
            raise ValueError(f"Payload mismatch in CORE record {record_index}")
        if record_index != 0 and target_address != header["virtual_address"]:
            raise ValueError("Reference text/data address differs from CORE")
        comparisons.append({
            "record_index": record_index,
            "inflated_payload_offset": payload_offset,
            "core_target_address": target_address,
            "size_bytes": payload_size,
            "payload_sha256": hashlib.sha256(payload).hexdigest(),
            "reference_program_header": header,
            "address_delta": header["virtual_address"] - target_address,
            "all_payload_bytes_match": True,
        })
        cursor = payload_end
    if cursor != len(inflated):
        raise ValueError("Unexplained bytes after the last CORE record")
    return {
        "entry_address": entry_address,
        "inflated_size": len(inflated),
        "inflated_sha256": hashlib.sha256(inflated).hexdigest(),
        "reference_size": len(elf),
        "reference_sha256": hashlib.sha256(elf).hexdigest(),
        "records": comparisons,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("core", type=Path)
    parser.add_argument("elf", type=Path)
    arguments = parser.parse_args()
    try:
        report = inspect_reference(arguments.core, arguments.elf)
        print(json.dumps(report, indent=2))
        return 0
    except (OSError, ValueError, struct.error, zlib.error) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
