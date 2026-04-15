param(
    [string]$RepoRoot = "",
    [string]$SpecPath = "",
    [string]$OutputRegistryPath = "",
    [string]$OutputTypecheckMapPath = ""
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

if ([string]::IsNullOrWhiteSpace($OutputRegistryPath)) {
    $OutputRegistryPath = Join-Path $RepoRoot "cursive\\src\\00_core\\generated\\diag_registry.inc"
}

if ([string]::IsNullOrWhiteSpace($OutputTypecheckMapPath)) {
    $OutputTypecheckMapPath = Join-Path $RepoRoot "cursive\\src\\04_analysis\\typing\\item\\typecheck_diag_map.inc"
}

function Normalize-Cell([string]$value) {
    $v = $value.Trim()
    if ($v.StartsWith('`') -and $v.EndsWith('`') -and $v.Length -ge 2 -and
        (-not $v.Substring(1, $v.Length - 2).Contains('`'))) {
        $v = $v.Substring(1, $v.Length - 2)
    }
    return $v.Trim()
}

function Escape-CppString([string]$value) {
    return ($value -replace "\\", "\\\\" -replace '"', '\"')
}

function Read-ExistingDiagMap {
    param([string[]]$Paths)

    $map = @{}
    $entryPattern = [regex]'\{"(?<diag>[^"]+)",\s*"(?<code>[EWIP]-[A-Z]{3}-[0-9]{4})"\}'
    foreach ($path in $Paths) {
        if (-not (Test-Path $path)) {
            continue
        }
        $text = Get-Content -Path $path -Raw
        foreach ($m in $entryPattern.Matches($text)) {
            $diag = [string]$m.Groups['diag'].Value
            $code = [string]$m.Groups['code'].Value
            if (-not $map.ContainsKey($diag)) {
                $map[$diag] = $code
            }
        }
    }
    return $map
}

if (-not (Test-Path $SpecPath)) {
    throw "Spec file not found: $SpecPath"
}

$diagCodePattern = [regex]'^[EWIP]-[A-Z]{3}-[0-9]{4}$'
$diagIdRefPattern = [regex]'Code\((?<id>[A-Za-z][A-Za-z0-9\-]*)\)'

$rowsByCode = @{}
$lines = Get-Content -Path $SpecPath
$insideDiagTable = $false
$skipNextSeparator = $false

foreach ($line in $lines) {
    if (-not $insideDiagTable) {
        if ($line -match '^\|\s*Code\s*\|\s*Severity\s*\|\s*Detection\s*\|\s*Condition\s*\|\s*$') {
            $insideDiagTable = $true
            $skipNextSeparator = $true
        }
        continue
    }

    if ($skipNextSeparator) {
        $skipNextSeparator = $false
        continue
    }

    if ($line -notmatch '^\s*\|') {
        $insideDiagTable = $false
        continue
    }

    $parts = $line -split '\|'
    if ($parts.Count -lt 5) {
        continue
    }

    $code = Normalize-Cell $parts[1]
    if (-not $diagCodePattern.IsMatch($code)) {
        continue
    }

    $severity = Normalize-Cell $parts[2]
    $condition = Normalize-Cell $parts[4]

    $rowsByCode[$code] = [PSCustomObject]@{
        code = $code
        severity = $severity
        condition = $condition
    }
}

if ($rowsByCode.Count -eq 0) {
    throw "No diagnostic rows were parsed from $SpecPath."
}

$mapByDiagId = @{}
foreach ($code in $rowsByCode.Keys) {
    $mapByDiagId[$code] = $code
}

$existingMap = Read-ExistingDiagMap -Paths @($OutputRegistryPath, $OutputTypecheckMapPath)
foreach ($diagId in $existingMap.Keys) {
    $code = [string]$existingMap[$diagId]
    if ($rowsByCode.ContainsKey($code) -and -not $mapByDiagId.ContainsKey($diagId)) {
        $mapByDiagId[$diagId] = $code
    }
}

$explicitMap = @{
    "If-Branch-Mismatch" = "E-MOD-2402"
    "IfCase-Branch-Mismatch" = "E-MOD-2402"
    "IfCase-Enum-NonExhaustive" = "E-SEM-2741"
    "IfCase-Modal-NonExhaustive" = "E-TYP-2060"
    "IfCase-NonExhaustive" = "E-SEM-2741"
    "IfCase-Unreachable" = "E-SEM-2751"
    "IfCase-Union-NonExhaustive" = "E-SEM-2705"
}

$removedDiagIds = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
foreach ($diagId in @(
                        )) {
    [void]$removedDiagIds.Add($diagId)
}

$specText = Get-Content -Path $SpecPath -Raw
$specDiagIds = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
foreach ($m in $diagIdRefPattern.Matches($specText)) {
    [void]$specDiagIds.Add([string]$m.Groups['id'].Value)
}

foreach ($diagId in $explicitMap.Keys) {
    $code = [string]$explicitMap[$diagId]
    if (-not $rowsByCode.ContainsKey($code)) {
        throw "Explicit diagnostic mapping references unknown code: $diagId -> $code"
    }
    if ($specDiagIds.Contains($diagId) -or $diagId.StartsWith("If", [System.StringComparison]::Ordinal)) {
        $mapByDiagId[$diagId] = $code
    }
}

foreach ($removed in $removedDiagIds) {
    if ($mapByDiagId.ContainsKey($removed)) {
        $mapByDiagId.Remove($removed)
    }
}

$orderedRows = @($rowsByCode.Values | Sort-Object code)
$orderedMap = @($mapByDiagId.GetEnumerator() | Sort-Object Name)

$registryDir = Split-Path -Parent $OutputRegistryPath
$typecheckDir = Split-Path -Parent $OutputTypecheckMapPath
New-Item -ItemType Directory -Path $registryDir -Force | Out-Null
New-Item -ItemType Directory -Path $typecheckDir -Force | Out-Null

$utf8NoBom = New-Object System.Text.UTF8Encoding($false)

$registry = New-Object System.Text.StringBuilder
[void]$registry.AppendLine("// ===========================================================================")
[void]$registry.AppendLine("// diag_registry.inc - Diagnostic Registry (generated from canonical language spec)")
[void]$registry.AppendLine("// ===========================================================================")
[void]$registry.AppendLine("// AUTO-GENERATED. Do not edit manually.")
[void]$registry.AppendLine("// Source: canonical language specification")
[void]$registry.AppendLine("// ===========================================================================")
[void]$registry.AppendLine("")
[void]$registry.AppendLine("struct DiagRegistryRow {")
[void]$registry.AppendLine("  const char* code;")
[void]$registry.AppendLine("  const char* severity;")
[void]$registry.AppendLine("  const char* condition;")
[void]$registry.AppendLine("};")
[void]$registry.AppendLine("")
[void]$registry.AppendLine("struct DiagIdCodeEntry {")
[void]$registry.AppendLine("  const char* diag_id;")
[void]$registry.AppendLine("  const char* code;")
[void]$registry.AppendLine("};")
[void]$registry.AppendLine("")
[void]$registry.AppendLine("static constexpr DiagRegistryRow kDiagRegistryRows[] = {")
foreach ($row in $orderedRows) {
    $code = Escape-CppString $row.code
    $severity = Escape-CppString $row.severity
    $condition = Escape-CppString $row.condition
    [void]$registry.AppendLine("  {`"$code`", `"$severity`", `"$condition`"},")
}
[void]$registry.AppendLine("};")
[void]$registry.AppendLine("")
[void]$registry.AppendLine("static constexpr DiagIdCodeEntry kDiagIdCodeMapEntries[] = {")
foreach ($entry in $orderedMap) {
    $diagId = Escape-CppString ([string]$entry.Name)
    $code = Escape-CppString ([string]$entry.Value)
    [void]$registry.AppendLine("  {`"$diagId`", `"$code`"},")
}
[void]$registry.AppendLine("};")
[System.IO.File]::WriteAllText($OutputRegistryPath, $registry.ToString(), $utf8NoBom)

$typecheck = New-Object System.Text.StringBuilder
[void]$typecheck.AppendLine("// ===========================================================================")
[void]$typecheck.AppendLine("// typecheck_diag_map.inc - Typecheck Diagnostic ID to Code Mapping")
[void]$typecheck.AppendLine("// ===========================================================================")
[void]$typecheck.AppendLine("// AUTO-GENERATED from the canonical language specification. Do not edit manually.")
[void]$typecheck.AppendLine("// ===========================================================================")
[void]$typecheck.AppendLine("")
[void]$typecheck.AppendLine("static const DiagMapEntry kTypecheckDiagMap[] = {")
foreach ($entry in $orderedMap) {
    $diagId = Escape-CppString ([string]$entry.Name)
    $code = Escape-CppString ([string]$entry.Value)
    [void]$typecheck.AppendLine("  {`"$diagId`", `"$code`"},")
}
[void]$typecheck.AppendLine("  {nullptr, nullptr},")
[void]$typecheck.AppendLine("};")
[System.IO.File]::WriteAllText($OutputTypecheckMapPath, $typecheck.ToString(), $utf8NoBom)

Write-Host "[diag-registry] rows=$($orderedRows.Count) map_entries=$($orderedMap.Count)"
