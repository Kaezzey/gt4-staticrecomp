# GT4Recomp

A learning-first, GT4-specific static recompilation project targeting C++20
and Windows x86-64. The intended result translates the selected game's R5900
code ahead of time and supplies the PS2 services that execution requires.

## Build

[M6 disassembly](docs/reverse-engineering/m6-disassembly.md) adds `gt4disasm` for
selected real GT4 ranges. Ten regions were inspected: 311 supported words match
Ghidra (246 non-NOP), while 177 remain explicitly unsupported. The decoder now
supports eighteen operations, including ANDI/ORI. All five CTest tests and 28
Python tests passed. Tutoring remains pending; see the local [M6 lesson](docs/lessons/m6.md).

[M4 native reconstruction](docs/lessons/m4.md) now reads the pinned CORE into
our own C++ executable-image model and writes an analysis ELF. All three payloads,
entry and declared memory ranges match the M3 reference policy; alignment is
corrected. Ghidra verified imported payloads and the declared zero-fill range.
Tutoring remains pending. This is analysis output, not a verified bootable port.

Use an x64 Visual Studio Developer PowerShell with MSVC, the Windows SDK,
CMake (3.24+), and Ninja available. The first configure downloads hash-pinned
zlib 1.3.1 into the ignored build directory. No PS2 runtime or reference builder
is needed by our native reconstruction path.

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=cl
cmake --build build
ctest --test-dir build --output-on-failure
.\build\gt4recomp.exe
```

For an offline configure, append
`-DGT4_ZLIB_ARCHIVE=C:/absolute/path/to/zlib-1.3.1.tar.gz` to the configure command.
The archive is still hash-checked. This workspace has a copy under
`private/dependencies/`. CTest now covers image reconstruction and ELF output as
well as instruction decoding and the original build smoke checks; no tests
establish CPU execution yet.

## Disassemble a selected region

```powershell
.\build\gt4disasm.exe private/fingerprint-check/CORE.GT4 0x10011c 31
python scripts/sample_disassembly.py
```

The count is instructions. The CLI prints the listing to stdout and unsupported
opcode counts to stderr. The script saves ten selected regions under ignored
`private/disassembly/`; it needs only standard Python. See the
[M6 evidence](docs/reverse-engineering/m6-disassembly.md) for Ghidra comparison commands.

## Reconstruct an analysis ELF

With the verified CORE copy from M2:

```powershell
New-Item -ItemType Directory -Force private/reconstructed | Out-Null
.\build\gt4core.exe --reference-analysis private/fingerprint-check/CORE.GT4 private/reconstructed/SCUS_973.28.elf
```

The output must not already exist. The explicit flag adopts the M3 builder's
unproven BSS/reginfo choices for analysis only. See the [M4 lesson](docs/lessons/m4.md)
for the implementation walkthrough, comparison commands and limitations.

## Verify the selected disc

The [M2 lesson](docs/lessons/m2.md) includes Python environment setup and the
12 standalone synthetic tests. With that environment available:

```powershell
& '.\private\tooling-venv\Scripts\python.exe' scripts/gt4disc.py verify 'iso/Gran Turismo 4 (USA) (v2.00).iso'
```

The [pinned manifest](docs/inputs/usa-v2.00.json) contains sizes, hashes and
metadata only. Verification fails on changed input and never updates it.

## Local data and source control

- `include/`, `src/`, `tools/`, `tests/`, and `docs/`: original source and notes.
- `build/`: ignored host compiler output.
- `iso/`, `private/`: ignored local inputs, extracted files, BIOS, and captures.
- `generated/`: ignored translated game code and other derivative output.

Keep sensitive inputs in the ignored directories; extensions alone cannot
identify every BIOS or extracted asset. Ignore rules do not protect files
already tracked or force-added. Review staged content before your commits.

The initial workspace was not a Git repository. Git initialization, staging,
and commits are left to the owner. No PCSX2 or PS2Recomp execution core is
included or required by this build.
