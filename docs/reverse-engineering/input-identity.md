# Selected input identity

Observed locally on 2026-09-09; no disc payload was copied or uploaded.

| Field | Value |
| --- | --- |
| Local relative path | `iso/Gran Turismo 4 (USA) (v2.00).iso` |
| Size | 5,314,478,080 bytes |
| SHA-256 | `67b6c0075837f3ae1132d608acf2858bf13b2dd62d6eae83dff76df02e4e824f` |
| Region/revision | USA / v2.00, confirmed by the owner on 2026-09-09 |
| Disc serial / boot executable | Unknown; not inspected |
| CORE.GT4 size/hash | Unknown; not extracted |
| Target selection | GT4 USA v2.00 |
| GT4Hooks compatibility | Owner reports being told USA v2.00 works with GT4Hooks; not tested locally yet |

The hash was computed once with PowerShell `Get-FileHash -Algorithm SHA256`.
A hash identifies these bytes; it does not prove their provenance, title, or
revision. No external disc database comparison has been performed.

For a manual repeat check:

```powershell
Get-FileHash -LiteralPath 'iso/Gran Turismo 4 (USA) (v2.00).iso' -Algorithm SHA256
```

Compare the full result to the recorded digest before analysis. M2 still needs
an automated mismatch check, an independently repeated result, and metadata
from the disc itself. No address-specific GT4 assumptions are established yet.
