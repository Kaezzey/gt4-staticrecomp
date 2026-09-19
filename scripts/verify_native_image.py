"""Verify native M4 output against the pinned M3 artifact and CORE payloads."""

import argparse
import hashlib
import json
from pathlib import Path
import sys
import struct
import zlib

from inspect_reference import inspect_reference


REFERENCE_RECORD = Path(__file__).resolve().parents[1] / "docs/inputs/usa-v2.00-reference.json"


def verify_native_image(core_path, reference_path, native_path):
    pinned_reference = json.loads(REFERENCE_RECORD.read_text(encoding="utf-8"))
    reference_hash = hashlib.sha256(reference_path.read_bytes()).hexdigest()
    if reference_hash != pinned_reference["reference_sha256"]:
        raise ValueError("Reference ELF differs from the pinned M3 artifact")

    # Each inspection compares every payload byte with independently inflated CORE.
    reference = inspect_reference(core_path, reference_path)
    native = inspect_reference(core_path, native_path)
    if reference["entry_address"] != native["entry_address"]:
        raise ValueError("Entry addresses differ")

    checked_fields = ("type", "virtual_address", "physical_address", "file_size", "memory_size", "flags")
    zero_fill_ranges = []
    for expected_record, actual_record in zip(reference["records"], native["records"]):
        expected_header = expected_record["reference_program_header"]
        actual_header = actual_record["reference_program_header"]
        for field in checked_fields:
            if expected_header[field] != actual_header[field]:
                raise ValueError(f"Record {actual_record['record_index']}: {field} differs")
        alignment = actual_header["alignment"]
        if alignment < 1 or alignment & (alignment - 1):
            raise ValueError("Native segment alignment is not a positive power of two")
        if actual_header["file_offset"] % alignment != actual_header["virtual_address"] % alignment:
            raise ValueError("Native segment violates ELF alignment congruence")
        if actual_header["memory_size"] < actual_header["file_size"]:
            raise ValueError("Native memory size is smaller than its file-backed payload")
        if actual_header["type"] == 1 and actual_header["memory_size"] > actual_header["file_size"]:
            zero_fill_ranges.append({
                "start": actual_header["virtual_address"] + actual_header["file_size"],
                "end_exclusive": actual_header["virtual_address"] + actual_header["memory_size"],
                "evidence": "reference analysis policy; real runtime extent unverified",
            })

    return {
        "entry_address": native["entry_address"],
        "native_elf_size": native["reference_size"],
        "native_elf_sha256": native["reference_sha256"],
        "reference_elf_sha256": reference_hash,
        "payload_records_matched": len(native["records"]),
        "loaded_layout_matches_reference_policy": True,
        "native_alignment_valid": True,
        "zero_fill_ranges": zero_fill_ranges,
        "records": native["records"],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("core", type=Path)
    parser.add_argument("reference_elf", type=Path)
    parser.add_argument("native_elf", type=Path)
    arguments = parser.parse_args()
    try:
        result = verify_native_image(arguments.core, arguments.reference_elf, arguments.native_elf)
        print(json.dumps(result, indent=2))
        return 0
    except (OSError, ValueError, struct.error, zlib.error) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
