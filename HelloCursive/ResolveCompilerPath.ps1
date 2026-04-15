param(
    [Parameter(Mandatory = $true)]
    [string]$RepoRoot,
    [string]$RequestedPath = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not [string]::IsNullOrWhiteSpace($RequestedPath)) {
    if (-not (Test-Path $RequestedPath)) {
        throw "Requested compiler path does not exist: $RequestedPath"
    }

    return (Resolve-Path $RequestedPath).Path
}

$candidate_paths = @(
    (Join-Path $RepoRoot "cursive\\build\\windows\\out\\cursive.exe"),
    (Join-Path $RepoRoot "cursive\\build\\windows\\Debug\\cursive.exe"),
    (Join-Path $RepoRoot "cursive\\build\\windows\\Release\\cursive.exe"),
    (Join-Path $RepoRoot "cursive\\build\\Debug\\cursive.exe"),
    (Join-Path $RepoRoot "cursive\\build\\Release\\cursive.exe"),
    (Join-Path $RepoRoot "build\\Debug\\cursive.exe"),
    (Join-Path $RepoRoot "build\\Release\\cursive.exe")
)

foreach ($candidate_path in $candidate_paths) {
    if (Test-Path $candidate_path) {
        return (Resolve-Path $candidate_path).Path
    }
}

$candidate_list = $candidate_paths -join [Environment]::NewLine
throw "Unable to locate cursive.exe. Checked:`n$candidate_list"
