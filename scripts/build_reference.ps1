param(
    [string] $Builder = 'private/reference-tools/GT4ElfBuilderTool-1.0.0/win-x64/PDTools.GT4ElfBuilderTool.exe',
    [string] $Core = 'private/fingerprint-check/CORE.GT4',
    [string] $Output = 'private/reference-elf/SCUS_973.28.reference.elf'
)

# Run from the repository root. This invokes an external M3 reference tool;
# it is not part of our normal C++ build or our eventual reconstruction path.
$ErrorActionPreference = 'Stop'
$manifest = Get-Content -Raw -LiteralPath 'docs/inputs/usa-v2.00.json' | ConvertFrom-Json
$expectedCore = $manifest.files.'/CORE.GT4;1'
$coreFile = Get-Item -LiteralPath $Core
$coreHash = (Get-FileHash -LiteralPath $Core -Algorithm SHA256).Hash.ToLowerInvariant()
if ($coreFile.Length -ne $expectedCore.size_bytes -or $coreHash -ne $expectedCore.sha256) {
    throw 'CORE does not match the pinned USA v2.00 input.'
}
if (Test-Path -LiteralPath $Output) {
    throw 'Output already exists. Choose a new filename for a repeat experiment.'
}
$builderPath = (Resolve-Path -LiteralPath $Builder).Path
$referenceRecord = Get-Content -Raw -LiteralPath 'docs/inputs/usa-v2.00-reference.json' | ConvertFrom-Json
$builderHash = (Get-FileHash -LiteralPath $builderPath -Algorithm SHA256).Hash.ToLowerInvariant()
if ($builderHash -ne $referenceRecord.reference_tool.executable_sha256) {
    throw 'Builder executable differs from the recorded reference tool.'
}
$corePath = $coreFile.FullName
$outputPath = [System.IO.Path]::GetFullPath($Output)
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $outputPath) | Out-Null

# The published .NET 6 tool was verified on this machine using .NET 8 roll-forward.
# Preserve the caller's setting; do not change the machine's .NET configuration.
$previousRollForward = $env:DOTNET_ROLL_FORWARD
try {
    $env:DOTNET_ROLL_FORWARD = 'Major'
    & $builderPath $corePath $outputPath | Out-File -LiteralPath ($outputPath + '.builder.log') -Encoding utf8
    if ($LASTEXITCODE -ne 0) {
        throw "Reference builder exited with code $LASTEXITCODE."
    }
} finally {
    $env:DOTNET_ROLL_FORWARD = $previousRollForward
}

# The external tool can return zero without producing output. Check independently.
if (-not (Test-Path -LiteralPath $outputPath)) {
    throw 'Reference builder produced no ELF. Inspect its local log.'
}
python scripts/inspect_reference.py $corePath $outputPath
if ($LASTEXITCODE -ne 0) {
    throw 'Reference payload comparison failed.'
}
Get-FileHash -LiteralPath $outputPath -Algorithm SHA256
