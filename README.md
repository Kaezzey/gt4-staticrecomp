# GT4Recomp

A learning-first, GT4-specific static recompilation project targeting C++20
and Windows x86-64. The intended result translates the selected game's R5900
code ahead of time and supplies the PS2 services that execution requires.

Current scope: M0 build skeleton. No instruction decoder, game execution,
executable reconstruction, or rendering is implemented. Understanding remains
part of milestone acceptance; a passing build alone does not complete it.

Start with [requirements and roadmap](docs/requirements.md),
[environment audit](docs/environment.md), and the [M0 lesson](docs/lessons/m0.md).
The [input record](docs/reverse-engineering/input-identity.md) records the ISO
fingerprint, owner-confirmed USA v2.00 target, and reported GT4Hooks compatibility.

## Build

Use an x64 Visual Studio Developer PowerShell with MSVC, the Windows SDK,
CMake (3.24+), and Ninja available. No downloaded dependencies are required.

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=cl
cmake --build build
ctest --test-dir build --output-on-failure
.\build\gt4recomp.exe
```

The test validates the build/link/test wiring only. It proves no PS2 behavior.

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
