# Reference register

Checked 2026-09-09. These are documentation references, not build dependencies.

| Primary source | Supported observation | Limitation / next check |
| --- | --- | --- |
| [GT4Hooks README](https://github.com/Nenkai/GT4Hooks) | Documents an ELF reconstruction workflow using GT4ElfBuilderTool | Explicitly targets Online US; validate retail compatibility before using |
| [Ghidra official repository](https://github.com/NationalSecurityAgency/ghidra#install) | Disassembly/analysis tooling; prebuilt install requires JDK 21 64-bit | Confirm R5900 extensions and actual installed version |
| [PCSX2 debugger](https://pcsx2.net/docs/advanced/debugger/) | Register/memory/disassembly views, breakpoints and analysis | Capture workflow and reproducibility remain to be tested locally |
| [PS2SDK repository](https://github.com/ps2dev/ps2sdk) | Separate EE and IOP software-interface reference | Homebrew conventions are not automatically GT4 runtime contracts |

The charter also lists PS2Recomp, OpenAdhoc, ImHex and a MOH recompilation case
study. Their implementation/completeness claims have not been validated in
this session and are not assumptions behind the initial build.

Before implementing R5900 semantics, locate an appropriate architecture manual
and record edition/page references alongside independent instruction experiments.
Keep architectural requirements distinct from emulator implementation choices.
