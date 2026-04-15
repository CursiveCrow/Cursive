param(
    [string]$CompilerPath = "C:/Dev/Cursive/build/Release/cursive.exe"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\\..\\..")).Path
$runId = Get-Date -Format "yyyyMMdd_HHmmss_fff"
$logsRoot = Join-Path $repoRoot "build\\logs"
New-Item -ItemType Directory -Path $logsRoot -Force | Out-Null
$scratchRoot = Join-Path ([System.IO.Path]::GetTempPath()) "cursive_static_conformance"
New-Item -ItemType Directory -Path $scratchRoot -Force | Out-Null
$workRoot = Join-Path $scratchRoot ("tmp_modals_static_conformance_" + $runId)
New-Item -ItemType Directory -Path $workRoot -Force | Out-Null

$manifestLines = @(
    "[[assembly]]",
    "name = ""probe""",
    "kind = ""executable""",
    "root = "".""",
    "out_dir = ""build/probe"""
)

$cases = @(
    @{
        id = "E-TYP-2050"
        expectCodes = @("E-TYP-2050")
        expectExit = 1
        source = @'
modal M {
}
public procedure main(move ctx: Context) -> i32 {
    return 0
}
'@
    },
    @{
        id = "E-TYP-2051"
        expectCodes = @("E-TYP-2051")
        expectExit = 1
        source = @'
modal M {
    @S {
    }
    @S {
    }
}
public procedure main(move ctx: Context) -> i32 {
    return 0
}
'@
    },
    @{
        id = "E-TYP-2052"
        expectCodes = @("E-TYP-2052")
        expectExit = 1
        source = @'
modal M {
    @S {
        x: i32
    }
    @T {
        y: i32
    }
}
public procedure main(move ctx: Context) -> i32 {
    let m: M@S = M@S { x: 0 }
    let _ = m.y
    return 0
}
'@
    },
    @{
        id = "E-TYP-2053"
        expectCodes = @("E-TYP-2053")
        expectExit = 1
        source = @'
modal M {
    @S {
        procedure f(~) -> i32 {
            return 0
        }
    }
}
public procedure main(move ctx: Context) -> i32 {
    let m: M@S = M@S { }
    let _ = m~>missing()
    return 0
}
'@
    },
    @{
        id = "E-TYP-2054"
        expectCodes = @("E-TYP-2054")
        expectExit = 1
        source = @'
modal M {
    @M {
    }
}
public procedure main(move ctx: Context) -> i32 {
    return 0
}
'@
    },
    @{
        id = "E-TYP-2055"
        expectCodes = @("E-TYP-2055")
        expectExit = 1
        source = @'
modal M {
    @S {
        transition t() -> @T {
            return M@S { }
        }
    }
    @T {
    }
}
public procedure main(move ctx: Context) -> i32 {
    return 0
}
'@
    },
    @{
        id = "E-TYP-2056"
        expectCodes = @("E-TYP-2056")
        expectExit = 1
        source = @'
modal M {
    @S {
        transition go() -> @T {
            return M@T { }
        }
    }
    @T {
    }
}
public procedure main(move ctx: Context) -> i32 {
    let m: M@T = M@T { }
    let _ = m~>go()
    return 0
}
'@
    },
    @{
        id = "E-TYP-2057"
        expectCodes = @("E-TYP-2057")
        expectExit = 1
        source = @'
modal M {
    @S {
        x: i32
    }
    @T {
        y: i32
    }
}
public procedure main(move ctx: Context) -> i32 {
    let m: M = widen(M@S { x: 0 })
    let _ = m.x
    return 0
}
'@
    },
    @{
        id = "E-TYP-2058"
        expectCodes = @("E-TYP-2058")
        expectExit = 1
        source = @'
modal M {
    @S {
        x: i32
        x: i32
    }
}
public procedure main(move ctx: Context) -> i32 {
    return 0
}
'@
    },
    @{
        id = "E-TYP-2059"
        expectCodes = @("E-TYP-2059")
        expectExit = 1
        source = @'
modal M {
    @S {
        transition t() -> @Missing {
        }
    }
}
public procedure main(move ctx: Context) -> i32 {
    return 0
}
'@
    },
    @{
        id = "E-TYP-2060"
        expectCodes = @("E-TYP-2060")
        expectExit = 1
        source = @'
modal M {
    @S {
    }
    @T {
    }
}
public procedure main(move ctx: Context) -> i32 {
    let m: M = widen(M@S { })
    return if m is {
        @S { 0 }
    }
}
'@
    },
    @{
        id = "E-TYP-2061"
        expectCodes = @("E-TYP-2061")
        expectExit = 1
        source = @'
modal M {
    @S {
        procedure f(~) -> i32 {
            return 0
        }
        procedure f(~) -> i32 {
            return 1
        }
    }
}
public procedure main(move ctx: Context) -> i32 {
    return 0
}
'@
    },
    @{
        id = "E-TYP-2062"
        expectCodes = @("E-TYP-2062")
        expectExit = 1
        source = @'
modal M {
    @S {
        transition t() -> @S {
            return M@S { }
        }
        transition t() -> @S {
            return M@S { }
        }
    }
}
public procedure main(move ctx: Context) -> i32 {
    return 0
}
'@
    },
    @{
        id = "E-TYP-2063"
        expectCodes = @("E-TYP-2063")
        expectExit = 1
        source = @'
modal M {
    @S {
        public x: i32
    }
}
public procedure main(move ctx: Context) -> i32 {
    return 0
}
'@
    },
    @{
        id = "E-TYP-2064"
        expectCodes = @("E-TYP-2064")
        expectExit = 1
        source = @'
modal M {
    @S {
        private procedure hidden(~) -> i32 {
            return 0
        }
    }
}
public procedure main(move ctx: Context) -> i32 {
    let m: M@S = M@S { }
    let _ = m~>hidden()
    return 0
}
'@
    },
    @{
        id = "E-TYP-2065"
        expectCodes = @("E-TYP-2065")
        expectExit = 1
        source = @'
modal M {
    @S {
        x: i32
        transition x() -> @S {
            return M@S { x: 0 }
        }
    }
}
public procedure main(move ctx: Context) -> i32 {
    return 0
}
'@
    },
    @{
        id = "E-TYP-1605-MODAL-UNIQUE"
        expectCodes = @("E-TYP-1605")
        expectExit = 1
        source = @'
public modal M {
    @S {
        public procedure need_unique(~!) -> i32 {
            return 1
        }
    }
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let m: const M@S = M@S { }
    let _ = m~>need_unique()
    return 0
}
'@
    },
    @{
        id = "E-TYP-1605-MODAL-SHARED"
        expectCodes = @("E-TYP-1605")
        expectExit = 1
        source = @'
public modal M {
    @S {
        public procedure need_shared(~%) -> i32 {
            return 1
        }
    }
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let m: const M@S = M@S { }
    let _ = m~>need_shared()
    return 0
}
'@
    },
    @{
        id = "E-TYP-2070"
        expectCodes = @("E-TYP-2070")
        expectExit = 1
        source = @'
modal M {
    @A {
        x: i32
    }
    @B {
        y: i32
    }
}
public procedure main(move ctx: Context) -> i32 {
    let state: M@A = M@A { x: 1 }
    let _general: M = state
    return 0
}
'@
    },
    @{
        id = "E-TYP-2071"
        expectCodes = @("E-TYP-2071")
        expectExit = 1
        source = @'
public procedure main(move ctx: Context) -> i32 {
    let _ = widen(0i32)
    return 0
}
'@
    },
    @{
        id = "E-TYP-2072"
        expectCodes = @("E-TYP-2072")
        expectExit = 1
        source = @'
modal M {
    @S {
    }
    @T {
    }
}
public procedure main(move ctx: Context) -> i32 {
    let s: M@S = M@S { }
    let g: M = widen(s)
    let _ = widen(g)
    return 0
}
'@
    },
    @{
        id = "E-TYP-2073"
        expectCodes = @("E-TYP-2073")
        expectExit = 1
        source = @'
public procedure main(move ctx: Context) -> i32 {
    let file: File@Read = File@Read { handle: 1usize }
    let _ = file
    return 0
}
'@
    },
    @{
        id = "E-TYP-2073-DIRITER"
        expectCodes = @("E-TYP-2073")
        expectExit = 1
        source = @'
public procedure main(move ctx: Context) -> i32 {
    let iter: DirIter@Open = DirIter@Open { }
    let _ = iter
    return 0
}
'@
    },
    @{
        id = "E-TYP-2073-CANCELTOKEN"
        expectCodes = @("E-TYP-2073")
        expectExit = 1
        source = @'
public procedure main(move ctx: Context) -> i32 {
    let token: CancelToken@Active = CancelToken@Active { }
    let _ = token
    return 0
}
'@
    },
    @{
        id = "E-TYP-2073-SPAWNED"
        expectCodes = @("E-TYP-2073")
        expectExit = 1
        source = @'
public procedure main(move ctx: Context) -> i32 {
    let handle: Spawned<i32>@Ready = Spawned<i32>@Ready { value: 1 }
    let _ = handle
    return 0
}
'@
    },
    @{
        id = "E-TYP-2073-TRACKED"
        expectCodes = @("E-TYP-2073")
        expectExit = 1
        source = @'
public procedure main(move ctx: Context) -> i32 {
    let handle: Tracked<i32, IoError>@Ready =
        Tracked<i32, IoError>@Ready { value: IoError::IoFailure }
    let _ = handle
    return 0
}
'@
    },
    @{
        id = "E-TYP-2073-ASYNC"
        expectCodes = @("E-TYP-2073")
        expectExit = 1
        source = @'
public procedure main(move ctx: Context) -> i32 {
    let a: Async<(), (), (), !>@Completed =
        Async<(), (), (), !>@Completed { value: () }
    let _ = a
    return 0
}
'@
    },
    @{
        id = "E-TYP-2303"
        expectCodes = @("E-TYP-2303")
        expectExit = 1
        source = @'
modal M<TArg> {
    @S {
    }
}
type BadState = M@S
public procedure main(move ctx: Context) -> i32 {
    return 0
}
'@
    },
    @{
        id = "E-TYP-2401"
        expectCodes = @("E-TYP-2401")
        expectExit = 1
        source = @'
class C {
    @S {
    }
}
record R <: C {
}
public procedure main(move ctx: Context) -> i32 {
    return 0
}
'@
    },
    @{
        id = "E-TYP-2403"
        expectCodes = @("E-TYP-2403")
        expectExit = 1
        source = @'
public class C {
    @Start {
        id: i32
    }
    @End {
        id: i32
        status: i32
    }
}
modal M <: C {
    @Start {
        id: i32
    }
}
public procedure main(move ctx: Context) -> i32 {
    return 0
}
'@
    },
    @{
        id = "E-TYP-2405"
        expectCodes = @("E-TYP-2405")
        expectExit = 1
        source = @'
public class C {
    @Start {
        id: i32
        status: i32
    }
}
modal M <: C {
    @Start {
        id: i32
    }
}
public procedure main(move ctx: Context) -> i32 {
    return 0
}
'@
    },
    @{
        id = "E-TYP-2407"
        expectCodes = @("E-TYP-2407")
        expectExit = 1
        source = @'
public class A {
    @S {
        id: i32
    }
}
public class B {
    @S {
        id: bool
    }
}
modal M <: A, B {
    @S {
        id: i32
    }
}
public procedure main(move ctx: Context) -> i32 {
    return 0
}
'@
    },
    @{
        id = "E-TYP-2409"
        expectCodes = @("E-TYP-2409")
        expectExit = 1
        source = @'
public class C {
    @S {
        id: i32
    }
    @S {
        id: bool
    }
}
public procedure main(move ctx: Context) -> i32 {
    return 0
}
'@
    },
    @{
        id = "E-CON-0133"
        expectCodes = @("E-CON-0133")
        expectExit = 1
        source = @'
public procedure main(move ctx: Context) -> i32 {
    let _ = parallel ctx~>cpu() {
        var gate: i32 = 1
        # gate write {
            let handle: Spawned<i32> = spawn { 3 }
            let _: i32 = wait handle
        }
        0
    }
    return 0
}
'@
    },
    @{
        id = "E-CON-0213"
        expectCodes = @("E-CON-0213")
        expectExit = 1
        source = @'
procedure YieldKeyViolation() -> Async<i32, (), (), !> {
    var gate: i32 = 0
    # gate write {
        let _: () = yield 2
    }
    return ()
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let _ = YieldKeyViolation()
    return 0
}
'@
    },
    @{
        id = "E-CON-0224"
        expectCodes = @("E-CON-0224")
        expectExit = 1
        source = @'
procedure YieldFromKeyChild() -> Async<i32, (), i32, !> {
    let _: () = yield 1
    return 7
}
procedure YieldFromKeyViolation() -> Async<i32, (), i32, !> {
    var gate: i32 = 0
    # gate write {
        let _: i32 = yield from YieldFromKeyChild()
    }
    return 0
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let _ = YieldFromKeyViolation()
    return 0
}
'@
    },
    @{
        id = "E-SEM-2854-MODAL-INVARIANT-RESULT"
        expectCodes = @("E-SEM-2854")
        expectExit = 1
        source = @'
modal M {
    @S {
        value: i32
    }
} where { @result >= 0 }

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
    },
    @{
        id = "E-SEM-3004-MODAL-INVARIANT-PURE"
        expectCodes = @("E-SEM-3004")
        expectExit = 1
        source = @'
modal M {
    @S {
        value: i32
    }
} where { (unsafe { self.value }) >= 0 }

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
    },
    @{
        id = "W-SYS-4010"
        expectCodes = @("W-SYS-4010")
        expectExit = 0
        source = @'
modal M {
    @Large {
        payload: [i64; 40]
    }
    @Small {
        value: i64
    }
}
public procedure main(move ctx: Context) -> i32 {
    let s: M@Large =
        M@Large {
            payload: [
                1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
                31, 32, 33, 34, 35, 36, 37, 38, 39, 40
            ]
        }
    let _g: M = widen(s)
    return 0
}
'@
    }
)

function Invoke-ModalDiagnosticCase {
    param([hashtable]$Case)

    $caseDirName = $Case.id.Replace("-", "_")
    $caseDir = Join-Path $workRoot $caseDirName
    New-Item -ItemType Directory -Path $caseDir | Out-Null
    Set-Content -Path (Join-Path $caseDir "cursive.toml") -Value $manifestLines
    Set-Content -Path (Join-Path $caseDir "main.cursive") -Value $Case.source

    Push-Location $caseDir
    $savedErrorPreference = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $rawOutput = & $CompilerPath build . --assembly probe --check --diag-json --quiet 2>&1
        $rc = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $savedErrorPreference
        Pop-Location
    }

    $outputLines = @()
    foreach ($line in $rawOutput) {
        $outputLines += $line.ToString()
    }

    $jsonLine = $null
    foreach ($line in $outputLines) {
        $trimmed = $line.TrimStart()
        if ($trimmed.StartsWith("{")) {
            $jsonLine = $trimmed
        }
    }

    $observedCodes = @()
    if ($null -ne $jsonLine) {
        $payload = $jsonLine | ConvertFrom-Json
        if ($null -ne $payload.diagnostics) {
            foreach ($diag in $payload.diagnostics) {
                $observedCodes += [string]$diag.code
            }
        }
    }

    $missingCodes = @()
    foreach ($code in $Case.expectCodes) {
        if (-not ($observedCodes -contains $code)) {
            $missingCodes += $code
        }
    }

    $rcMatches = $rc -eq $Case.expectExit
    $pass = $rcMatches -and ($missingCodes.Count -eq 0)

    return @{
        id = $Case.id
        pass = $pass
        exit_code = $rc
        expected_exit = $Case.expectExit
        expected_codes = $Case.expectCodes
        observed_codes = $observedCodes
        missing_codes = $missingCodes
        case_dir = $caseDir
        output = $outputLines
    }
}

$results = @()
foreach ($case in $cases) {
    $result = Invoke-ModalDiagnosticCase -Case $case
    $results += $result
    if ($result.pass) {
        Write-Output ("[PASS] " + $result.id +
                      " rc=" + $result.exit_code +
                      " codes=" + (($result.observed_codes -join ",") ))
    } else {
        Write-Output ("[FAIL] " + $result.id +
                      " rc=" + $result.exit_code +
                      " expected_rc=" + $result.expected_exit +
                      " missing=" + (($result.missing_codes -join ",") ) +
                      " observed=" + (($result.observed_codes -join ",") ))
    }
}

$reportPath = Join-Path $logsRoot ("modal_static_conformance_report_" + $runId + ".json")
$results | ConvertTo-Json -Depth 8 | Set-Content -Path $reportPath
Write-Output ("report=" + $reportPath)

$failures = @($results | Where-Object { -not $_.pass })
$exitCode = 0
if ($failures.Count -gt 0) {
    Write-Output ("failures=" + $failures.Count + "/" + $cases.Count)
    $exitCode = 1
} else {
    Write-Output ("pass=" + $cases.Count + "/" + $cases.Count)
}

if (Test-Path $workRoot) {
    Remove-Item -Recurse -Force $workRoot
}

exit $exitCode
