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

$pythonScript = Join-Path $PSScriptRoot "generate_static_rule_registry.py"
if (-not (Test-Path -LiteralPath $pythonScript)) {
    throw "Static rule registry generator missing python entrypoint: $pythonScript"
}

$args = @(
    $pythonScript,
    "--repo-root", $RepoRoot,
    "--mapping-path", $MappingPath,
    "--output-path", $OutputPath
)

if (-not [string]::IsNullOrWhiteSpace($SpecPath)) {
    $args += @("--spec-path", $SpecPath)
}
if (-not [string]::IsNullOrWhiteSpace($ReportPath)) {
    $args += @("--report-path", $ReportPath)
}
if ($Strict) {
    $args += "--strict"
}

& python @args
exit $LASTEXITCODE
