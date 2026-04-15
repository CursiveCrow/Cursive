param(
    [string]$CompilerPath = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($CompilerPath)) {
    $CompilerPath = (Join-Path $PSScriptRoot "..\..\cursive\build\Debug\cursive.exe")
}

$workspaceRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$llvmBinManifestPath = ((Join-Path $workspaceRoot "extern\llvm\llvm-21.1.8-x86_64\bin") -replace "\\", "/")
$runId = Get-Date -Format "yyyyMMdd_HHmmss_fff"
$scratchRoot = Join-Path ([System.IO.Path]::GetTempPath()) "cursive_log_runtime_metadata"
$caseRoot = Join-Path $scratchRoot ("issue513_log_runtime_metadata_" + $runId)
New-Item -ItemType Directory -Path $caseRoot -Force | Out-Null

$manifestPath = Join-Path $caseRoot "Cursive.toml"
$sourcePath = Join-Path $caseRoot "Main.cursive"
$diagJsonPath = Join-Path $caseRoot "diag.json"
$stderrPath = Join-Path $caseRoot "stderr.txt"
$runtimeLogPath = Join-Path $caseRoot "runtime.log"

function Import-VcVarsEnvironment {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhere)) {
        throw "Case 'issue513_log_runtime_metadata' missing vswhere.exe at $vswhere"
    }

    $vcvarsPath = & $vswhere `
        -latest `
        -products * `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -find "VC\Auxiliary\Build\vcvars64.bat"
    if ([string]::IsNullOrWhiteSpace($vcvarsPath)) {
        throw "Case 'issue513_log_runtime_metadata' could not locate vcvars64.bat."
    }

    $envDump = & cmd.exe /d /c "`"$vcvarsPath`" >nul && set"
    if ($LASTEXITCODE -ne 0) {
        throw "Case 'issue513_log_runtime_metadata' failed to import vcvars64.bat environment."
    }

    foreach ($line in $envDump) {
        $eq = $line.IndexOf("=")
        if ($eq -lt 1) {
            continue
        }
        $name = $line.Substring(0, $eq)
        if (($name -ne "PATH") -and
            ($name -ne "LIB") -and
            ($name -ne "LIBPATH") -and
            ($name -ne "INCLUDE")) {
            continue
        }
        Set-Item -Path ("Env:" + $name) -Value $line.Substring($eq + 1)
    }
}

[System.IO.File]::WriteAllLines(
    $manifestPath,
    @(
        "[toolchain]",
        "llvm_bin = ""$llvmBinManifestPath""",
        "",
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""executable""",
        "root = "".""",
        "out_dir = ""build/probe"""
    )
)

[System.IO.File]::WriteAllText(
    $sourcePath,
@'
[[log(label: "issue513-runtime-procedure")]]
procedure LoggedProbe() -> i32 {
    [[log(label: "issue513-runtime-statement")]]
    let base: i32 = 0
    let observed: i32 = 7
    [[log(label: "issue513-runtime-expression", expected: 7)]] observed
    [[log(label: "issue513-runtime-binding")]] let copied: i32 = observed
    let _ = base
    return copied
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let result: i32 = LoggedProbe()
    if (result != 7) {
        return 1
    }
    return 0
}
'@
)

Import-VcVarsEnvironment
Write-Host "CASE_ROOT=$caseRoot"

function Get-RuntimeTraceRows {
    param(
        [Parameter(Mandatory = $true)]
        [string]$LogPath
    )

    $rows = New-Object System.Collections.Generic.List[object]
    $index = 0
    foreach ($line in [System.IO.File]::ReadAllLines($LogPath)) {
        if ([string]::IsNullOrWhiteSpace($line) -or $line -eq "runtime_trace_v1") {
            continue
        }

        $seqMatch = [regex]::Match($line, '(?:^|;)seq=(\d+)')
        $tidMatch = [regex]::Match($line, '(?:^|;)tid=(\d+)')
        $rows.Add([PSCustomObject]@{
            Index = $index
            Line = $line
            HasSeq = $seqMatch.Success
            Seq = if ($seqMatch.Success) { [uint64]$seqMatch.Groups[1].Value } else { [uint64]0 }
            HasTid = $tidMatch.Success
            Tid = if ($tidMatch.Success) { [uint64]$tidMatch.Groups[1].Value } else { [uint64]0 }
        }) | Out-Null
        $index += 1
    }

    return $rows.ToArray()
}

function Assert-RuntimeTraceRowsHaveMetadataAndAscendingSeq {
    param(
        [Parameter(Mandatory = $true)]
        [object[]]$Rows,
        [Parameter(Mandatory = $true)]
        [string]$Label
    )

    if ($Rows.Count -eq 0) {
        throw "$Label expected at least one runtime trace row."
    }

    for ($i = 0; $i -lt $Rows.Count; $i += 1) {
        $row = $Rows[$i]
        if ((-not $row.HasSeq) -or (-not $row.HasTid)) {
            throw "$Label expected every matched row to include tid= and seq= metadata."
        }
        if ($row.Tid -eq 0) {
            throw "$Label expected non-zero Windows thread identifiers on matched rows."
        }
        if (($i -gt 0) -and ($Rows[$i - 1].Seq -ge $row.Seq)) {
            throw "$Label expected strictly ascending seq= order in file order."
        }
    }
}

Push-Location $caseRoot
try {
    & $CompilerPath --incremental off --diag-json --quiet Main.cursive 1> $diagJsonPath 2> $stderrPath
    $buildExit = $LASTEXITCODE
} finally {
    Pop-Location
}

$diagJson = (Get-Content -Path $diagJsonPath -Raw) | ConvertFrom-Json
$buildErrors = @($diagJson.diagnostics | Where-Object {
    $_.severity -eq "error" -or $_.severity -eq "panic"
}).Count
$unexpectedBuildErrors = @($diagJson.diagnostics | Where-Object {
    ($_.severity -eq "error" -or $_.severity -eq "panic") -and
    ($_.code -ne "E-OUT-0404")
})
if ($unexpectedBuildErrors.Count -ne 0) {
    throw "Case 'issue513_log_runtime_metadata' observed unexpected compiler diagnostics before runtime verification."
}

$objPath = Join-Path $caseRoot "build\probe\obj\probe.obj"
if (-not (Test-Path $objPath)) {
    throw "Case 'issue513_log_runtime_metadata' missing compiler-generated object artifact: $objPath"
}

if (($buildExit -ne 0) -and
    (($buildErrors -ne 1) -or ($diagJson.diagnostics[0].code -ne "E-OUT-0404"))) {
    throw "Case 'issue513_log_runtime_metadata' expected either a full successful build or only the known final-link wrapper diagnostic E-OUT-0404."
}

$supportRoot = Join-Path $workspaceRoot "cursive\build"
$lldLinkPath = Join-Path $supportRoot "tools\lld-link.exe"
$runtimeLibPath = Join-Path $supportRoot "runtime\cursive0_rt.lib"
if ((-not (Test-Path $lldLinkPath)) -or (-not (Test-Path $runtimeLibPath))) {
    throw "Case 'issue513_log_runtime_metadata' missing bundled linker support artifacts."
}

$exePath = Join-Path $caseRoot "build\probe\bin\probe_manual.exe"
$mapPath = Join-Path $caseRoot "build\probe\bin\probe_manual.map"
& $lldLinkPath `
    /NOLOGO `
    /OUT:$exePath `
    /ENTRY:main `
    /SUBSYSTEM:CONSOLE `
    /STACK:1048576,65536 `
    /MAP:$mapPath `
    /NODEFAULTLIB `
    $objPath `
    $runtimeLibPath
if ($LASTEXITCODE -ne 0) {
    throw "Case 'issue513_log_runtime_metadata' manual link step failed with exit $LASTEXITCODE."
}

$savedPath = $env:PATH
$env:PATH = (Join-Path $supportRoot "runtime") + ";" + $savedPath
$savedSink = $env:CURSIVE_RUNTIME_SINK
$savedRuntimePath = $env:CURSIVE_RUNTIME_PATH
$env:CURSIVE_RUNTIME_SINK = "file"
$env:CURSIVE_RUNTIME_PATH = $runtimeLogPath
$run = Start-Process -FilePath $exePath -NoNewWindow -Wait -PassThru
$env:PATH = $savedPath
if ($null -ne $savedSink) {
    $env:CURSIVE_RUNTIME_SINK = $savedSink
} else {
    Remove-Item Env:CURSIVE_RUNTIME_SINK -ErrorAction SilentlyContinue
}
if ($null -ne $savedRuntimePath) {
    $env:CURSIVE_RUNTIME_PATH = $savedRuntimePath
} else {
    Remove-Item Env:CURSIVE_RUNTIME_PATH -ErrorAction SilentlyContinue
}
if ($run.ExitCode -ne 0) {
    throw "Case 'issue513_log_runtime_metadata' expected runtime exit 0 but got $($run.ExitCode)."
}

if (-not (Test-Path $runtimeLogPath)) {
    throw "Case 'issue513_log_runtime_metadata' missing runtime log: $runtimeLogPath"
}

$traceRows = @(Get-RuntimeTraceRows -LogPath $runtimeLogPath)
$labelMinimums = [ordered]@{
    "issue513-runtime-procedure" = 2
    "issue513-runtime-statement" = 1
    "issue513-runtime-binding" = 1
    "issue513-runtime-expression" = 1
}
$matchedRows = New-Object System.Collections.Generic.List[object]
foreach ($label in $labelMinimums.Keys) {
    $needle = "label%3D" + $label
    $rowsForLabel = @($traceRows | Where-Object { $_.Line.Contains($needle) })
    if ($rowsForLabel.Count -lt $labelMinimums[$label]) {
        throw "Case 'issue513_log_runtime_metadata' expected at least $($labelMinimums[$label]) row(s) for label '$label'."
    }
    foreach ($row in $rowsForLabel) {
        $matchedRows.Add($row) | Out-Null
    }
}

$uniqueMatchedRows = @($matchedRows | Sort-Object Index -Unique)
Assert-RuntimeTraceRowsHaveMetadataAndAscendingSeq `
    -Rows $uniqueMatchedRows `
    -Label "Case 'issue513_log_runtime_metadata'"

Write-Host "[compiler-runtime] issue513_log_runtime_metadata: build_exit=$buildExit build_errors=$buildErrors run_exit=$($run.ExitCode) matched_rows=$($uniqueMatchedRows.Count)"
