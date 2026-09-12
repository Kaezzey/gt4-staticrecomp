# Persistent project instructions

## Human-readable code

Explicit owner requirement, recorded 2026-09-13:

All code written for this project must be HUMAN READABLE. Always assume a
human will read, review, and learn from it. Never justify obscure or compressed
code by assuming it will not be inspected by a person.

Use descriptive names, straightforward control flow, and clear formatting.
Prefer explicit, understandable steps over clever expressions or unnecessary
abstractions. Explain non-obvious intent and assumptions where they matter.
This requirement also applies to generated C++ and project tooling scripts.

The owner's exception permits assuming that test files and test scripts may
not be read by a human. This does not extend to general project scripts.

## Commits

The owner creates commits. Do not create commits on their behalf.
