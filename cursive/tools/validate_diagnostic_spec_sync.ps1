param(
    [string]$RepoRoot = "",
    [string]$SpecPath = "",
    [string]$RegistryPath = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\\..")).Path
}

if ([string]::IsNullOrWhiteSpace($SpecPath)) {
    $resolveSpecPath = Join-Path $PSScriptRoot "resolve_spec_path.ps1"
    $SpecPath = (& $resolveSpecPath -RepoRoot $RepoRoot)
    if ([string]::IsNullOrWhiteSpace($SpecPath)) {
        throw "Unable to resolve canonical language spec path."
    }
}

if ([string]::IsNullOrWhiteSpace($RegistryPath)) {
    $RegistryPath = Join-Path $RepoRoot "cursive\\src\\00_core\\generated\\diag_registry.inc"
}

if (-not (Test-Path $SpecPath)) {
    throw "Spec file not found: $SpecPath"
}
if (-not (Test-Path $RegistryPath)) {
    throw "Generated registry not found: $RegistryPath"
}

$codePattern = [regex]'[EWIP]-[A-Z]{3}-[0-9]{4}'

$specCodes = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
$sourceCodes = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)

$specText = Get-Content -Path $SpecPath -Raw
foreach ($m in $codePattern.Matches($specText)) {
    [void]$specCodes.Add($m.Value)
}

# Detect active compiler diagnostic emissions instead of scanning arbitrary comments.
$srcRoot = Join-Path $RepoRoot "cursive\\src"
$patterns = @(
    'MakeDiagnostic\("(?<code>[EWIP]-[A-Z]{3}-[0-9]{4})"',
    'MakeDiagnosticById\("(?<code>[EWIP]-[A-Z]{3}-[0-9]{4})"',
    '\.code\s*=\s*"(?<code>[EWIP]-[A-Z]{3}-[0-9]{4})"',
    'error_code\.empty\(\)\s*\?\s*"(?<code>[EWIP]-[A-Z]{3}-[0-9]{4})"',
    'code\.empty\(\)\s*\?\s*"(?<code>[EWIP]-[A-Z]{3}-[0-9]{4})"'
)

$sourceFiles = Get-ChildItem -Path $srcRoot -Recurse -File -Include *.cpp,*.h,*.hpp,*.inc
foreach ($file in $sourceFiles) {
    $text = Get-Content -Path $file.FullName -Raw
    foreach ($pattern in $patterns) {
        $regex = [regex]$pattern
        foreach ($match in $regex.Matches($text)) {
            if ($match.Groups["code"].Success) {
                [void]$sourceCodes.Add($match.Groups["code"].Value)
            }
        }
    }
}

$missingInSpec = New-Object System.Collections.Generic.List[string]
foreach ($code in $sourceCodes) {
    if (-not $specCodes.Contains($code)) {
        $missingInSpec.Add($code)
    }
}

if ($missingInSpec.Count -gt 0) {
    $ordered = @($missingInSpec | Sort-Object)
    Write-Host "[diag-sync] FAIL: active compiler diagnostics missing from canonical language spec:"
    foreach ($code in $ordered) {
        Write-Host "  - $code"
    }
    throw "Diagnostic spec sync validation failed. Update the canonical language spec diagnostic tables."
}

$registryText = Get-Content -Path $RegistryPath -Raw
$registryRows = ([regex]::Matches($registryText, '\{"[EWIP]-[A-Z]{3}-[0-9]{4}",')).Count
if ($registryRows -eq 0) {
    throw "Generated registry appears empty: $RegistryPath"
}

Write-Host "[diag-sync] PASS: source_codes=$($sourceCodes.Count) spec_codes=$($specCodes.Count) registry_rows=$registryRows"
