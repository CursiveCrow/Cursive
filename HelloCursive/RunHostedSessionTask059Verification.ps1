param(
    [string]$CompilerPath = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

Set-Location $PSScriptRoot
$workspaceRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$resolveCompilerPath = Join-Path $PSScriptRoot "ResolveCompilerPath.ps1"
$CompilerPath = (& $resolveCompilerPath -RepoRoot $workspaceRoot -RequestedPath $CompilerPath)

$runId = Get-Date -Format "yyyyMMdd_HHmmss_fff"
$logsRoot = Join-Path ([System.IO.Path]::GetTempPath()) "cursive_task059_verification"
$runLogDir = Join-Path $logsRoot ("hosted_session_" + $runId)
New-Item -ItemType Directory -Path $runLogDir -Force | Out-Null

$hostedBuildDiagPath = Join-Path $runLogDir "hostedshared_diag.json"
$hostedBuildStderrPath = Join-Path $runLogDir "hostedshared_stderr.txt"
$hostedAltBuildDiagPath = Join-Path $runLogDir "hostedsharedalt_diag.json"
$hostedAltBuildStderrPath = Join-Path $runLogDir "hostedsharedalt_stderr.txt"
$caseRoot = Join-Path $runLogDir "case"
New-Item -ItemType Directory -Path $caseRoot -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $caseRoot "HostedSharedDep") -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $caseRoot "HostedShared") -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $caseRoot "HostedSharedAlt") -Force | Out-Null

$manifestLines = @(
    "[[assembly]]",
    "name = ""hostedshareddep""",
    "kind = ""dependency""",
    "root = ""./HostedSharedDep""",
    "out_dir = ""build/hostedshareddep""",
    "",
    "[[assembly]]",
    "name = ""hostedshared""",
    "kind = ""library""",
    "root = ""./HostedShared""",
    "out_dir = ""build/hostedshared""",
    "emit_ir = ""ll""",
    "",
    "[[assembly]]",
    "name = ""hostedsharedalt""",
    "kind = ""library""",
    "root = ""./HostedSharedAlt""",
    "out_dir = ""build/hostedsharedalt""",
    "emit_ir = ""ll"""
)
[System.IO.File]::WriteAllLines((Join-Path $caseRoot "Cursive.toml"), $manifestLines)
[System.IO.File]::WriteAllText(
    (Join-Path $caseRoot "HostedSharedDep\HostedSharedDepSupport.cursive"),
@'
var dep_counter: i32 = 0

public procedure reset() -> () {
    dep_counter = 0
}

public procedure next_value() -> i32 {
    dep_counter = dep_counter + 1
    return dep_counter
}
'@)
[System.IO.File]::WriteAllText(
    (Join-Path $caseRoot "HostedShared\HostedSharedRouter.cursive"),
@'
import hostedshareddep

record HostedFsContext {
    fs: $FileSystem
}

var library_counter: i32 = 0

procedure HostedSessionNextValue() -> i32 {
    library_counter = library_counter + 1
    let dep_value: i32 = hostedshareddep::next_value()
    return library_counter * 100 + dep_value
}

procedure BusySpin(spin_steps: u32) -> () {
    var remaining: u32 = spin_steps
    loop {
        if (remaining == 0u32) {
            break
        }
        let _ = hostedshareddep::next_value()
        remaining = remaining - 1u32
    }
    return ()
}

public procedure run(ctx: Context) -> i32 {
    let _ = ctx
    library_counter = 0
    hostedshareddep::reset()
    return 0
}

[[host_export("C-unwind"), unwind("catch"), mangle("hc_hosted_session_next")]]
public procedure host_session_next(ctx: HostedFsContext) -> i32 {
    let _ = ctx.fs
    return HostedSessionNextValue()
}

[[host_export("C-unwind"), unwind("catch"), mangle("hc_hosted_fail_zero")]]
public procedure host_fail_zero(ctx: HostedFsContext) -> i32 {
    let _ = ctx.fs
    let ptr_null: Ptr<i32>@Null = Ptr::null()
    let ptr_valid: Ptr<i32>@Valid =
        unsafe { transmute<Ptr<i32>@Null, Ptr<i32>@Valid>(ptr_null) }
    return *ptr_valid
}

[[host_export("C-unwind"), unwind("catch"), mangle("hc_hosted_hold_session")]]
public procedure host_hold_session(ctx: HostedFsContext, spin_steps: u32) -> i32 {
    let _ = ctx.fs
    BusySpin(spin_steps)
    return 707
}
'@)
[System.IO.File]::WriteAllText(
    (Join-Path $caseRoot "HostedSharedAlt\Library.cursive"),
@'
record HostedFsContext {
    fs: $FileSystem
}

var alt_counter: i32 = 0

public procedure run(ctx: Context) -> i32 {
    let _ = ctx
    alt_counter = 0
    return 0
}

[[host_export("C-unwind"), unwind("catch"), mangle("hc_hosted_alt_session_next")]]
public procedure host_alt_session_next(ctx: HostedFsContext) -> i32 {
    let _ = ctx.fs
    alt_counter = alt_counter + 1
    return 900 + alt_counter
}
'@)

function Invoke-HostedAssemblyBuild {
    param(
        [Parameter(Mandatory = $true)]
        [string]$AssemblyName,
        [Parameter(Mandatory = $true)]
        [string]$SourcePath,
        [Parameter(Mandatory = $true)]
        [string]$DiagPath,
        [Parameter(Mandatory = $true)]
        [string]$StderrPath
    )

    Push-Location $caseRoot
    try {
        & $CompilerPath --incremental off --diag-json --quiet --assembly $AssemblyName $SourcePath 1> $DiagPath 2> $StderrPath
    }
    finally {
        Pop-Location
    }
    $buildExit = $LASTEXITCODE
    Write-Host "${AssemblyName}_BUILD_EXIT=$buildExit"
    if ($buildExit -ne 0) {
        throw "Hosted-session verification failed to build assembly '$AssemblyName'. Logs: $DiagPath ; $StderrPath"
    }

    $diagJson = Get-Content -Path $DiagPath -Raw | ConvertFrom-Json
    $errorCount = @($diagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Hosted-session verification build '$AssemblyName' reported $errorCount error diagnostics."
    }
}

function Assert-Match {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Text,
        [Parameter(Mandatory = $true)]
        [string]$Pattern,
        [Parameter(Mandatory = $true)]
        [string]$Message
    )

    if ($Text -notmatch $Pattern) {
        throw $Message
    }
}

Invoke-HostedAssemblyBuild `
    -AssemblyName "hostedshared" `
    -SourcePath "HostedShared\HostedSharedRouter.cursive" `
    -DiagPath $hostedBuildDiagPath `
    -StderrPath $hostedBuildStderrPath

Invoke-HostedAssemblyBuild `
    -AssemblyName "hostedsharedalt" `
    -SourcePath "HostedSharedAlt\Library.cursive" `
    -DiagPath $hostedAltBuildDiagPath `
    -StderrPath $hostedAltBuildStderrPath

$requiredArtifacts = @(
    (Join-Path $caseRoot "build\hostedshared\bin\hostedshared.dll"),
    (Join-Path $caseRoot "build\hostedshared\lib\hostedshared.lib"),
    (Join-Path $caseRoot "build\hostedshared\ir\hostedshared.ll"),
    (Join-Path $caseRoot "build\hostedsharedalt\bin\hostedsharedalt.dll"),
    (Join-Path $caseRoot "build\hostedsharedalt\lib\hostedsharedalt.lib"),
    (Join-Path $caseRoot "build\hostedsharedalt\ir\hostedsharedalt.ll")
)

foreach ($artifact in $requiredArtifacts) {
    if (-not (Test-Path $artifact)) {
        throw "Hosted-session verification missing artifact: $artifact"
    }
}

$hostedIrText = Get-Content -Path (Join-Path $caseRoot "build\hostedshared\ir\hostedshared.ll") -Raw
$hostedAltIrText = Get-Content -Path (Join-Path $caseRoot "build\hostedsharedalt\ir\hostedsharedalt.ll") -Raw

foreach ($irText in @($hostedIrText, $hostedAltIrText)) {
    Assert-Match -Text $irText `
        -Pattern '@__cursive_host_session_owner_token\s*=' `
        -Message "Hosted-session verification expected a module owner-token global in LLVM IR."
    Assert-Match -Text $irText `
        -Pattern 'declare\s+i64\s+@cursive_host_session_register\(ptr,\s*ptr\)' `
        -Message "Hosted-session verification expected owner-aware register helper declaration."
    Assert-Match -Text $irText `
        -Pattern 'declare\s+i32\s+@cursive_host_session_try_enter\(i64,\s*ptr,\s*ptr\)' `
        -Message "Hosted-session verification expected owner-aware try-enter helper declaration."
    Assert-Match -Text $irText `
        -Pattern 'declare\s+i32\s+@cursive_host_session_leave\(i64,\s*ptr\)' `
        -Message "Hosted-session verification expected owner-aware leave helper declaration."
    Assert-Match -Text $irText `
        -Pattern 'declare\s+i32\s+@cursive_host_session_try_retire\(i64,\s*ptr,\s*ptr\)' `
        -Message "Hosted-session verification expected owner-aware retire helper declaration."
    Assert-Match -Text $irText `
        -Pattern 'declare\s+i32\s+@cursive_host_session_abort_live\(i64,\s*ptr,\s*ptr\)' `
        -Message "Hosted-session verification expected owner-aware abort-live helper declaration."
    Assert-Match -Text $irText `
        -Pattern 'declare\s+i32\s+@cursive_host_session_enter_retired\(i64,\s*ptr,\s*ptr\)' `
        -Message "Hosted-session verification expected owner-aware enter-retired helper declaration."
    Assert-Match -Text $irText `
        -Pattern 'declare\s+i32\s+@cursive_host_session_leave_retired\(i64,\s*ptr\)' `
        -Message "Hosted-session verification expected owner-aware leave-retired helper declaration."
    Assert-Match -Text $irText `
        -Pattern 'declare\s+i32\s+@cursive_host_session_abort_retired\(i64,\s*ptr,\s*ptr\)' `
        -Message "Hosted-session verification expected owner-aware abort-retired helper declaration."
}

$hostedCreateMatch = [regex]::Match(
    $hostedIrText,
    '(?ms)define\s+i64\s+@__cursive_host_session_create\(\)\s*\{(?<body>.*?)^\}')
$hostedDestroyMatch = [regex]::Match(
    $hostedIrText,
    '(?ms)define\s+i32\s+@__cursive_host_session_destroy\(i64[^)]*\)\s*\{(?<body>.*?)^\}')
$hostedThunkMatch = [regex]::Match(
    $hostedIrText,
    '(?ms)define\s+i32\s+@hc_hosted_session_next\(i64[^)]*\)\s*\{(?<body>.*?)^\}')

if (-not $hostedCreateMatch.Success -or -not $hostedDestroyMatch.Success -or -not $hostedThunkMatch.Success) {
    throw "Hosted-session verification expected analyzable hosted lifecycle and thunk bodies in LLVM IR."
}

foreach ($body in @(
    $hostedCreateMatch.Groups["body"].Value,
    $hostedDestroyMatch.Groups["body"].Value,
    $hostedThunkMatch.Groups["body"].Value
)) {
    Assert-Match -Text $body `
        -Pattern '@__cursive_host_session_owner_token' `
        -Message "Hosted-session verification expected lifecycle/thunk bodies to pass the owner token."
}

$previousPath = $env:PATH
$env:PATH = @(
    (Join-Path $caseRoot "build\hostedshared\bin"),
    (Join-Path $caseRoot "build\hostedsharedalt\bin"),
    $previousPath
) -join ";"

try {
Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
using System.Threading.Tasks;

public static class HostedSharedNative {
    [DllImport("hostedshared.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "__cursive_host_abi_version")]
    public static extern int AbiVersion();

    [DllImport("hostedshared.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "__cursive_host_session_create")]
    public static extern ulong SessionCreate();

    [DllImport("hostedshared.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "__cursive_host_session_destroy")]
    public static extern uint SessionDestroy(ulong handle);

    [DllImport("hostedshared.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "hc_hosted_session_next")]
    public static extern int SessionNext(ulong handle);

    [DllImport("hostedshared.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "hc_hosted_fail_zero")]
    public static extern int FailZero(ulong handle);

    [DllImport("hostedshared.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "hc_hosted_hold_session")]
    public static extern int HoldSession(ulong handle, uint spinSteps);

}

public static class HostedSharedAltNative {
    [DllImport("hostedsharedalt.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "__cursive_host_abi_version")]
    public static extern int AbiVersion();

    [DllImport("hostedsharedalt.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "__cursive_host_session_create")]
    public static extern ulong SessionCreate();

    [DllImport("hostedsharedalt.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "__cursive_host_session_destroy")]
    public static extern uint SessionDestroy(ulong handle);

    [DllImport("hostedsharedalt.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "hc_hosted_alt_session_next")]
    public static extern int SessionNext(ulong handle);
}

public static class HostedSessionHarness {
    public static Task<int> StartHoldSessionAsync(ulong handle, uint spinSteps) {
        return Task.Run(() => HostedSharedNative.HoldSession(handle, spinSteps));
    }

    public static Task<uint> StartDestroySessionAsync(ulong handle) {
        return Task.Run(() => HostedSharedNative.SessionDestroy(handle));
    }
}
"@

    $abiVersion = [HostedSharedNative]::AbiVersion()
    $altAbiVersion = [HostedSharedAltNative]::AbiVersion()
    if ($abiVersion -ne 1 -or $altAbiVersion -ne 1) {
        throw "Hosted-session verification observed unexpected hosted ABI versions: main=$abiVersion alt=$altAbiVersion"
    }

    $sessionA = [HostedSharedNative]::SessionCreate()
    $sessionB = [HostedSharedNative]::SessionCreate()
    if (($sessionA -eq 0) -or ($sessionB -eq 0) -or ($sessionA -eq $sessionB)) {
        throw "Hosted-session verification failed to create two distinct live sessions in hostedshared.dll."
    }

    try {
        if ([HostedSharedNative]::SessionNext($sessionA) -ne 101) {
            throw "Hosted-session verification expected hostedshared session A first metric to be 101."
        }
        if ([HostedSharedNative]::SessionNext($sessionA) -ne 202) {
            throw "Hosted-session verification expected hostedshared session A second metric to be 202."
        }
        if ([HostedSharedNative]::SessionNext($sessionB) -ne 101) {
            throw "Hosted-session verification expected hostedshared session B first metric to be 101."
        }
        if ([HostedSharedNative]::SessionNext(0) -ne 0) {
            throw "Hosted-session verification expected handle 0 rejection for hosted thunk entry."
        }
        if ([HostedSharedNative]::FailZero($sessionA) -ne 0) {
            throw "Hosted-session verification expected catch-mode panic path to return zero."
        }
    }
    finally {
        $destroyA = [HostedSharedNative]::SessionDestroy($sessionA)
        $destroyASecond = [HostedSharedNative]::SessionDestroy($sessionA)
        $destroyB = [HostedSharedNative]::SessionDestroy($sessionB)
        if ($destroyA -ne 1 -or $destroyASecond -ne 0 -or $destroyB -ne 1) {
            throw "Hosted-session verification observed incorrect basic destroy semantics."
        }
    }

    $ownerSessionMain = [HostedSharedNative]::SessionCreate()
    $ownerSessionAlt = [HostedSharedAltNative]::SessionCreate()
    if (($ownerSessionMain -eq 0) -or ($ownerSessionAlt -eq 0) -or ($ownerSessionMain -eq $ownerSessionAlt)) {
        throw "Hosted-session verification expected distinct owner-isolation sessions across hosted DLLs."
    }

    try {
        if ([HostedSharedNative]::SessionNext($ownerSessionMain) -ne 101) {
            throw "Hosted-session verification expected hostedshared owner-control call to succeed."
        }
        if ([HostedSharedAltNative]::SessionNext($ownerSessionAlt) -ne 901) {
            throw "Hosted-session verification expected hostedsharedalt owner-control call to succeed."
        }
        if ([HostedSharedAltNative]::SessionNext($ownerSessionMain) -ne 0) {
            throw "Hosted-session verification observed cross-DLL thunk acceptance for a foreign-owned handle."
        }
        if ([HostedSharedNative]::SessionNext($ownerSessionAlt) -ne 0) {
            throw "Hosted-session verification observed reverse cross-DLL thunk acceptance for a foreign-owned handle."
        }
        if ([HostedSharedAltNative]::SessionDestroy($ownerSessionMain) -ne 0) {
            throw "Hosted-session verification observed cross-DLL destroy acceptance for a foreign-owned handle."
        }
        if ([HostedSharedNative]::SessionDestroy($ownerSessionAlt) -ne 0) {
            throw "Hosted-session verification observed reverse cross-DLL destroy acceptance for a foreign-owned handle."
        }
    }
    finally {
        if ([HostedSharedNative]::SessionDestroy($ownerSessionMain) -ne 1) {
            throw "Hosted-session verification failed to destroy the hostedshared owner-control session."
        }
        if ([HostedSharedAltNative]::SessionDestroy($ownerSessionAlt) -ne 1) {
            throw "Hosted-session verification failed to destroy the hostedsharedalt owner-control session."
        }
    }

    $busySession = [HostedSharedNative]::SessionCreate()
    if ($busySession -eq 0) {
        throw "Hosted-session verification failed to create busy-session test handle."
    }
    try {
        $holdTask = [HostedSessionHarness]::StartHoldSessionAsync($busySession, [uint32]5000000)
        Start-Sleep -Milliseconds 100
        if ($holdTask.IsCompleted) {
            throw "Hosted-session verification busy-session control call completed before reentry could be tested."
        }
        if ([HostedSharedNative]::SessionNext($busySession) -ne 0) {
            throw "Hosted-session verification observed reentrant same-session hosted entry acceptance."
        }
        if ([HostedSharedNative]::SessionDestroy($busySession) -ne 0) {
            throw "Hosted-session verification observed destroy acceptance while a hosted call was active."
        }
        if (-not $holdTask.Wait(30000)) {
            throw "Hosted-session verification hold-session control call did not complete within timeout."
        }
        if ($holdTask.Result -ne 707) {
            throw "Hosted-session verification observed hold-session result $($holdTask.Result) instead of 707."
        }
        if ([HostedSharedNative]::SessionDestroy($busySession) -ne 1) {
            throw "Hosted-session verification failed to destroy the busy-session test handle after call completion."
        }
        $busySession = 0
    }
    finally {
        if ($busySession -ne 0) {
            [void][HostedSharedNative]::SessionDestroy($busySession)
        }
    }

}
finally {
    $env:PATH = $previousPath
}

Write-Host "TASK059_HOSTED_SESSION_VERIFICATION=PASS"
Write-Host "TASK059_LOG_DIR=$runLogDir"
