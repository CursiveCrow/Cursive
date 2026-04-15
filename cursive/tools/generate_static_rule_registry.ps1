param(
    [Parameter(Mandatory = $true)]
    [string]$RepoRoot,
    [string]$SpecPath = "",
    [Parameter(Mandatory = $true)]
    [string]$MappingPath,
    [Parameter(Mandatory = $true)]
    [string]$OutputPath,
    [string]$ReportPath = "",
    [switch]$Strict
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Normalize-RelPath([string]$BasePath, [string]$Path) {
    $base = [System.IO.Path]::GetFullPath($BasePath)
    $full = [System.IO.Path]::GetFullPath($Path)
    $baseNorm = $base.TrimEnd([char[]]@([char]92, [char]47))
    $uriBase = New-Object System.Uri(($baseNorm + [System.IO.Path]::DirectorySeparatorChar))
    $uriPath = New-Object System.Uri($full)
    $rel = $uriBase.MakeRelativeUri($uriPath).ToString()
    return $rel.Replace('\\', '/')
}

function Escape-CppString([string]$Value) {
    return $Value.Replace('\\', '\\\\').Replace('"', '\\"').Replace("`r", '\r').Replace("`n", '\n')
}

if (-not (Test-Path -LiteralPath $RepoRoot)) {
    throw "RepoRoot not found: $RepoRoot"
}
if ([string]::IsNullOrWhiteSpace($SpecPath)) {
    $resolveSpecPath = Join-Path $PSScriptRoot "resolve_spec_path.ps1"
    $SpecPath = (& $resolveSpecPath -RepoRoot $RepoRoot)
    if ([string]::IsNullOrWhiteSpace($SpecPath)) {
        throw "Unable to resolve canonical language spec path."
    }
}
if (-not (Test-Path -LiteralPath $SpecPath)) {
    throw "SpecPath not found: $SpecPath"
}
if (-not (Test-Path -LiteralPath $MappingPath)) {
    throw "MappingPath not found: $MappingPath"
}

$mapping = (Get-Content -LiteralPath $MappingPath -Raw) | ConvertFrom-Json
$defaultFamily = [string]$mapping.default_family
if ([string]::IsNullOrWhiteSpace($defaultFamily)) {
    throw "Mapping file missing default_family"
}

$pathFamilyDefaults = @()
foreach ($entry in $mapping.path_family_defaults) {
    $pathFamilyDefaults += [PSCustomObject]@{
        Regex  = [regex]([string]$entry.regex)
        Family = [string]$entry.family
    }
}

$familyOverrides = @{}
if ($mapping.rule_family_overrides) {
    foreach ($p in $mapping.rule_family_overrides.PSObject.Properties) {
        $familyOverrides[[string]$p.Name] = [string]$p.Value
    }
}

$diagOverrides = @{}
if ($mapping.rule_diag_overrides) {
    foreach ($p in $mapping.rule_diag_overrides.PSObject.Properties) {
        $diagOverrides[[string]$p.Name] = [string]$p.Value
    }
}

$ruleSourceOverrides = @{}
if ($mapping.rule_source_overrides) {
    foreach ($p in $mapping.rule_source_overrides.PSObject.Properties) {
        $ruleSourceOverrides[[string]$p.Name] = [string]$p.Value
    }
}

function Resolve-RuleFamily([string]$RuleId, [string]$SourceRel) {
    if ($familyOverrides.ContainsKey($RuleId)) {
        return $familyOverrides[$RuleId]
    }
    foreach ($map in $pathFamilyDefaults) {
        if ($map.Regex.IsMatch($SourceRel)) {
            return $map.Family
        }
    }
    return $defaultFamily
}

function Get-SpecRulePremises {
    param([string]$Path)

    $ruleHeaderPattern = [regex]'^\*\*\(([^)]+)\)\*\*$'
    $ruleBarPattern = [regex]'^[─-]{3,}$'
    $premiseSplitPattern = [regex]'\s{4,}'
    $lines = Get-Content -LiteralPath $Path
    $premisesByRule = @{}
    $index = 0

    while ($index -lt $lines.Count) {
        $match = $ruleHeaderPattern.Match($lines[$index].Trim())
        if (-not $match.Success) {
            $index += 1
            continue
        }

        $ruleId = [string]$match.Groups[1].Value.Trim()
        $index += 1
        $premiseItems = New-Object System.Collections.Generic.List[string]

        while ($index -lt $lines.Count) {
            $line = [string]$lines[$index]
            $trimmed = $line.Trim()
            if ([string]::IsNullOrWhiteSpace($trimmed)) {
                $index += 1
                continue
            }
            if ($ruleBarPattern.IsMatch($trimmed)) {
                break
            }
            foreach ($part in $premiseSplitPattern.Split($trimmed)) {
                $premise = $part.Trim()
                if (-not [string]::IsNullOrWhiteSpace($premise)) {
                    $premiseItems.Add($premise) | Out-Null
                }
            }
            $index += 1
        }

        $premisesByRule[$ruleId] = @($premiseItems)
    }

    return $premisesByRule
}

$sourceRoot = Join-Path $RepoRoot "cursive/src"
if (-not (Test-Path -LiteralPath $sourceRoot)) {
    throw "Source root not found: $sourceRoot"
}
$premisesByRule = Get-SpecRulePremises -Path $SpecPath

$files = Get-ChildItem -LiteralPath $sourceRoot -Recurse -File |
    Where-Object { $_.Extension -eq ".cpp" -or $_.Extension -eq ".h" }

$rulePattern = [regex]'SPEC_RULE(?:_AT)?\("([^"]+)"\)'
$diagCodePattern = [regex]'^[EWI]-[A-Z]{3}-[0-9]{4}$'

$ruleToEntry = @{}
$ruleToSources = @{}
$familyConflicts = New-Object System.Collections.Generic.List[object]
$unmappedRules = New-Object System.Collections.Generic.List[string]

foreach ($file in $files) {
    $content = Get-Content -LiteralPath $file.FullName -Raw
    $matches = $rulePattern.Matches($content)
    if ($matches.Count -eq 0) {
        continue
    }

    $sourceRel = Normalize-RelPath -BasePath $sourceRoot -Path $file.FullName

    foreach ($m in $matches) {
        $ruleId = [string]$m.Groups[1].Value

        $family = Resolve-RuleFamily -RuleId $ruleId -SourceRel $sourceRel
        if ([string]::IsNullOrWhiteSpace($family)) {
            $unmappedRules.Add($ruleId)
            continue
        }

        $diagId = $null
        if ($diagOverrides.ContainsKey($ruleId)) {
            $diagId = $diagOverrides[$ruleId]
        } elseif ($diagCodePattern.IsMatch($ruleId)) {
            $diagId = $ruleId
        }

        if ($ruleToEntry.ContainsKey($ruleId)) {
            if (-not $ruleToSources.ContainsKey($ruleId)) {
                $ruleToSources[$ruleId] = New-Object System.Collections.Generic.List[string]
            }
            if (-not $ruleToSources[$ruleId].Contains($sourceRel)) {
                $ruleToSources[$ruleId].Add($sourceRel)
            }
            continue
        }

        $ruleToEntry[$ruleId] = [PSCustomObject]@{
            rule_id = $ruleId
            conclusion_family = $family
            diag_id = $diagId
            source_path = $sourceRel
            premises_text = $(if ($premisesByRule.ContainsKey($ruleId)) { (@($premisesByRule[$ruleId]) -join "`n") } else { $null })
        }
        $ruleToSources[$ruleId] = New-Object System.Collections.Generic.List[string]
        $ruleToSources[$ruleId].Add($sourceRel)
    }
}

$invalidSourceOverrides = New-Object System.Collections.Generic.List[string]
$appliedSourceOverrides = @{}
foreach ($ruleId in $ruleSourceOverrides.Keys) {
    $preferredSource = $ruleSourceOverrides[$ruleId]
    if (-not $ruleToEntry.ContainsKey($ruleId)) {
        $invalidSourceOverrides.Add("${ruleId}:missing-rule")
        continue
    }
    if (-not $ruleToSources.ContainsKey($ruleId) -or -not $ruleToSources[$ruleId].Contains($preferredSource)) {
        $invalidSourceOverrides.Add("${ruleId}:missing-source:${preferredSource}")
        continue
    }

    $entry = $ruleToEntry[$ruleId]
    if ($entry.source_path -ne $preferredSource) {
        $entry = [PSCustomObject]@{
            rule_id = $entry.rule_id
            conclusion_family = (Resolve-RuleFamily -RuleId $ruleId -SourceRel $preferredSource)
            diag_id = $entry.diag_id
            source_path = $preferredSource
            premises_text = $entry.premises_text
        }
        $ruleToEntry[$ruleId] = $entry
    }
    $appliedSourceOverrides[$ruleId] = $preferredSource
}

$sortedRules = $ruleToEntry.Keys | Sort-Object
$outputDir = Split-Path -Parent $OutputPath
if (-not [string]::IsNullOrWhiteSpace($outputDir)) {
    New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
}
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)

$sb = New-Object System.Text.StringBuilder
$null = $sb.AppendLine("// Auto-generated by cursive/tools/generate_static_rule_registry.ps1")
$null = $sb.AppendLine("// DO NOT EDIT MANUALLY.")
$null = $sb.AppendLine("static const StaticRuleMeta kStaticRules[] = {")
foreach ($ruleId in $sortedRules) {
    $entry = $ruleToEntry[$ruleId]
    $rid = Escape-CppString $entry.rule_id
    $fam = Escape-CppString $entry.conclusion_family
    $src = Escape-CppString $entry.source_path
    $diagField = "std::nullopt"
    if ($entry.diag_id) {
        $diag = Escape-CppString ([string]$entry.diag_id)
        $diagField = ('std::string_view("{0}")' -f $diag)
    }
    $premisesField = "std::nullopt"
    if ($null -ne $entry.premises_text) {
        $premises = Escape-CppString ([string]$entry.premises_text)
        $premisesField = ('std::string_view("{0}")' -f $premises)
    }
    $null = $sb.AppendLine(('    {{"{0}", "{1}", {2}, "{3}", {4}}},' -f $rid, $fam, $diagField, $src, $premisesField))
}
$null = $sb.AppendLine("};")

[System.IO.File]::WriteAllText($OutputPath, $sb.ToString(), $utf8NoBom)

$duplicateRuleIds = @()
foreach ($ruleId in $sortedRules) {
    if ($ruleToSources[$ruleId].Count -gt 1 -and -not $appliedSourceOverrides.ContainsKey($ruleId)) {
        $duplicateRuleIds += [PSCustomObject]@{
            rule_id = $ruleId
            source_count = $ruleToSources[$ruleId].Count
            sources = $ruleToSources[$ruleId]
        }
    }
}

$report = [ordered]@{
    generated_at = (Get-Date).ToString("o")
    source_root = $sourceRoot
    rule_count = $sortedRules.Count
    unique_rule_count = $sortedRules.Count
    duplicate_rule_ids = $duplicateRuleIds
    applied_source_overrides = $appliedSourceOverrides
    invalid_source_overrides = $invalidSourceOverrides
    family_conflicts = $familyConflicts
    unmapped_rules = $unmappedRules
    output_path = $OutputPath
}

if (-not [string]::IsNullOrWhiteSpace($ReportPath)) {
    $reportDir = Split-Path -Parent $ReportPath
    if (-not [string]::IsNullOrWhiteSpace($reportDir)) {
        New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
    }
    [System.IO.File]::WriteAllText(
        $ReportPath,
        (($report | ConvertTo-Json -Depth 10) + "`n"),
        $utf8NoBom
    )
}

if ($Strict) {
    if ($unmappedRules.Count -gt 0) {
        throw "Strict mode failed: unmapped rules detected ($($unmappedRules.Count))."
    }
    if ($invalidSourceOverrides.Count -gt 0) {
        throw "Strict mode failed: invalid source overrides detected ($($invalidSourceOverrides -join ', '))."
    }
}

Write-Host "[static-rule-registry] rules=$($sortedRules.Count) unmapped=$($unmappedRules.Count) conflicts=$($familyConflicts.Count)"
