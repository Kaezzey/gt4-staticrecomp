# GT4Recomp

A learning-first, GT4-specific static recompilation project targeting C++20
and Windows x86-64. The intended result translates the selected game's R5900
code ahead of time and supplies the PS2 services that execution requires.

## Build

M3 now provides a [reference executable and layout comparison](docs/lessons/m3.md).
All three CORE payloads match, and Ghidra imports both loadable segments
byte-for-byte. Tutoring remains pending; our own reconstruction is the next
milestone. The reference has documented layout and decoding limitations.

Use an x64 Visual Studio Developer PowerShell with MSVC, the Windows SDK,
CMake (3.24+), and Ninja available. No downloaded dependencies are required.

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=cl
cmake --build build
ctest --test-dir build --output-on-failure
.\build\gt4recomp.exe
```

The test validates the build/link/test wiring only. It proves no PS2 behavior.

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
