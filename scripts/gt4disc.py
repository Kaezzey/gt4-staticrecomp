"""Inspect or verify the selected GT4 disc; never write game data or a baseline."""

import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

import pycdlib


CHUNK_SIZE = 1024 * 1024
DEFAULT_MANIFEST = Path(__file__).resolve().parents[1] / "docs/inputs/usa-v2.00.json"


def fingerprint(stream):
    """Hash bytes from the current position through EOF using bounded memory."""
    digest = hashlib.sha256()
    size = 0
    while chunk := stream.read(CHUNK_SIZE):
        digest.update(chunk)
        size += len(chunk)
    return {"size_bytes": size, "sha256": digest.hexdigest()}


def read_config(data):
    fields = {}
    for line in data.decode("ascii").splitlines():
        if not line.strip():
            continue
        key, separator, value = line.partition("=")
        key, value = key.strip(), value.strip()
        if not separator or key in fields:
            raise ValueError("Malformed or duplicate SYSTEM.CNF field")
        fields[key] = value
    if not {"BOOT2", "VER", "VMODE"} <= fields.keys():
        raise ValueError("SYSTEM.CNF needs BOOT2, VER and VMODE")
    # Deliberately support only this project's root-level PS2 boot filename form.
    match = re.fullmatch(r"cdrom0:\\([A-Z]{4}_[0-9]{3}\.[0-9]{2});1", fields["BOOT2"])
    if not match:
        raise ValueError("Unsupported BOOT2 path: " + fields["BOOT2"])
    executable = match[1]
    return {
        "boot_path": "/" + executable + ";1",
        "serial": executable[:4] + "-" + executable[5:].replace(".", ""),
        "version": fields["VER"],
        "video_mode": fields["VMODE"],
    }


def inspect_disc(path, disc_identity=None):
    if disc_identity is None:
        with path.open("rb") as stream:
            disc_identity = fingerprint(stream)
    iso = pycdlib.PyCdlib()
    iso.open(str(path))
    try:
        with iso.open_file_from_iso(iso_path="/SYSTEM.CNF;1") as stream:
            config_data = stream.read(4097)
        if len(config_data) > 4096:
            raise ValueError("SYSTEM.CNF exceeds the supported 4096-byte limit")
        config = read_config(config_data)
        files = {}
        for name in sorted({"/SYSTEM.CNF;1", "/CORE.GT4;1", config["boot_path"]}):
            with iso.open_file_from_iso(iso_path=name) as stream:
                files[name] = fingerprint(stream)
        return {"schema_version": 1, "disc": disc_identity,
                "system": config, "files": files}
    finally:
        iso.close()


def validate_identity(value):
    if not isinstance(value, dict) or set(value) != {"size_bytes", "sha256"}:
        raise ValueError("Invalid fingerprint fields")
    if type(value["size_bytes"]) is not int or value["size_bytes"] < 0:
        raise ValueError("Invalid fingerprint size")
    if not isinstance(value["sha256"], str) or not re.fullmatch("[0-9a-f]{64}", value["sha256"]):
        raise ValueError("Invalid SHA-256 digest")


def validate_manifest(value):
    if not isinstance(value, dict) or set(value) != {"schema_version", "disc", "system", "files"}:
        raise ValueError("Invalid manifest fields")
    if type(value["schema_version"]) is not int or value["schema_version"] != 1:
        raise ValueError("Unsupported manifest schema")
    validate_identity(value["disc"])
    system = value["system"]
    if not isinstance(system, dict) or set(system) != {"boot_path", "serial", "version", "video_mode"}:
        raise ValueError("Invalid system metadata")
    if not all(isinstance(item, str) and item for item in system.values()):
        raise ValueError("System metadata must contain nonempty strings")
    if not re.fullmatch(r"/[A-Z]{4}_[0-9]{3}\.[0-9]{2};1", system["boot_path"]):
        raise ValueError("Invalid manifest boot path")
    files = value["files"]
    if not isinstance(files, dict) or set(files) != {"/SYSTEM.CNF;1", "/CORE.GT4;1", system["boot_path"]}:
        raise ValueError("Manifest must fingerprint SYSTEM.CNF, CORE.GT4 and the boot executable")
    for identity in files.values():
        validate_identity(identity)


def differences(expected, actual, location="manifest"):
    result = []
    for key in sorted(expected.keys() | actual.keys()):
        label = f"{location}.{key}"
        if key not in expected or key not in actual:
            result.append(f"{label}: missing or unexpected field")
        elif isinstance(expected[key], dict) and isinstance(actual[key], dict):
            result.extend(differences(expected[key], actual[key], label))
        elif expected[key] != actual[key]:
            result.append(f"{label}: expected {expected[key]!r}, observed {actual[key]!r}")
    return result


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    inspect = commands.add_parser("inspect", help="Print observed metadata as JSON; writes no files")
    inspect.add_argument("iso", type=Path)
    verify = commands.add_parser("verify", help="Compare against a read-only baseline; fail on mismatch")
    verify.add_argument("iso", type=Path)
    verify.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    args = parser.parse_args(argv)
    try:
        if args.command == "inspect":
            print(json.dumps(inspect_disc(args.iso), indent=2, sort_keys=True))
            return 0
        expected = json.loads(args.manifest.read_text(encoding="utf-8"))
        validate_manifest(expected)
        with args.iso.open("rb") as stream:
            disc = fingerprint(stream)
        mismatches = differences(expected["disc"], disc, "disc")
        if not mismatches:
            actual = inspect_disc(args.iso, disc)
            mismatches = differences(expected, actual)
        if mismatches:
            print("FAIL: input differs from the pinned manifest", file=sys.stderr)
            print("\n".join(mismatches), file=sys.stderr)
            return 1
        print(f"PASS: {expected['system']['serial']} / VER {expected['system']['version']}; "
              "ISO and all three file fingerprints match")
        return 0
    except (OSError, ValueError, pycdlib.pycdlibexception.PyCdlibException) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
