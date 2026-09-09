# Environment audit — 2026-09-09

| Component | Observed result |
| --- | --- |
| Git | 2.51.0.windows.1; workspace initially had no `.git` repository |
| CMake | 4.1.2 |
| Ninja | 1.13.2 |
| MSVC | 19.44.35215, x64, VS 2022 Build Tools |
| LLVM | `clang++` and `clang-cl` found; not selected/tested |
| Python | 3.14.3; optional for analysis, not needed for M0 |
| Java | Oracle JDK 26.0.2.1, 64-bit; explicit `java -version` succeeded; not on PATH |
| Ghidra | 12.1.3 confirmed from installation properties at the owner-supplied location |
| PCSX2 | 2.8.2.0 confirmed from executable metadata at `C:/Program Files/PCSX2/pcsx2-qt.exe` |
| ImHex | Not located in initial PATH/common-directory audit; installation unknown |

No applications were installed. The audit did not search every drive.
MSVC is selected for the first build because an x64 toolchain is already
available. Use Developer PowerShell so compiler, linker, and Windows SDK paths
are configured together.

Ghidra's official prebuilt-install instructions currently specify a 64-bit
JDK 21; its source-build requirements differ. Verify the selected release's
instructions when installing rather than freezing the charter's release number.
See [official installation instructions](https://github.com/NationalSecurityAgency/ghidra#install).

Ghidra launcher: `C:/Users/niket/OneDrive/Desktop/ghidra_12.1.3_PUBLIC/ghidraRun.bat`.
Java executable: `C:/Program Files/Java/jdk-26.0.2.1/bin/java.exe`.
The installed Ghidra properties specify Java minimum 21 and no maximum; launch
compatibility with this JDK has not been tested in this session. PCSX2 debugger
capture is likewise not yet functionally validated. ImHex remains unlocated.

Build validation: fresh CMake configure, Ninja build, and CTest succeeded with
MSVC x64; 2/2 tests passed. The first configure failed with `kernel32.lib` missing
because the shell lacked a complete SDK library environment. Loading Visual
Studio Developer PowerShell and reconfiguring with `cmake --fresh` resolved it.
No source changes or installations were required to fix that environment issue.

If the normal shell has the compiler but lacks the SDK paths, initialize it:

```powershell
Import-Module 'C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/Tools/Microsoft.VisualStudio.DevShell.dll'
Enter-VsDevShell -VsInstallPath 'C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools' -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64'
```

Then use the README build commands. If a failed configure already created a
cache, add `--fresh` to the configure command once.
