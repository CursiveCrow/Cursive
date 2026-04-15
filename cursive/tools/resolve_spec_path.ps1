param(
    [string]$RepoRoot = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\\..")).Path
}

function Test-SpecCandidate {
    param([string]$Path)

    $text = [System.IO.File]::ReadAllText($Path)
    return ($text -match '(?m)^# Cursive Language Specification\s*$') -and
           ($text -match 'This file is the canonical normative language specification\.')
}

$candidates = New-Object System.Collections.Generic.List[string]
$files = Get-ChildItem -Path $RepoRoot -Recurse -File -Filter *.md |
    Where-Object {
        $_.FullName -notmatch '[\\/](?:\.git|build|extern|node_modules|\.vs)[\\/]'
    }

foreach ($file in $files) {
    if (Test-SpecCandidate -Path $file.FullName) {
        $candidates.Add($file.FullName) | Out-Null
    }
}

if ($candidates.Count -eq 0) {
    throw "Unable to resolve canonical language spec under repo root: $RepoRoot"
}

if ($candidates.Count -gt 1) {
    $ordered = @($candidates | Sort-Object)
    throw "Multiple canonical language spec candidates found: $($ordered -join '; ')"
}

(Resolve-Path $candidates[0]).Path
