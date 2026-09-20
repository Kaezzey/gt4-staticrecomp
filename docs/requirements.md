# Requirements extracted from the supplied charter

Source: `GT4_STATIC_RECOMP_LEARNING_CHARTER.md`, supplied on 2026-09-09.
This is a project-specification summary, not an imported agent instruction file.
The current request is to establish requirements and get started; the owner
explicitly retains responsibility for all commits.

## Intended outcomes and boundaries

1. Learning takes priority over speed and boot progress. Explain a new mechanism,
   predict an observable result, implement a small slice, compare evidence, and
   preserve a worked explanation. Track BUILD, VERIFY, and EXPLAIN separately.
2. Initially support exactly one fingerprinted GT4 disc revision on Windows
   x86-64 with C++20. Filename labels do not establish revision identity.
3. Independently implement executable reconstruction, decoding, control-flow
   analysis, ahead-of-time C++ generation, and a focused runtime. A general PS2
   emulator or a turnkey recompilation project is outside the intended scope.
4. PCSX2 is an observation/reference tool, not an embedded execution engine.
   PS2Recomp is not a dependency or generator for normal project output.
5. Keep game payloads, BIOS, captures containing game bytes, and translated game
   code in ignored local directories. Public fixtures should be original synthetic
   examples or independently reviewed distributable metadata.
6. Represent guest state explicitly: fixed-width integers, controlled guest
   addresses, explicit endian/alignment rules, deterministic tests, and no C++
   undefined behavior as a substitute for guest semantics.
7. Unsupported instructions/services stop with useful context. Any diagnostic
   stub must be documented; returning success cannot establish correctness.
8. Implement hardware and services in response to observed GT4 use. Delay GPU
   backend choices and performance optimization until reference behavior exists.

## First sequence and acceptance evidence

| Milestone | Artifact and evidence | Current state |
| --- | --- | --- |
| M0 | Core library, CLI, CMake/CTest, ignore rules, documentation; build and test from a fresh build directory | Accepted 2026-09-13: build verified; owner reports Luna tutoring passed |
| M1 | PS2 architecture lesson; explain EE, IOP, VU, GS and data movement | Guide prepared; Luna tutoring in progress; owner authorized M2 concurrently |
| M2 | One revision record, file sizes and SHA-256; repeatable comparison rejects changed input | BUILD/VERIFY passed 2026-09-13: manifest, verifier, 12 tests and real-input comparisons; EXPLAIN pending |
| M3 | Reference reconstruction from the selected CORE; map file bytes to loaded addresses and inspect independently | BUILD/VERIFY passed: repeatable ELF, 3/3 payloads match, Ghidra byte import verified; EXPLAIN pending; reference layout caveats recorded |
| M4 | Own image reconstruction; compare loaded bytes, addresses, zero-fill, entry point | BUILD/VERIFY passed 2026-09-19 for explicit reference analysis policy: native C++, payload/layout comparisons and Ghidra zero-fill verification; EXPLAIN pending; runtime BSS remains unresolved |
| M5 | Small instruction decoder with explicit unsupported results | BUILD/VERIFY passed 2026-09-19: 16 operations, 33 hand-selected words, negative/unsupported cases and independent Ghidra comparisons; EXPLAIN pending |
| M6 | Native disassembler, ten real regions, independent comparisons and unsupported report | BUILD/VERIFY passed 2026-09-20: 488 words, 311 Ghidra matches (246 non-NOP), 177 unsupported, zero mismatches; EXPLAIN pending |
| M7-M8 | Basic blocks, control-flow graph and evidence-backed function map | Pending |
| M9-M12 | Guest state/memory, small test interpreter, generated straight-line and branching synthetic programs | Pending |
| M13 | One real GT4 function compiled natively; at least five valid input states match relevant registers, touched memory, writes and continuation | First major technical landmark |

After M13: automate snapshots (M14); expand COP1/MMI/VU0 (M15-M17);
continuous execution and observed OS/file/scheduling/IOP services (M18-M23);
Adhoc inspection/execution lessons (M24-M25); DMA/VIF/VU/GIF/GS and verified
pixels (M26-M34); input/audio/menu/car/track/race milestones (M35-M41).
These are an evidence-driven curriculum, not an estimated delivery schedule.

## Questions to resolve with evidence

- Target is USA v2.00, confirmed by the owner and SYSTEM.CNF VER metadata.
  Serial SCUS-97328 and input hashes are recorded in `docs/inputs/usa-v2.00.json`.
  Do not transplant Online US addresses.
- PDTools elfbuilder-1.0.0 reconstructs this CORE; full GT4Hooks injection/runtime
  compatibility is untested. M4 explicitly scopes synthetic BSS/reginfo to an
  opt-in analysis policy and fixes ELF alignment. Runtime initialization still
  requires original-loader or dynamic evidence.
- Verify an R5900-capable Ghidra analysis setup using sample encodings;
  generic MIPS support alone does not prove EE extension coverage.
- Locate/install observation tools and validate PCSX2 register/memory capture
  on this machine. Record emulator version, settings, and capture boundary.
- Choose a tractable real function only after examining the actual image.
- Investigate relocated, dynamically loaded, or modified executable code;
  static discovery cannot assume every executable address is known initially.
- Decide the Adhoc runtime boundary later: executing the recompiled original
  VM and replacing it with a separate VM are different designs. The learning
  exercise does not by itself require replacing the game's VM in production.
- PCSX2 is an implementation oracle, not proof of perfect hardware accuracy.
  Separate manual requirements, observed behavior, and our hypotheses.

## Evidence convention

Use Confirmed, High confidence, Hypothesis, or Unknown. Each discovery records
input hash, guest address/file offset as applicable, tool/version, observed
state, hypothesis, comparison and next experiment. Compare all relevant state,
not only return values. A self-written interpreter and generator can share a
bug, so independent observations remain necessary.

No external implementation source was copied or used to design the skeleton.
Public documentation was consulted for tool roles and compatibility caveats;
see [references](references/README.md).
