param(
    [string]$CompilerPath = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

Set-Location $PSScriptRoot
$workspaceRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$resolveCompilerPath = Join-Path $PSScriptRoot "ResolveCompilerPath.ps1"
$CompilerPath = (& $resolveCompilerPath -RepoRoot $workspaceRoot -RequestedPath $CompilerPath)
$llvmBinManifestPath = ((Join-Path $workspaceRoot "extern\\llvm\\llvm-21.1.8-x86_64\\bin") -replace "\\", "/")
$runId = Get-Date -Format "yyyyMMdd_HHmmss_fff"
$logsRoot = Join-Path ([System.IO.Path]::GetTempPath()) "cursive_hello_runs"
$runLogDir = Join-Path $logsRoot ("hello_build_run_" + $runId)
New-Item -ItemType Directory -Path $runLogDir -Force | Out-Null

$buildDiagPath = Join-Path $runLogDir "build_run_diag.json"
$buildStderrPath = Join-Path $runLogDir "build_run_stderr.txt"
$rawBuildDiagPath = Join-Path $runLogDir "rawshared_build_diag.json"
$rawBuildStderrPath = Join-Path $runLogDir "rawshared_build_stderr.txt"
$runStdoutPath = Join-Path $runLogDir "run_stdout.txt"
$runStderrPath = Join-Path $runLogDir "run_stderr.txt"
$expectationLogPath = Join-Path $runLogDir "hello_expectations.log"
$rawSharedCaseRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("cursive_rawshared_case_" + $runId)
$rawSharedDll = Join-Path $rawSharedCaseRoot "build\\rawshared\\bin\\rawshared.dll"
$rawSharedImportLib = Join-Path $rawSharedCaseRoot "build\\rawshared\\lib\\rawshared.lib"

function ConvertTo-CursiveUtf16ArrayLiteral {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Value
    )

    $bytes = [System.Text.Encoding]::Unicode.GetBytes($Value + [char]0)
    $codeUnits = New-Object System.Collections.Generic.List[string]
    for ($i = 0; $i -lt $bytes.Length; $i += 2) {
        $codeUnit = [System.BitConverter]::ToUInt16($bytes, $i)
        $codeUnits.Add("$codeUnit" + "u16")
    }

    return @{
        Count = [int]($bytes.Length / 2)
        Literal = "[" + ($codeUnits -join ", ") + "]"
    }
}

& $CompilerPath --incremental off --diag-json --quiet --log-file $expectationLogPath Main.cursive 1> $buildDiagPath 2> $buildStderrPath
$buildExit = $LASTEXITCODE
Write-Host "BUILD_EXIT=$buildExit"
Write-Host "LOG_DIR=$runLogDir"
Write-Host "EXPECTATION_LOG=$expectationLogPath"
if ($buildExit -ne 0) {
    exit $buildExit
}

New-Item -ItemType Directory -Path $rawSharedCaseRoot -Force | Out-Null
[System.IO.File]::WriteAllLines(
    (Join-Path $rawSharedCaseRoot "Cursive.toml"),
    @(
        "[toolchain]",
        "llvm_bin = ""$llvmBinManifestPath""",
        "",
        "[[assembly]]",
        "name = ""rawshared""",
        "kind = ""library""",
        "root = "".""",
        "out_dir = ""build/rawshared"""
    )
)
[System.IO.File]::WriteAllText(
    (Join-Path $rawSharedCaseRoot "Library.cursive"),
    (Get-Content -Path (Join-Path $PSScriptRoot "RawShared\\Library.cursive") -Raw)
)
Push-Location $rawSharedCaseRoot
try {
    & $CompilerPath --incremental off --diag-json --quiet --assembly rawshared Library.cursive 1> $rawBuildDiagPath 2> $rawBuildStderrPath
    $rawBuildExit = $LASTEXITCODE
}
finally {
    Pop-Location
}
Write-Host "RAWSHARED_BUILD_EXIT=$rawBuildExit"
if ($rawBuildExit -ne 0) {
    exit $rawBuildExit
}

$exePath = Join-Path $PSScriptRoot "build\main\bin\main.exe"
if (-not (Test-Path $exePath)) {
    throw "Missing output executable: $exePath"
}

$requiredArtifacts = @(
    (Join-Path $PSScriptRoot "build\linkedshared\bin\linkedshared.dll"),
    (Join-Path $PSScriptRoot "build\linkedshared\lib\linkedshared.lib"),
    (Join-Path $PSScriptRoot "build\linkedstatic\lib\linkedstatic.lib"),
    (Join-Path $PSScriptRoot "build\hostedshared\bin\hostedshared.dll"),
    (Join-Path $PSScriptRoot "build\hostedshared\lib\hostedshared.lib"),
    (Join-Path $PSScriptRoot "build\hostedsharedalt\bin\hostedsharedalt.dll"),
    (Join-Path $PSScriptRoot "build\hostedsharedalt\lib\hostedsharedalt.lib"),
    $rawSharedDll,
    $rawSharedImportLib
)

foreach ($artifact in $requiredArtifacts) {
    if (-not (Test-Path $artifact)) {
        throw "Missing expected linked-library artifact: $artifact"
    }
}

$previousErrorActionPreference = $ErrorActionPreference
$nativePrefVar = Get-Variable -Name PSNativeCommandUseErrorActionPreference -ErrorAction SilentlyContinue
$previousNativeErrorPreference = $null
if ($null -ne $nativePrefVar) {
    $previousNativeErrorPreference = $PSNativeCommandUseErrorActionPreference
    $PSNativeCommandUseErrorActionPreference = $false
}
$previousPath = $env:PATH
$runtimePathEntries = @(
    (Join-Path $PSScriptRoot "build\\linkedshared\\bin"),
    (Join-Path $PSScriptRoot "build\\hostedshared\\bin"),
    (Join-Path $PSScriptRoot "build\\hostedsharedalt\\bin")
)
$env:PATH = (($runtimePathEntries + @($previousPath)) -join ";")
$ErrorActionPreference = "Continue"
& $exePath 1> $runStdoutPath 2> $runStderrPath
$runExit = $LASTEXITCODE
$env:PATH = $previousPath
$ErrorActionPreference = $previousErrorActionPreference
if ($null -ne $nativePrefVar) {
    $PSNativeCommandUseErrorActionPreference = $previousNativeErrorPreference
}
Write-Host "RUN_EXIT=$runExit"
if ($runExit -eq 0) {
    $stdoutText = (Get-Content -Path $runStdoutPath -Raw)
    if ($null -eq $stdoutText) {
        $stdoutText = ""
    }
    $stderrText = (Get-Content -Path $runStderrPath -Raw)
    if ($null -eq $stderrText) {
        $stderrText = ""
    }
    if (-not $stdoutText.Contains("modals-fs-write-stdout-sentinel")) {
        throw "HelloCursive run did not emit the FileSystem::write_stdout sentinel to stdout."
    }
    if (-not $stderrText.Contains("modals-fs-write-stderr-sentinel")) {
        throw "HelloCursive run did not emit the FileSystem::write_stderr sentinel to stderr."
    }
    if (-not (Test-Path $expectationLogPath)) {
        throw "HelloCursive run did not produce the expectation log: $expectationLogPath"
    }

    $expectationLogText = (Get-Content -Path $expectationLogPath -Raw)
    if ($null -eq $expectationLogText) {
        $expectationLogText = ""
    }
    $expectationPassCount = ([regex]::Matches($expectationLogText, "cmp%3Dpass")).Count
    $expectationFailCount = ([regex]::Matches($expectationLogText, "cmp%3Dfail")).Count
    Write-Host "EXPECTATION_PASS_COUNT=$expectationPassCount"
    Write-Host "EXPECTATION_FAIL_COUNT=$expectationFailCount"
    if ($expectationFailCount -ne 0) {
        throw "HelloCursive expectation log recorded failing runtime expectations."
    }

    $hostedRuntimePathEntries = @(
        (Join-Path $PSScriptRoot "build\\hostedshared\\bin"),
        (Join-Path $PSScriptRoot "build\\hostedsharedalt\\bin"),
        (Split-Path -Parent $rawSharedDll)
    )
    $previousHostedPath = $env:PATH
    $env:PATH = (($hostedRuntimePathEntries + @($previousHostedPath)) -join ";")

    try {
    Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
using System.Threading.Tasks;

public static class HostedSharedNative {
    [StructLayout(LayoutKind.Sequential)]
    public struct HostedAggregateResult {
        public int First;
        public int Second;
        public int Third;
    }

    [DllImport("hostedshared.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "__cursive_host_abi_version")]
    public static extern int AbiVersion();

    [DllImport("hostedshared.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "__cursive_host_session_create")]
    public static extern ulong SessionCreate();

    [DllImport("hostedshared.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "__cursive_host_session_destroy")]
    public static extern uint SessionDestroy(ulong handle);

    [DllImport("hostedshared.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "hc_hosted_context_metric")]
    public static extern int ContextMetric(ulong handle);

    [DllImport("hostedshared.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "hc_hosted_session_next")]
    public static extern int SessionNext(ulong handle);

    [DllImport("hostedshared.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "hc_hosted_fail_zero")]
    public static extern int FailZero(ulong handle);

    [DllImport("hostedshared.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "hc_hosted_aggregate_result")]
    public static extern HostedAggregateResult AggregateResult(ulong handle);

    [DllImport("hostedshared.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "hc_hosted_aggregate_fail_zero")]
    public static extern HostedAggregateResult AggregateFailZero(ulong handle);

    [DllImport("hostedshared.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "hc_hosted_hold_session")]
    public static extern int HoldSession(ulong handle, IntPtr readyEvent, uint sleepMs);

    [DllImport("hostedshared.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "hc_hosted_configure_destroy_hook")]
    public static extern uint ConfigureDestroyHook(ulong handle, IntPtr eventHandle, uint sleepMs);
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
    public static Task<int> StartHoldSessionAsync(ulong handle, IntPtr readyEvent, uint sleepMs) {
        return Task.Run(() => HostedSharedNative.HoldSession(handle, readyEvent, sleepMs));
    }

    public static Task<uint> StartDestroySessionAsync(ulong handle) {
        return Task.Run(() => HostedSharedNative.SessionDestroy(handle));
    }
}

public static class Kernel32Sync {
    public const uint WAIT_OBJECT_0 = 0;
    public const uint WAIT_TIMEOUT = 258;

    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    public static extern IntPtr CreateEventW(IntPtr lpEventAttributes,
                                             bool bManualReset,
                                             bool bInitialState,
                                             string lpName);

    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern uint WaitForSingleObject(IntPtr hHandle, uint dwMilliseconds);

    [DllImport("kernel32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool CloseHandle(IntPtr hObject);

}

[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
public delegate int RawSharedNextDelegate();

public static class RawSharedNative {
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    public static extern IntPtr LoadLibraryW(string lpFileName);

    [DllImport("kernel32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool FreeLibrary(IntPtr hModule);

    [DllImport("kernel32.dll", CharSet = CharSet.Ansi, SetLastError = true)]
    public static extern IntPtr GetProcAddress(IntPtr hModule, string procName);

    public static RawSharedNextDelegate BindNext(IntPtr hModule) {
        IntPtr proc = GetProcAddress(hModule, "hc_rawshared_next");
        if (proc == IntPtr.Zero) {
            throw new InvalidOperationException("Failed to resolve hc_rawshared_next.");
        }
        return Marshal.GetDelegateForFunctionPointer<RawSharedNextDelegate>(proc);
    }
}
"@

    $abiVersion = [HostedSharedNative]::AbiVersion()
    if ($abiVersion -ne 1) {
        throw "Hosted shared library ABI version mismatch. Expected 1, observed $abiVersion."
    }
    $altAbiVersion = [HostedSharedAltNative]::AbiVersion()
    if ($altAbiVersion -ne 1) {
        throw "Hosted shared alt library ABI version mismatch. Expected 1, observed $altAbiVersion."
    }

    $sessionA = [HostedSharedNative]::SessionCreate()
    $sessionB = [HostedSharedNative]::SessionCreate()
    if (($sessionA -eq 0) -or ($sessionB -eq 0)) {
        throw "Hosted shared library failed to create live sessions."
    }
    if ($sessionA -eq $sessionB) {
        throw "Hosted shared library reissued the same live session handle for two distinct sessions."
    }

    try {
        $contextMetric = [HostedSharedNative]::ContextMetric($sessionA)
        $sessionAFirst = [HostedSharedNative]::SessionNext($sessionA)
        $sessionASecond = [HostedSharedNative]::SessionNext($sessionA)
        $sessionBFirst = [HostedSharedNative]::SessionNext($sessionB)
        $invalidCatch = [HostedSharedNative]::SessionNext(0)
        $panicCatch = [HostedSharedNative]::FailZero($sessionA)

        if ($contextMetric -ne 21) {
            throw "Hosted shared library context metric returned $contextMetric instead of 21."
        }
        if ($sessionAFirst -ne 101) {
            throw "Hosted shared library session A first metric returned $sessionAFirst instead of 101."
        }
        if ($sessionASecond -ne 202) {
            throw "Hosted shared library session A second metric returned $sessionASecond instead of 202."
        }
        if ($sessionBFirst -ne 101) {
            throw "Hosted shared library session B first metric returned $sessionBFirst instead of 101."
        }
        if ($invalidCatch -ne 0) {
            throw "Hosted shared library invalid-handle catch path returned $invalidCatch instead of 0."
        }
        if ($panicCatch -ne 0) {
            throw "Hosted shared library panic catch path returned $panicCatch instead of 0."
        }
    }
    finally {
        $destroyA = [HostedSharedNative]::SessionDestroy($sessionA)
        $destroyASecond = [HostedSharedNative]::SessionDestroy($sessionA)
        $destroyB = [HostedSharedNative]::SessionDestroy($sessionB)
        $destroyZero = [HostedSharedNative]::SessionDestroy(0)

        if ($destroyA -ne 1) {
            throw "Hosted shared library failed to destroy session A."
        }
        if ($destroyASecond -ne 0) {
            throw "Hosted shared library accepted double-destroy on session A."
        }
        if ($destroyB -ne 1) {
            throw "Hosted shared library failed to destroy session B."
        }
        if ($destroyZero -ne 0) {
            throw "Hosted shared library accepted destroy on handle 0."
        }
    }

    $ownerSessionMain = [HostedSharedNative]::SessionCreate()
    $ownerSessionAlt = [HostedSharedAltNative]::SessionCreate()
    if (($ownerSessionMain -eq 0) -or ($ownerSessionAlt -eq 0)) {
        throw "Hosted libraries failed to create owner-isolation test sessions."
    }
    if ($ownerSessionMain -eq $ownerSessionAlt) {
        throw "Hosted libraries reissued the same hosted-session handle token across distinct hosted DLLs."
    }

    try {
        $ownerMainValid = [HostedSharedNative]::SessionNext($ownerSessionMain)
        $ownerAltValid = [HostedSharedAltNative]::SessionNext($ownerSessionAlt)
        $ownerWrongOnAlt = [HostedSharedAltNative]::SessionNext($ownerSessionMain)
        $ownerWrongOnMain = [HostedSharedNative]::SessionNext($ownerSessionAlt)
        $ownerWrongDestroyOnAlt = [HostedSharedAltNative]::SessionDestroy($ownerSessionMain)
        $ownerWrongDestroyOnMain = [HostedSharedNative]::SessionDestroy($ownerSessionAlt)

        if ($ownerMainValid -ne 101) {
            throw "Hosted shared library owner-isolation control call returned $ownerMainValid instead of 101."
        }
        if ($ownerAltValid -ne 901) {
            throw "Hosted shared alt library owner-isolation control call returned $ownerAltValid instead of 901."
        }
        if ($ownerWrongOnAlt -ne 0) {
            throw "Hosted shared alt library accepted a session handle owned by hostedshared.dll and returned $ownerWrongOnAlt instead of 0."
        }
        if ($ownerWrongOnMain -ne 0) {
            throw "Hosted shared library accepted a session handle owned by hostedsharedalt.dll and returned $ownerWrongOnMain instead of 0."
        }
        if ($ownerWrongDestroyOnAlt -ne 0) {
            throw "Hosted shared alt library accepted destroy on a session handle owned by hostedshared.dll."
        }
        if ($ownerWrongDestroyOnMain -ne 0) {
            throw "Hosted shared library accepted destroy on a session handle owned by hostedsharedalt.dll."
        }
    }
    finally {
        $ownerDestroyMain = [HostedSharedNative]::SessionDestroy($ownerSessionMain)
        $ownerDestroyAlt = [HostedSharedAltNative]::SessionDestroy($ownerSessionAlt)
        if ($ownerDestroyMain -ne 1) {
            throw "Hosted shared library failed to destroy the owner-isolation control session."
        }
        if ($ownerDestroyAlt -ne 1) {
            throw "Hosted shared alt library failed to destroy the owner-isolation control session."
        }
    }

    $busySession = [HostedSharedNative]::SessionCreate()
    if ($busySession -eq 0) {
        throw "Hosted shared library failed to create busy-session test handle."
    }
    $holdEvent = [Kernel32Sync]::CreateEventW([IntPtr]::Zero, $true, $false, $null)
    if ($holdEvent -eq [IntPtr]::Zero) {
        $holdEventError = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
        throw "Failed to create busy-session coordination event, win32=$holdEventError."
    }

    try {
        $holdTask = [HostedSessionHarness]::StartHoldSessionAsync($busySession, $holdEvent, [uint32]250)
        $holdWait = [Kernel32Sync]::WaitForSingleObject($holdEvent, [uint32]5000)
        if ($holdWait -ne [Kernel32Sync]::WAIT_OBJECT_0) {
            throw "Hosted busy-session test did not observe the hold-session entry point becoming active. wait=$holdWait"
        }

        $busyReentry = [HostedSharedNative]::SessionNext($busySession)
        $busyDestroyReject = [HostedSharedNative]::SessionDestroy($busySession)
        if ($busyReentry -ne 0) {
            throw "Hosted shared library accepted a second hosted entry on a busy session and returned $busyReentry instead of 0."
        }
        if ($busyDestroyReject -ne 0) {
            throw "Hosted shared library accepted destroy on a session while a hosted call was in progress."
        }

        if (-not $holdTask.Wait(5000)) {
            throw "Hosted shared library busy-session hold call did not complete within timeout."
        }
        $holdResult = $holdTask.Result
        if ($holdResult -ne 707) {
            throw "Hosted shared library hold-session control call returned $holdResult instead of 707."
        }

        $busyDestroy = [HostedSharedNative]::SessionDestroy($busySession)
        if ($busyDestroy -ne 1) {
            throw "Hosted shared library failed to destroy the busy-session test handle after the active call completed."
        }
        $busySession = 0
    }
    finally {
        if ($holdEvent -ne [IntPtr]::Zero) {
            if (-not [Kernel32Sync]::CloseHandle($holdEvent)) {
                $holdCloseError = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
                throw "Failed to close busy-session coordination event, win32=$holdCloseError."
            }
        }
        if ($busySession -ne 0) {
            $busyCleanup = [HostedSharedNative]::SessionDestroy($busySession)
            if (($busyCleanup -ne 0) -and ($busyCleanup -ne 1)) {
                throw "Hosted shared library returned invalid cleanup status $busyCleanup for the busy-session handle."
            }
        }
    }

    $destroyBusySession = [HostedSharedNative]::SessionCreate()
    if ($destroyBusySession -eq 0) {
        throw "Hosted shared library failed to create destroy-busy test handle."
    }
    $destroyEvent = [Kernel32Sync]::CreateEventW([IntPtr]::Zero, $true, $false, $null)
    if ($destroyEvent -eq [IntPtr]::Zero) {
        $destroyEventError = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
        throw "Failed to create destroy-busy coordination event, win32=$destroyEventError."
    }

    try {
        $destroyHookStatus = [HostedSharedNative]::ConfigureDestroyHook($destroyBusySession, $destroyEvent, [uint32]250)
        if ($destroyHookStatus -ne 1) {
            throw "Hosted shared library failed to configure the destroy-busy hook."
        }

        $destroyTask = [HostedSessionHarness]::StartDestroySessionAsync($destroyBusySession)
        $destroyWait = [Kernel32Sync]::WaitForSingleObject($destroyEvent, [uint32]5000)
        if ($destroyWait -ne [Kernel32Sync]::WAIT_OBJECT_0) {
            throw "Hosted destroy-busy test did not observe destroy teardown entering the configured hook. wait=$destroyWait"
        }

        $destroyBusyReentry = [HostedSharedNative]::SessionNext($destroyBusySession)
        if ($destroyBusyReentry -ne 0) {
            throw "Hosted shared library accepted hosted entry while session destroy was already in progress and returned $destroyBusyReentry instead of 0."
        }

        if (-not $destroyTask.Wait(5000)) {
            throw "Hosted shared library destroy-busy test did not complete within timeout."
        }
        $destroyBusyResult = $destroyTask.Result
        if ($destroyBusyResult -ne 1) {
            throw "Hosted shared library destroy-busy control call returned $destroyBusyResult instead of 1."
        }

        $destroyBusySecond = [HostedSharedNative]::SessionDestroy($destroyBusySession)
        if ($destroyBusySecond -ne 0) {
            throw "Hosted shared library accepted a second destroy after the destroy-busy test completed."
        }
        $destroyBusySession = 0
    }
    finally {
        if ($destroyEvent -ne [IntPtr]::Zero) {
            if (-not [Kernel32Sync]::CloseHandle($destroyEvent)) {
                $destroyCloseError = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
                throw "Failed to close destroy-busy coordination event, win32=$destroyCloseError."
            }
        }
        if ($destroyBusySession -ne 0) {
            $destroyBusyCleanup = [HostedSharedNative]::SessionDestroy($destroyBusySession)
            if (($destroyBusyCleanup -ne 0) -and ($destroyBusyCleanup -ne 1)) {
                throw "Hosted shared library returned invalid cleanup status $destroyBusyCleanup for the destroy-busy session."
            }
        }
    }

    $aggregateSession = [HostedSharedNative]::SessionCreate()
    if ($aggregateSession -eq 0) {
        throw "Hosted shared library failed to create aggregate-return test session."
    }
    if (($aggregateSession -eq $sessionA) -or ($aggregateSession -eq $sessionB)) {
        throw "Hosted shared library reissued a previously destroyed session handle."
    }

    try {
        $aggregateSuccess = [HostedSharedNative]::AggregateResult($aggregateSession)
        $aggregateInvalid = [HostedSharedNative]::AggregateResult(0)
        $aggregatePanic = [HostedSharedNative]::AggregateFailZero($aggregateSession)

        if (($aggregateSuccess.First -ne 11) -or
            ($aggregateSuccess.Second -ne 22) -or
            ($aggregateSuccess.Third -ne 33)) {
            throw "Hosted aggregate-return success path returned ($($aggregateSuccess.First), $($aggregateSuccess.Second), $($aggregateSuccess.Third)) instead of (11, 22, 33)."
        }
        if (($aggregateInvalid.First -ne 0) -or
            ($aggregateInvalid.Second -ne 0) -or
            ($aggregateInvalid.Third -ne 0)) {
            throw "Hosted aggregate-return invalid-handle catch path returned ($($aggregateInvalid.First), $($aggregateInvalid.Second), $($aggregateInvalid.Third)) instead of (0, 0, 0)."
        }
        if (($aggregatePanic.First -ne 0) -or
            ($aggregatePanic.Second -ne 0) -or
            ($aggregatePanic.Third -ne 0)) {
            throw "Hosted aggregate-return panic catch path returned ($($aggregatePanic.First), $($aggregatePanic.Second), $($aggregatePanic.Third)) instead of (0, 0, 0)."
        }
    }
    finally {
        $destroyAggregate = [HostedSharedNative]::SessionDestroy($aggregateSession)
        if ($destroyAggregate -ne 1) {
            throw "Hosted shared library failed to destroy aggregate-return test session."
        }
    }

    $rawHandleA = [RawSharedNative]::LoadLibraryW($rawSharedDll)
    if ($rawHandleA -eq [IntPtr]::Zero) {
        $rawLoadError = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
        throw "Raw shared library failed to load (first load), win32=$rawLoadError."
    }

    try {
        $rawNextA = [RawSharedNative]::BindNext($rawHandleA)
        $rawFirst = $rawNextA.Invoke()
        $rawSecond = $rawNextA.Invoke()

        if ($rawFirst -ne 41) {
            throw "Raw shared library first metric returned $rawFirst instead of 41."
        }
        if ($rawSecond -ne 42) {
            throw "Raw shared library second metric returned $rawSecond instead of 42."
        }
    }
    finally {
        if (-not [RawSharedNative]::FreeLibrary($rawHandleA)) {
            $rawFreeError = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
            throw "Raw shared library failed to unload after first load, win32=$rawFreeError."
        }
    }

    $rawHandleB = [RawSharedNative]::LoadLibraryW($rawSharedDll)
    if ($rawHandleB -eq [IntPtr]::Zero) {
        $rawReloadError = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
        throw "Raw shared library failed to load (second load), win32=$rawReloadError."
    }

    try {
        $rawNextB = [RawSharedNative]::BindNext($rawHandleB)
        $rawReloadFirst = $rawNextB.Invoke()
        if ($rawReloadFirst -ne 41) {
            throw "Raw shared library reload metric returned $rawReloadFirst instead of 41."
        }
    }
    finally {
        if (-not [RawSharedNative]::FreeLibrary($rawHandleB)) {
            $rawReloadFreeError = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
            throw "Raw shared library failed to unload after second load, win32=$rawReloadFreeError."
        }
    }

    $attachCaseRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("cursive_attach_failure_case_" + $runId)
    $null = New-Item -ItemType Directory -Path $attachCaseRoot -Force
    foreach ($dir in @("trace", "stagea", "stageb", "stagec")) {
        $null = New-Item -ItemType Directory -Path (Join-Path $attachCaseRoot $dir) -Force
    }
    $attachManifest = @(
        "[toolchain]",
        "llvm_bin = ""$llvmBinManifestPath""",
        "",
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""library""",
        "root = "".""",
        "out_dir = ""build/probe"""
    )
    [System.IO.File]::WriteAllLines((Join-Path $attachCaseRoot "Cursive.toml"), $attachManifest)
    [System.IO.File]::WriteAllText((Join-Path $attachCaseRoot "Main.cursive"), @'
import stagec

[[export("C"), mangle("hc_issue_attach_probe_ping")]]
public procedure probe_ping() -> i32 {
    return 7
}
'@)
    [System.IO.File]::WriteAllText((Join-Path $attachCaseRoot "trace\\Main.cursive"), @"
[[library(name: "kernel32", kind: "raw-dylib")]]
extern "system" {
    [[mangle(none)]]
    procedure CreateEventA(lpEventAttributes: *mut u8, bManualReset: i32, bInitialState: i32, lpName: *imm u8) -> *mut u8
    [[mangle(none)]]
    procedure SetEvent(hEvent: *mut u8) -> i32
    [[mangle(none)]]
    procedure CloseHandle(hObject: *mut u8) -> i32
}

var trace_event_1_name: [u8; 18] = [67u8, 117u8, 114u8, 115u8, 105u8, 118u8, 101u8, 65u8, 116u8, 116u8, 97u8, 99u8, 104u8, 69u8, 118u8, 116u8, 49u8, 0u8]
var trace_event_2_name: [u8; 18] = [67u8, 117u8, 114u8, 115u8, 105u8, 118u8, 101u8, 65u8, 116u8, 116u8, 97u8, 99u8, 104u8, 69u8, 118u8, 116u8, 50u8, 0u8]
var trace_event_3_name: [u8; 18] = [67u8, 117u8, 114u8, 115u8, 105u8, 118u8, 101u8, 65u8, 116u8, 116u8, 97u8, 99u8, 104u8, 69u8, 118u8, 116u8, 51u8, 0u8]
var trace_event_4_name: [u8; 18] = [67u8, 117u8, 114u8, 115u8, 105u8, 118u8, 101u8, 65u8, 116u8, 116u8, 97u8, 99u8, 104u8, 69u8, 118u8, 116u8, 52u8, 0u8]
var trace_event_5_name: [u8; 18] = [67u8, 117u8, 114u8, 115u8, 105u8, 118u8, 101u8, 65u8, 116u8, 116u8, 97u8, 99u8, 104u8, 69u8, 118u8, 116u8, 53u8, 0u8]
var trace_event_6_name: [u8; 18] = [67u8, 117u8, 114u8, 115u8, 105u8, 118u8, 101u8, 65u8, 116u8, 116u8, 97u8, 99u8, 104u8, 69u8, 118u8, 116u8, 54u8, 0u8]
var trace_event_7_name: [u8; 18] = [67u8, 117u8, 114u8, 115u8, 105u8, 118u8, 101u8, 65u8, 116u8, 116u8, 97u8, 99u8, 104u8, 69u8, 118u8, 116u8, 55u8, 0u8]
var trace_event_8_name: [u8; 18] = [67u8, 117u8, 114u8, 115u8, 105u8, 118u8, 101u8, 65u8, 116u8, 116u8, 97u8, 99u8, 104u8, 69u8, 118u8, 116u8, 56u8, 0u8]

procedure TraceEventNamePtr(value: u8) -> *imm u8 {
    if (value == 1u8) {
        let ptr_safe: Ptr<u8>@Valid = &trace_event_1_name[0usize]
        return unsafe { transmute<Ptr<u8>@Valid, *imm u8>(ptr_safe) }
    }
    if (value == 2u8) {
        let ptr_safe: Ptr<u8>@Valid = &trace_event_2_name[0usize]
        return unsafe { transmute<Ptr<u8>@Valid, *imm u8>(ptr_safe) }
    }
    if (value == 3u8) {
        let ptr_safe: Ptr<u8>@Valid = &trace_event_3_name[0usize]
        return unsafe { transmute<Ptr<u8>@Valid, *imm u8>(ptr_safe) }
    }
    if (value == 4u8) {
        let ptr_safe: Ptr<u8>@Valid = &trace_event_4_name[0usize]
        return unsafe { transmute<Ptr<u8>@Valid, *imm u8>(ptr_safe) }
    }
    if (value == 5u8) {
        let ptr_safe: Ptr<u8>@Valid = &trace_event_5_name[0usize]
        return unsafe { transmute<Ptr<u8>@Valid, *imm u8>(ptr_safe) }
    }
    if (value == 6u8) {
        let ptr_safe: Ptr<u8>@Valid = &trace_event_6_name[0usize]
        return unsafe { transmute<Ptr<u8>@Valid, *imm u8>(ptr_safe) }
    }
    if (value == 7u8) {
        let ptr_safe: Ptr<u8>@Valid = &trace_event_7_name[0usize]
        return unsafe { transmute<Ptr<u8>@Valid, *imm u8>(ptr_safe) }
    }

    let ptr_safe: Ptr<u8>@Valid = &trace_event_8_name[0usize]
    return unsafe { transmute<Ptr<u8>@Valid, *imm u8>(ptr_safe) }
}

public procedure record_event(value: u8) -> () {
    let name_ptr: *imm u8 = TraceEventNamePtr(value)
    let event_handle: *mut u8 = unsafe { CreateEventA(null, 1, 0, name_ptr) }
    if (event_handle != null) {
        let _ = unsafe { SetEvent(event_handle) }
        let _ = unsafe { CloseHandle(event_handle) }
    }
    return ()
}
"@)
    [System.IO.File]::WriteAllText((Join-Path $attachCaseRoot "stagea\\Main.cursive"), @'
import trace

record StageAGuard {
    value: i32
    token: unique i32

    procedure drop(~!) -> () {
        trace::record_event(5u8)
        return ()
    }
}

procedure BuildStageAGuard() -> StageAGuard {
    trace::record_event(1u8)
    return StageAGuard { value: 1, token: 11 }
}

var stage_a_guard: StageAGuard = BuildStageAGuard()

public procedure stagea_ready() -> i32 {
    return stage_a_guard.value
}
'@)
    [System.IO.File]::WriteAllText((Join-Path $attachCaseRoot "stageb\\Main.cursive"), @'
import stagea
import trace

record StageBGuard {
    value: i32
    token: unique i32

    procedure drop(~!) -> () {
        trace::record_event(4u8)
        return ()
    }
}

procedure BuildStageBGuard() -> StageBGuard {
    let _ = stagea::stagea_ready()
    trace::record_event(2u8)
    return StageBGuard { value: 2, token: 22 }
}

record StageBPanicGuard {
    value: i32
    token: unique i32

    procedure drop(~!) -> () {
        trace::record_event(6u8)
        return ()
    }
}

procedure BuildStageBPanic() -> StageBPanicGuard {
    trace::record_event(3u8)
    let ptr_null: Ptr<i32>@Null = Ptr::null()
    let ptr_valid: Ptr<i32>@Valid =
        unsafe { transmute<Ptr<i32>@Null, Ptr<i32>@Valid>(ptr_null) }
    return StageBPanicGuard { value: *ptr_valid, token: 33 }
}

var stage_b_guard: StageBGuard = BuildStageBGuard()
var stage_b_panic: StageBPanicGuard = BuildStageBPanic()

public procedure stageb_ready() -> i32 {
    return stage_b_guard.value
}
'@)
    [System.IO.File]::WriteAllText((Join-Path $attachCaseRoot "stagec\\Main.cursive"), @'
import stageb
import trace

record StageCGuard {
    value: i32
    token: unique i32

    procedure drop(~!) -> () {
        trace::record_event(8u8)
        return ()
    }
}

procedure BuildStageCGuard() -> StageCGuard {
    let _ = stageb::stageb_ready()
    trace::record_event(7u8)
    return StageCGuard { value: 3, token: 44 }
}

var stage_c_guard: StageCGuard = BuildStageCGuard()
'@)

    $attachDiagPath = Join-Path $runLogDir "attach_failure_diag.json"
    $attachStderrPath = Join-Path $runLogDir "attach_failure_stderr.txt"
    Push-Location $attachCaseRoot
    try {
        & $CompilerPath --incremental off --diag-json --quiet --assembly probe Main.cursive 1> $attachDiagPath 2> $attachStderrPath
        $attachBuildExit = $LASTEXITCODE
    }
    finally {
        Pop-Location
    }
    if ($attachBuildExit -ne 0) {
        throw "Attach-failure probe build failed with exit $attachBuildExit."
    }

    $probeDll = Join-Path $attachCaseRoot "build\\probe\\bin\\probe.dll"
    if (-not (Test-Path $probeDll)) {
        throw "Attach-failure probe missing artifact: $probeDll"
    }

    $attachRuntimePathEntries = @(
        (Join-Path $attachCaseRoot "build\\probe\\bin")
    )
    $previousAttachPath = $env:PATH
    $env:PATH = (($attachRuntimePathEntries + @($previousAttachPath)) -join ";")

    try {
        $probeHandle = [RawSharedNative]::LoadLibraryW($probeDll)
        $probeLoadError = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
        if ($probeHandle -ne [IntPtr]::Zero) {
            try {
                throw "Attach-failure probe library unexpectedly loaded, win32=$probeLoadError."
            }
            finally {
                $null = [RawSharedNative]::FreeLibrary($probeHandle)
            }
        }
        if ($probeLoadError -ne 1114) {
            throw "Attach-failure probe returned win32=$probeLoadError instead of ERROR_DLL_INIT_FAILED (1114)."
        }
    }
    finally {
        $env:PATH = $previousAttachPath
    }
    }
    finally {
        $env:PATH = $previousHostedPath
    }
}
exit $runExit
