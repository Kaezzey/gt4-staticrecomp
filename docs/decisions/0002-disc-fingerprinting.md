# 0002 — Python observation tool with an explicit baseline

Status: implemented for M2 on 2026-09-13.

Context: one selected GT4 disc needs repeatable identity checks and contained
file metadata. The project permits supporting Python analysis tools.

Decision: use standard-library SHA-256/JSON and pycdlib 1.20.0 for read-only ISO
access. Install only into ignored `private/tooling-venv/`. Check the complete
disc plus SYSTEM.CNF, its boot executable and CORE.GT4 against a versioned,
metadata-only manifest. Fail verification on mismatches; never silently repin.

Alternatives: a custom ISO filesystem parser would add a distinct learning
topic beyond M2; defer it. OS mounting would introduce mount state and host
drive letters. Manual extraction alone would make the normal identity check
depend on stale copies. Implementing SHA-256 ourselves is outside the learning
goal. No cryptographic or ISO parsing dependency is added to the C++ runtime.

Consequences: observation tools need the pinned Python package; normal C++
builds do not. Inputs are streamed rather than loaded into memory. Synthetic
ISO tests are standalone unittest tests. Filesystem parsing is not independently
validated by the PowerShell hash check, since pycdlib supplied those copies.
The original GT4 data remains local and ignored.
