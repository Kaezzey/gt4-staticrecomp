"""Capture the ten manually selected M6 regions; keep game-derived text private."""

import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tool", type=Path, default=Path("build/gt4disasm.exe"))
    parser.add_argument("--core", type=Path, default=Path("private/fingerprint-check/CORE.GT4"))
    parser.add_argument("--regions", type=Path, default=Path("docs/inputs/usa-v2.00-disassembly-regions.json"))
    parser.add_argument("--output", type=Path, default=Path("private/disassembly"))
    arguments = parser.parse_args()
    manifest = json.loads(arguments.regions.read_text(encoding="utf-8"))
    if hashlib.sha256(arguments.core.read_bytes()).hexdigest() != manifest["core_sha256"]:
        raise ValueError("CORE does not match the region manifest")
    arguments.output.mkdir(parents=True, exist_ok=True)
    listings = []
    reports = []
    for region in manifest["regions"]:
        result = subprocess.run(
            [str(arguments.tool.resolve()), str(arguments.core), region["start"], str(region["count"])],
            capture_output=True, text=True, check=True)
        if len(result.stdout.splitlines()) != region["count"]:
            raise ValueError(f"Incomplete listing at {region['start']}")
        listings.append(result.stdout)
        reports.append(result.stderr)
        (arguments.output / f"{region['start']}.txt").write_text(result.stdout, encoding="utf-8")
    (arguments.output / "listing.txt").write_text("".join(listings), encoding="utf-8")
    (arguments.output / "unsupported.txt").write_text("".join(reports), encoding="utf-8")
    print(f"Captured {len(listings)} regions in {arguments.output}")


if __name__ == "__main__":
    main()
