# Selected input identity

Observed locally on 2026-09-09; no disc payload was copied or uploaded.

| Field | Value |
| --- | --- |
| Local relative path | `iso/Gran Turismo 4 (USA) (v2.00).iso` |
| Size | 5,314,478,080 bytes |
| SHA-256 | `67b6c0075837f3ae1132d608acf2858bf13b2dd62d6eae83dff76df02e4e824f` |
| Region/revision | USA / v2.00, confirmed by the owner on 2026-09-09 |
| Disc serial / boot executable | `SCUS-97328`, normalized from `SCUS_973.28` in the disc's BOOT2 field |
| CORE.GT4 size/hash | 2,020,861 bytes; `85d26aa8430154967b2633eede929286694ac39e99762527edcec365fd642ff9` |
| Target selection | GT4 USA v2.00 |
| GT4Hooks compatibility | Owner reports compatibility; the referenced ELF builder works on this CORE (M3). Hooks/injection/runtime remain untested. |

The ISO hash was first computed with PowerShell on 2026-09-09 and matched by
Python inspection and a separate verification run on 2026-09-13. Disc metadata
now confirms `VER = 2.00`, `VMODE = NTSC`, and the BOOT2 filename above.
USA remains the owner's confirmed region. No external disc database comparison
has been performed; the normalized serial comes from the boot filename, not a
physical disc-label inspection.

The versioned [manifest](../inputs/usa-v2.00.json) records all sizes and SHA-256
digests for the ISO, SYSTEM.CNF, boot executable, and CORE.GT4. Verify with:

```powershell
& '.\private\tooling-venv\Scripts\python.exe' scripts/gt4disc.py verify 'iso/Gran Turismo 4 (USA) (v2.00).iso'
```

Result on 2026-09-13: PASS for the ISO and all three contained files. PowerShell
independently matched each contained file's size and digest using local-only
copies under `private/fingerprint-check/`. Those copies and the Python virtual
environment are confirmed ignored. The manifest contains metadata only.

The ISO library performed file location/extraction for both contained-file
checks; this validates hashing independently, not two independent filesystem
parsers. M3 now provides a [reference executable and layout comparison](../lessons/m3.md).
No game-function map or full GT4Hooks runtime compatibility has been established.
