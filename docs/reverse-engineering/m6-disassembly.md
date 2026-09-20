# M6 disassembly evidence

2026-09-20: BUILD/VERIFY passed. EXPLAIN remains pending owner-reported tutoring.

`gt4disasm` reconstructs the pinned USA v2.00 CORE with our native image reader,
then walks a caller-selected text range in four-byte steps. It formats the M5
subset plus ANDI/ORI, added after observing those instructions in startup.
Each line includes guest address, original word and operands, or an explicit
unsupported marker. The diagnostic stream reports counts by opcode family.

## Reproduce the native listing

Build using the README's Visual Studio Developer PowerShell commands, then:

```powershell
.\build\gt4disasm.exe private/fingerprint-check/CORE.GT4 0x10011c 31
python scripts/sample_disassembly.py
```

The first command prints a region to stdout and its summary to stderr. The second
captures all ten windows from the [region manifest](../inputs/usa-v2.00-disassembly-regions.json),
replacing named generated reports in `private/disassembly/`. Use `--output` for
another directory. Keep game words and assembly in ignored local output.
Numbers may be decimal or `0x` hexadecimal. Count means instructions, not bytes.
Exit 0 means the listing completed, including unsupported rows; exit 1 means an
input/range/output failure; exit 2 means incorrect argument count.

The native frontends share `tools/common/verified_core.cpp` for size/hash checks.
`gt4core` retains its original behavior. `gt4disasm` reads the original text record
directly, needing no synthetic BSS policy or intermediate ELF. The pinned M4 ELF
serves only as the independent Ghidra input.

## Selected regions and results

Selection was manual: entry/startup windows, an address constructed during
startup, four direct JAL destinations and one direct J destination. The manifest
records each selection's evidence. These are ten code inspection regions, not ten
claimed functions. A window may cross a return into neighboring code.

| Start | Words | Ghidra matches | Unsupported |
| --- | ---: | ---: | ---: |
| 0x00100008 | 69 | 0 | 69 |
| 0x0010011c | 31 | 28 | 3 |
| 0x00100198 | 20 | 16 | 4 |
| 0x001001e8 | 16 | 13 | 3 |
| 0x00100228 | 32 | 31 | 1 |
| 0x005b7560 | 64 | 51 | 13 |
| 0x005adf20 | 64 | 48 | 16 |
| 0x0048ef90 | 64 | 43 | 21 |
| 0x00107f08 | 64 | 49 | 15 |
| 0x005a3140 | 64 | 32 | 32 |
| Total | 488 | 311 | 177 |

All 311 supported words agree in mnemonic and operands, including 246 non-NOP
instructions. Zero mismatches. All 488 words were byte-checked against imported
memory. The first entry window is wholly unsupported; the actual instructions
in the other windows exceed the acceptance threshold.

The [metadata report](../inputs/usa-v2.00-disassembly.json) records hashes,
tool/language, region totals and every unsupported family count. Local
`unsupported.txt` retains counts by region; `comparison.tsv` retains each word,
our assembly, Ghidra assembly and result. Metadata contains no payload bytes.

## Independent comparison

Ghidra 12.1.3 uses `MIPS:LE:64:64-32addr`. The script checks the imported ELF's
SHA-256 and each listed word. `PseudoDisassembler` decodes one instruction at a
time so EE-only opcodes cannot prevent inspecting later base instructions.
Unsupported rows are counted, not accepted as matching instructions. Duplicate
addresses, byte mismatches, fewer than 100 non-NOP matches or any assembly
mismatch fail verification.

Normalization removes whitespace and leading hexadecimal zeroes and explicitly
expands aliases NOP, LI, BEQZ, BNEZ and B. LI expansion distinguishes ADDIU from
ORI using the primary opcode; register/immediate values still come from Ghidra.
Other differences are failures to investigate, never silently discarded.

From the repository root, use a temporary Ghidra project directory without `+`:

```powershell
New-Item -ItemType Directory -Force (Join-Path $env:TEMP 'GT4Recomp-M6') | Out-Null
$env:JAVA_HOME = 'C:\Program Files\Java\jdk-26.0.2.1'
& 'C:\Users\niket\OneDrive\Desktop\ghidra_12.1.3_PUBLIC\support\analyzeHeadless.bat' `
    (Join-Path $env:TEMP 'GT4Recomp-M6') Disassembly `
    -import "$PWD/private/reconstructed/SCUS_973.28.elf" -overwrite `
    -processor 'MIPS:LE:64:64-32addr' -noanalysis `
    -scriptPath "$PWD/scripts/ghidra" `
    -postScript CompareDisassembly.java "$PWD/private/disassembly/listing.txt" `
        "$PWD/private/disassembly/comparison.tsv" `
    -log "$PWD/private/disassembly/ghidra.log"
```

`-overwrite` replaces the imported program in this disposable project. Ghidra
needs normal AppData cache access. Check the current output for both markers:

```text
M6_COMPARISON matched=311 non_nop=246 unsupported=177 mismatched=0
M6_DISASSEMBLY_VERIFIED
```

Headless exit status alone is insufficient. The generic MIPS language does not
establish complete R5900 coverage; matching assembly proves neither execution
correctness nor that every decodable word is code.

## Implementation and validation

`src/ee/disassemble.cpp` owns register names, formatting, target display, bounded
traversal and unsupported reporting. The CLI owns strict argument parsing and
input loading. The whole range must be aligned, nonempty and inside file-backed
text before listing begins. Signed offsets use widened arithmetic and address
displays wrap within the project's 32-bit guest address model.

The common encoding/address reference is
[MIPS64 Instruction Set, Volume II, revision 0.95](https://www.ece.lsu.edu/ee4720/mips64v2.pdf):
ANDI p.45, ORI p.239, BEQ p.62, BNE p.83, and J/JAL entries. Logical immediates
print unsigned; offsets and ADDIU immediates print signed. Branch destinations
use PC+4 plus signed offset times four; jumps take upper address bits from PC+4.
M7 will model control flow and delay slots, beyond displaying destinations.

Validation: 5/5 CTest tests and 28/28 Python tests passed, including real-input
checks. Synthetic cases cover operand order, negative/unsigned immediates,
branch underflow/wrap, jump-region boundaries, invalid ranges and output failure.
CLI checks cover malformed/overflowing numbers, decimal/hex equivalence, missing
input and unsupported reporting. Existing M4 tests verify reconstruction and hash
rejection after extraction of the shared reader. Decoder fixtures now cover 37
hand-selected supported words.

Remaining scope: full EE decoding, code/data separation, function boundaries,
control-flow graphs and execution. No commits were created.
