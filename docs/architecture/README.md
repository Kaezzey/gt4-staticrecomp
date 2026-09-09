# Architecture notes

The intended high-level pipeline is:

```text
local GT4 input -> reconstructed loaded image -> decoded instructions
 -> control-flow analysis -> generated C++ -> host compiler -> native execution
                                                           + focused PS2 runtime
```

Static recompilation translates before execution. Decompilation additionally
tries to recover higher-level structure and intent. A test interpreter executes
decoded operations at runtime; it will be useful for validation but is not the
intended final execution path for known GT4 code.

This pipeline is a design target derived from the charter, not implemented
functionality. The next lesson will establish the EE/IOP/VU/GS responsibilities
before inspecting executable packaging. Detailed hardware claims and diagrams
will be sourced and tested as the relevant subsystem is introduced.
