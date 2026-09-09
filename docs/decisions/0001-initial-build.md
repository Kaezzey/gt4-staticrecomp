# 0001 — Minimal C++ build laboratory

Status: implemented initial choice; revisitable.

Context: an empty source workspace and an existing Windows x64 MSVC toolchain.
The first task needs a repeatable, understandable build before PS2 semantics.

Decision: C++20, CMake 3.24+, Ninja, one static library, one CLI, and a test
executable registered with CTest. Use the installed MSVC toolchain first.

Alternatives: Visual Studio's multi-configuration generator is viable, but Ninja
keeps the documented initial Debug workflow simple. Clang is available but adds
no necessary capability to this slice. A test framework can be added once actual
test cases justify it; no dependency is needed for the initial linkage check.

Consequences: no dependency downloads; small code surface to explain. The library
currently exposes project identity only. This does not decide CPU-state design,
renderer API, scheduling, IOP service modeling, or snapshot format.
