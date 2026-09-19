# 0003 — Separate parsed evidence from the reference analysis layout

Status: implemented for M4 on 2026-09-19; runtime layout remains unresolved.

Decision: C++ owns raw-DEFLATE container parsing, an ExecutableImage with the
three original records, bounds validation and an ELF32 exporter. Use hash-pinned
zlib 1.3.1 for decompression. The Windows CLI validates the selected CORE hash
with CNG before parsing. It never invokes PDTools or Python to reconstruct.

Export is explicitly named reference analysis and requires a CLI flag. It
adopts the M3 reference's 8 MiB zero-fill and reginfo shift for comparison,
without modifying original record addresses. Correct ELF alignment and omit
the optional section table. Keep permissions/flags scoped to the same policy.

Alternatives: silently copy reference BSS into the core model (rejected: lacks
runtime evidence); omit all zero-fill from comparison (rejected: would not
validate reference loaded-image semantics); invent a runtime BSS size from
nearby addresses (rejected: unsupported inference); write a DEFLATE or SHA-256
implementation (outside this milestone's learning objective); use the external
builder in production (contradicts independent reconstruction).

Consequences: native analysis output matches the defined reference image while
being a structurally different ELF. Game loading is not proven. Before runtime
execution, establish loader behavior independently and introduce a runtime
layout informed by that evidence. The analysis policy must not become a hidden
default. A section table may be added later if annotations require one.
