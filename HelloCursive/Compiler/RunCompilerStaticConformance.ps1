param(
    [string]$CompilerPath = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# -----------------------------------------------------------------------------
# SCOPE POLICY (ENFORCED)
# -----------------------------------------------------------------------------
# This harness is strictly for compiler static/conformance verification:
# - parse/resolve/typecheck diagnostics
# - static rule/spec trace coverage
# - compiler conformance trace validation
#
# Do NOT add runtime/dynamic/semantic execution checks here.
# Runtime-facing validation must live in the HelloCursive runtime path
# (RunHelloCursive.ps1 / main.exe via module routers).
# -----------------------------------------------------------------------------

if ([string]::IsNullOrWhiteSpace($CompilerPath)) {
    $repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
    $workspaceRoot = (Resolve-Path (Join-Path $repoRoot "..")).Path
    $resolveCompilerPath = Join-Path $repoRoot "ResolveCompilerPath.ps1"
    $CompilerPath = (& $resolveCompilerPath -RepoRoot $workspaceRoot -RequestedPath $CompilerPath)
}

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$workspaceRoot = (Resolve-Path (Join-Path $repoRoot "..")).Path

function Get-CursiveSpecificationPath([string]$Root) {
    $resolverPath = Join-Path $Root "cursive\\tools\\resolve_spec_path.ps1"
    if (-not (Test-Path $resolverPath)) {
        throw "Static conformance harness missing spec resolver: $resolverPath"
    }

    $resolved = (& $resolverPath -RepoRoot $Root)
    if ([string]::IsNullOrWhiteSpace($resolved)) {
        throw "Static conformance harness could not resolve canonical language spec."
    }

    return (Resolve-Path $resolved).Path
}

$canonicalSpecPath = Get-CursiveSpecificationPath -Root $workspaceRoot
$runId = Get-Date -Format "yyyyMMdd_HHmmss_fff"
$logsRoot = Join-Path $workspaceRoot "build\\logs\\HelloCursive\\Compiler"
New-Item -ItemType Directory -Path $logsRoot -Force | Out-Null
$scratchRoot = Join-Path ([System.IO.Path]::GetTempPath()) "cursive_static_conformance"
New-Item -ItemType Directory -Path $scratchRoot -Force | Out-Null
$workRoot = Join-Path $scratchRoot ("tmp_compiler_static_conformance_" + $runId)
New-Item -ItemType Directory -Path $workRoot -Force | Out-Null
$runLogRoot = Join-Path $logsRoot ("compiler_static_conformance_" + $runId)
New-Item -ItemType Directory -Path $runLogRoot -Force | Out-Null
$runCasesLogRoot = Join-Path $runLogRoot "cases"
New-Item -ItemType Directory -Path $runCasesLogRoot -Force | Out-Null

$manifestLines = @(
    "[toolchain]",
    "target_profile = ""x86_64-win64""",
    "",
    "[[assembly]]",
    "name = ""probe""",
    "kind = ""executable""",
    "root = "".""",
    "out_dir = ""build/probe"""
)

function Convert-ToCursiveMangle([string]$Text) {
    $bytes = [System.Text.Encoding]::UTF8.GetBytes($Text)
    $builder = New-Object System.Text.StringBuilder
    foreach ($byte in $bytes) {
        if ((($byte -ge [byte][char]'0') -and ($byte -le [byte][char]'9')) -or
            (($byte -ge [byte][char]'A') -and ($byte -le [byte][char]'Z')) -or
            (($byte -ge [byte][char]'a') -and ($byte -le [byte][char]'z'))) {
            [void]$builder.Append([char]$byte)
        }
        else {
            [void]$builder.AppendFormat("_x{0:x2}", [int]$byte)
        }
    }
    return $builder.ToString()
}

function Get-CursiveHostBodySymbol([string[]]$PathComponents) {
    $scoped = Convert-ToCursiveMangle([string]::Join("::", $PathComponents))
    return Convert-ToCursiveMangle([string]::Join("::", @($scoped, "__host_body")))
}

function Get-PeAddressOfEntryPoint([string]$Path) {
    $bytes = [System.IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 0x40) {
        throw "PE image too small to contain DOS header: $Path"
    }

    $e_lfanew = [System.BitConverter]::ToInt32($bytes, 0x3C)
    if (($e_lfanew -lt 0) -or (($e_lfanew + 0x2C) -ge $bytes.Length)) {
        throw "PE image has invalid e_lfanew: $Path"
    }

    $peSignature = [System.BitConverter]::ToUInt32($bytes, $e_lfanew)
    if ($peSignature -ne 0x00004550) {
        throw "PE image missing PE signature: $Path"
    }

    $optionalHeaderOffset = $e_lfanew + 24
    return [System.BitConverter]::ToUInt32($bytes, $optionalHeaderOffset + 16)
}

function Get-CoffExportNames([string]$Path) {
    $llvmReadObj = Get-LlvmToolPath "llvm-readobj"
    $output = (Invoke-ExternalToolCapture `
        -ToolPath $llvmReadObj `
        -Args @("--coff-exports", $Path) `
        -FailureLabel "llvm-readobj --coff-exports '$Path'").StdoutText

    $names = New-Object System.Collections.Generic.List[string]
    foreach ($match in [regex]::Matches($output, 'Name:\s+([^\r\n]+)')) {
        $names.Add($match.Groups[1].Value.Trim())
    }
    return $names.ToArray()
}

function Get-LlvmReadObjText {
    param(
        [Alias("Args")]
        [string[]]$ToolArgs,
        [string]$FailureLabel
    )

    $toolPath = Get-LlvmToolPath "llvm-readobj.exe"
    $normalizedArgs = @()
    foreach ($arg in $ToolArgs) {
        $normalizedArgs += (Convert-ToLlvmToolArg $arg)
    }
    return (Invoke-ExternalToolCapture `
        -ToolPath $toolPath `
        -Args $normalizedArgs `
        -FailureLabel $FailureLabel).StdoutText
}

function Get-ElfDynamicSymbolNames([string]$Path) {
    $output = Get-LlvmReadObjText `
        -Args @("--dyn-symbols", $Path) `
        -FailureLabel "llvm-readobj --dyn-symbols '$Path'"
    $names = New-Object System.Collections.Generic.List[string]
    foreach ($match in [regex]::Matches($output, '(?m)^\s*Name:\s+([^\r\n]+)$')) {
        $name = $match.Groups[1].Value.Trim()
        if ([string]::IsNullOrWhiteSpace($name) -or $name -eq "(0)") {
            continue
        }
        $names.Add($name)
    }
    return $names.ToArray()
}

function Get-LinkDebugCommands([string]$StderrPath) {
    $stderrText = Get-Content -Path $StderrPath -Raw
    $commands = New-Object System.Collections.Generic.List[string]
    foreach ($match in [regex]::Matches($stderrText, '(?m)^(?:[^:\r\n]+(?:\.exe)?\s*:\s+)?\[link-debug\] cmd=(.+)$')) {
        $commands.Add($match.Groups[1].Value.Trim())
    }
    return $commands.ToArray()
}

function Get-LinkDebugLines([string]$StderrPath) {
    $stderrText = Get-Content -Path $StderrPath -Raw
    $lines = New-Object System.Collections.Generic.List[string]
    foreach ($match in [regex]::Matches($stderrText, '(?m)^(?:[^:\r\n]+(?:\.exe)?\s*:\s+)?(\[link-debug\][^\r\n]*)$')) {
        $lines.Add($match.Groups[1].Value.Trim())
    }
    return $lines.ToArray()
}

function Assert-FreshMapSidecar([string]$CaseName, [string]$ExePath, [string]$LinkCommand = "") {
    $mapPath = [System.IO.Path]::ChangeExtension($ExePath, ".map")
    if (-not [string]::IsNullOrWhiteSpace($LinkCommand)) {
        $expectedMapArg = "/MAP:$mapPath"
        if ($LinkCommand -notmatch [regex]::Escape($expectedMapArg)) {
            throw "Case '$CaseName' expected linker command to contain $expectedMapArg."
        }
    }
    if (-not (Test-Path $mapPath)) {
        throw "Case '$CaseName' missing map artifact: $mapPath"
    }
    $exeItem = Get-Item $ExePath
    $mapItem = Get-Item $mapPath
    if ($mapItem.LastWriteTimeUtc.AddSeconds(5) -lt $exeItem.LastWriteTimeUtc) {
        throw "Case '$CaseName' expected map artifact '$mapPath' to be refreshed with '$ExePath'."
    }
}

function Assert-BuildFailureResult {
    param(
        [pscustomobject]$Result,
        [string]$CaseId,
        [string[]]$ExpectedCodes,
        [string[]]$ForbiddenCodes = @()
    )

    if ($Result.ExitCode -ne 1) {
        throw "Case '$CaseId' expected exit 1 but got $($Result.ExitCode)."
    }

    $errorCount = @($Result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -lt 1) {
        throw "Case '$CaseId' expected at least one compile-time error."
    }

    foreach ($expectedCode in $ExpectedCodes) {
        $ifIsCount = @($Result.DiagJson.diagnostics | Where-Object {
            $_.code -eq $expectedCode
        }).Count
        if ($ifIsCount -lt 1) {
            throw "Case '$CaseId' expected diagnostic code '$expectedCode'."
        }
    }

    foreach ($forbiddenCode in $ForbiddenCodes) {
        $forbiddenCount = @($Result.DiagJson.diagnostics | Where-Object {
            $_.code -eq $forbiddenCode
        }).Count
        if ($forbiddenCount -gt 0) {
            throw "Case '$CaseId' unexpectedly produced diagnostic code '$forbiddenCode'."
        }
    }
}

function Get-BuildProgressAssemblies([string]$StderrPath) {
    $stderrText = Get-Content -Path $StderrPath -Raw
    $assemblies = New-Object System.Collections.Generic.List[string]
    foreach ($match in [regex]::Matches($stderrText, '(?m)^\s*Compiling\s+([^\s]+)\s+\(')) {
        $assemblies.Add($match.Groups[1].Value.Trim())
    }
    return $assemblies.ToArray()
}

function Test-IsNativePosixHost() {
    return [System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Linux) -or
        [System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::OSX)
}

function Get-LlvmToolPath([string]$ToolName) {
    $candidates = New-Object System.Collections.Generic.List[string]
    $candidates.Add($ToolName)
    if ($ToolName.EndsWith(".exe", [System.StringComparison]::OrdinalIgnoreCase)) {
        $candidates.Add($ToolName.Substring(0, $ToolName.Length - 4))
    }
    else {
        $candidates.Add($ToolName + ".exe")
    }

    foreach ($candidate in $candidates) {
        $toolPath = Join-Path $workspaceRoot ("extern\llvm\llvm-21.1.8-x86_64\bin\" + $candidate)
        if (Test-Path $toolPath) {
            return $toolPath
        }
    }
    throw "LLVM tool not found under extern\\llvm\\llvm-21.1.8-x86_64\\bin for '$ToolName'."
}

function Convert-ToLlvmToolArg([string]$Value) {
    if ([string]::IsNullOrWhiteSpace($Value)) {
        return $Value
    }
    if ($Value.StartsWith("-")) {
        return $Value
    }
    if (Test-Path $Value) {
        return (Resolve-Path $Value).Path
    }
    return $Value
}

function Invoke-LlvmToolText {
    param(
        [string]$ToolName,
        [string[]]$ToolArgs,
        [string]$FailureLabel
    )

    $toolPath = Get-LlvmToolPath $ToolName
    $normalizedToolArgs = @()
    foreach ($toolArg in $ToolArgs) {
        $normalizedToolArgs += (Convert-ToLlvmToolArg $toolArg)
    }
    return (Invoke-ExternalToolCapture `
        -ToolPath $toolPath `
        -Args $normalizedToolArgs `
        -FailureLabel $FailureLabel).StdoutText
}

function Invoke-ExternalToolCapture {
    param(
        [string]$ToolPath,
        [Alias("Args")]
        [string[]]$ToolArgs,
        [string]$FailureLabel
    )

    $cleanArgs = @()
    foreach ($arg in $ToolArgs) {
        if ($null -eq $arg) {
            continue
        }
        $argText = [string]$arg
        if ([string]::IsNullOrWhiteSpace($argText)) {
            continue
        }
        $cleanArgs += $argText
    }

    $quoteArg = {
        param([string]$Value)

        if ([string]::IsNullOrEmpty($Value)) {
            return '""'
        }
        if ($Value -notmatch '[\s"]') {
            return $Value
        }

        $builder = New-Object System.Text.StringBuilder
        [void]$builder.Append('"')
        $backslashCount = 0
        foreach ($ch in $Value.ToCharArray()) {
            if ($ch -eq '\') {
                $backslashCount += 1
                continue
            }
            if ($ch -eq '"') {
                [void]$builder.Append('\', ($backslashCount * 2) + 1)
                [void]$builder.Append('"')
                $backslashCount = 0
                continue
            }
            if ($backslashCount -gt 0) {
                [void]$builder.Append('\', $backslashCount)
                $backslashCount = 0
            }
            [void]$builder.Append($ch)
        }
        if ($backslashCount -gt 0) {
            [void]$builder.Append('\', $backslashCount * 2)
        }
        [void]$builder.Append('"')
        return $builder.ToString()
    }

    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $ToolPath
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.CreateNoWindow = $true
    $psi.Arguments = (($cleanArgs | ForEach-Object { & $quoteArg $_ }) -join " ")

    $proc = New-Object System.Diagnostics.Process
    $proc.StartInfo = $psi
    [void]$proc.Start()
    $stdoutText = $proc.StandardOutput.ReadToEnd()
    $stderrText = $proc.StandardError.ReadToEnd()
    $proc.WaitForExit()

    if ($proc.ExitCode -ne 0) {
        $details = ""
        if (-not [string]::IsNullOrWhiteSpace($stderrText)) {
            $details = $stderrText.TrimEnd()
        } elseif (-not [string]::IsNullOrWhiteSpace($stdoutText)) {
            $details = $stdoutText.TrimEnd()
        }
        if ([string]::IsNullOrWhiteSpace($details)) {
            $details = "exit=$($proc.ExitCode)"
        }
        throw "$FailureLabel failed.`ntool=$ToolPath`nargs=$($psi.Arguments)`n$details"
    }

    return [PSCustomObject]@{
        StdoutText = $stdoutText
        StderrText = $stderrText
        ExitCode = $proc.ExitCode
    }
}

function New-FakeLlvmAsExecutable {
    param(
        [string]$OutputPath,
        [string]$ReportedVersion,
        [switch]$FailAssemble
    )

    if (Test-Path $OutputPath) {
        Remove-Item $OutputPath -Force
    }

    $escapedVersion = $ReportedVersion.Replace('\', '\\').Replace('"', '\"')
    $typeName = "FakeLlvmAs_" + [Guid]::NewGuid().ToString("N")
    $assembleBody = if ($FailAssemble) {
@"
        return 1;
"@
    } else {
@"
        if (args.Length >= 3) {
            for (int i = 0; i + 1 < args.Length; ++i) {
                if (args[i] == "-o") {
                    File.WriteAllBytes(args[i + 1], Array.Empty<byte>());
                    return 0;
                }
            }
        }

        return 1;
"@
    }
    $source = @"
using System;
using System.IO;

public static class $typeName {
    public static int Main(string[] args) {
        if (args.Length == 1 && args[0] == "--version") {
            Console.WriteLine("LLVM version $escapedVersion");
            return 0;
        }

$assembleBody
    }
}
"@

    Add-Type -TypeDefinition $source -Language CSharp -OutputAssembly $OutputPath -OutputType ConsoleApplication | Out-Null
}

function Get-ObjdumpSymbolBlock {
    param(
        [string]$DisassemblyText,
        [string]$Symbol,
        [string]$CaseId
    )

    $match = [regex]::Match(
        $DisassemblyText,
        "(?ms)^[0-9A-Fa-f]+\s+<" + [regex]::Escape($Symbol) + ">:\r?\n(.*?)(?=^[0-9A-Fa-f]+\s+<|\z)")
    if (-not $match.Success) {
        throw "Case '$CaseId' missing disassembly block for symbol '$Symbol'."
    }
    return $match.Groups[1].Value
}

function New-BulkErrorSource([int]$Count) {
    $lines = New-Object System.Collections.Generic.List[string]
    $lines.Add("public procedure main(move ctx: Context) -> i32 {")
    $lines.Add("    let _ = ctx")
    $lines.Add("    return 0")
    $lines.Add("}")
    for ($i = 0; $i -lt $Count; $i++) {
        $lines.Add("public procedure probe_${i}() -> i32 {")
        $lines.Add("    let mismatch_${i}: i32 = true")
        $lines.Add("    return 0")
        $lines.Add("}")
    }
    return [string]::Join("`n", $lines)
}

function New-StaticCheckSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let mismatch: i32 = true
    return 0
}
"@
}

function New-SystemCtorSafeSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let _sys = System()
    return 0
}
"@
}

function New-SystemRecordLiteralSafeSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let _sys = System{}
    return 0
}
"@
}

function New-NoAmbientExplicitContextFlowSource() {
    return @"
procedure helper(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    return helper(move ctx)
}
"@
}

function New-ExportWrappedCapabilitySource() {
    return @"
[[layout(C)]]
record ContextPtrBox {
    ptr: *imm Context
}

[[mangle("hc_bad_export_wrapped_context_ptr"), export("C")]]
public procedure bad_export(move box: ContextPtrBox) -> i32 {
    let _ = box
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-ParseErrorSource([int]$Count) {
    $lines = New-Object System.Collections.Generic.List[string]
    for ($i = 0; $i -lt $Count; $i++) {
        $lines.Add("public procedure malformed_${i}( -> i32 {")
        $lines.Add("    return 0")
        $lines.Add("}")
    }
    return [string]::Join("`n", $lines)
}

function New-ResolveErrorSource([int]$Count) {
    $lines = New-Object System.Collections.Generic.List[string]
    $lines.Add("public procedure main(move ctx: Context) -> i32 {")
    $lines.Add("    let _ = ctx")
    for ($i = 0; $i -lt $Count; $i++) {
        $lines.Add("    let unresolved_${i}: i32 = missing_symbol_${i}")
    }
    $lines.Add("    return 0")
    $lines.Add("}")
    return [string]::Join("`n", $lines)
}

function New-MainMissingSource() {
    return @"
public procedure helper(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-OrderedDiagnosticsSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
public procedure probe_first() -> i32 {
    let mismatch_first: i32 = true
    return 0
}

public procedure probe_second() -> i32 {
    let mismatch_second: i32 = true
    return 0
}
"@
}

function New-MinimalMainSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue515SystemGetEnvSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let present: string@View = ctx.sys~>get_env("SystemRoot")
    let missing: string@View = ctx.sys~>get_env("CURSIVE_SENTINEL_ABSENT_6D5D4754A0CB4C4E8A33E4BC1CDEF001")
    if (string::is_empty(present)) {
        return 10
    }
    if (string::is_empty(missing) == false) {
        return 20
    }
    return 0
}
"@
}

function New-Issue516SystemExitSource() {
    return @'
public procedure HaltWithCode(sys: System) -> ! {
    return sys~>exit(73)
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue517SystemRunSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let status: i32 = ctx.sys~>run("echo issue517")
    let _ = status
    return 0
}
"@
}

function New-Issue518FileSystemOpenReadSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let opened_ok: bool = if ctx.fs~>open_read("probe.txt") is {
        reader: unique File@Read {
            let closed: File@Closed = reader~>close()
            let _ = closed
            true
        }
        read_err: IoError {
            let _ = read_err
            false
        }
    }
    let _ = opened_ok
    return 0
}
"@
}

function New-Issue519FileSystemOpenWriteSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let fs: $FileSystem = ctx.fs
    let probe_path: string@View = "probe.txt"
    let initial_text: string@View = "abcdef"
    let overwrite_text: string@View = "XY"
    let initial_bytes: bytes@View = bytes::view_string(initial_text)
    let overwrite_bytes: bytes@View = bytes::view_string(overwrite_text)

    let seeded: bool = if fs~>create_write(probe_path) is {
        seed_writer: unique File@Write {
            let write_result: () | IoError = seed_writer~>write(initial_bytes)
            let flush_result: () | IoError = seed_writer~>flush()
            let closed: File@Closed = seed_writer~>close()
            let _ = closed
            let write_ok: bool = if write_result is {
                seed_write_unit: () {
                    let _ = seed_write_unit
                    true
                }
                seed_write_err: IoError {
                    let _ = seed_write_err
                    false
                }
            }
            let flush_ok: bool = if flush_result is {
                seed_flush_unit: () {
                    let _ = seed_flush_unit
                    true
                }
                seed_flush_err: IoError {
                    let _ = seed_flush_err
                    false
                }
            }
            write_ok && flush_ok
        }
        seed_open_err: IoError {
            let _ = seed_open_err
            false
        }
    }
    let reopened_ok: bool = if (seeded == true) {
        if fs~>open_write(probe_path) is {
            writer: unique File@Write {
                let write_result: () | IoError = writer~>write(overwrite_bytes)
                let flush_result: () | IoError = writer~>flush()
                let closed: File@Closed = writer~>close()
                let _ = closed
                let write_ok: bool = if write_result is {
                    write_unit: () {
                        let _ = write_unit
                        true
                    }
                    write_err: IoError {
                        let _ = write_err
                        false
                    }
                }
                let flush_ok: bool = if flush_result is {
                    flush_unit: () {
                        let _ = flush_unit
                        true
                    }
                    flush_err: IoError {
                        let _ = flush_err
                        false
                    }
                }
                write_ok && flush_ok
            }
            open_err: IoError {
                let _ = open_err
                false
            }
        }
    } else {
        false
    }
    let _ = reopened_ok
    return 0
}
'@
}

function New-Issue539SharedClosureEscapeAllowedSource() {
    return @"
procedure ReadShared(value: shared i32, delta: i32) -> i32 {
    var observed: i32 = 0
    # value read {
        observed = value + delta
    }
    return observed
}

procedure MakeSharedAdder(seed: shared i32) -> |i32| -> i32 [shared: { seed: shared i32 }] {
    return |delta: i32| -> i32 {
        ReadShared(seed, delta)
    }
}

public procedure main(move ctx: Context) -> i32 {
    let backing: unique i32 = 4
    let shared_seed: shared i32 = backing
    let add = MakeSharedAdder(shared_seed)
    let _ = add(3)
    let _ = ctx
    return 0
}
"@
}

function New-Issue539SharedClosureMissingDepsSource() {
    return @"
procedure ReadShared(value: shared i32, delta: i32) -> i32 {
    var observed: i32 = 0
    # value read {
        observed = value + delta
    }
    return observed
}

procedure MakeSharedAdder(seed: shared i32) -> |i32| -> i32 {
    return |delta: i32| -> i32 {
        ReadShared(seed, delta)
    }
}

public procedure main(move ctx: Context) -> i32 {
    let backing: unique i32 = 4
    let shared_seed: shared i32 = backing
    let add = MakeSharedAdder(shared_seed)
    let _ = add(3)
    let _ = ctx
    return 0
}
"@
}

function New-Issue539SharedClosureLifetimeErrSource() {
    return @"
procedure ReadShared(value: shared i32, delta: i32) -> i32 {
    var observed: i32 = 0
    # value read {
        observed = value + delta
    }
    return observed
}

procedure MakeLocalSharedEscaper() -> |i32| -> i32 [shared: { shared_seed: shared i32 }] {
    let backing: unique i32 = 4
    let shared_seed: shared i32 = backing
    return |delta: i32| -> i32 {
        ReadShared(shared_seed, delta)
    }
}

public procedure main(move ctx: Context) -> i32 {
    let add = MakeLocalSharedEscaper()
    let _ = add(3)
    let _ = ctx
    return 0
}
"@
}

function New-Issue540InvariantResolutionSource() {
    return @'
record Issue540InvariantProbe {
    value: i32
} |: { self.value >= 0 }

procedure count_to(limit: i32) -> i32 {
    var index: i32 = 0
    loop index < limit |: { limit == limit } {
        index = index + 1
    }
    return index
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let probe: Issue540InvariantProbe = Issue540InvariantProbe { value: 3 }
    return count_to(probe.value) - 3
}
'@
}

function New-Issue542AsyncAliasSubtypingConformanceSource() {
    return @'
type UserFuture<T> = Future<T>

procedure MakeAlias() -> UserFuture<i32> {
    return 7
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let alias_value: UserFuture<i32> = MakeAlias()
    let widened: Async<(), (), i32, !> = alias_value
    let result: i32 = sync widened
    return result - 7
}
'@
}

function New-Issue543RefinementAliasInteropSource() {
    return @'
type PositiveI32 = i32 |: { self > 0 }
type PositiveI32Alias = i32 |: { self > 0 }

type PositiveUnderTenI32 = i32 |: { (self > 0) && (self < 10) }

procedure accept_alias(value: PositiveI32Alias) -> i32 {
    return value
}

procedure accept_positive(value: PositiveI32) -> i32 {
    return value
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let positive: PositiveI32 = 7
    let bounded: PositiveUnderTenI32 = 7
    let alias_metric: i32 = accept_alias(positive)
    let widened_metric: i32 = accept_positive(bounded)
    return (alias_metric + widened_metric) - 14
}
'@
}

function New-Issue543RefinementNarrowingRejectSource() {
    return @'
type PositiveI32 = i32 |: { self > 0 }
type PositiveUnderTenI32 = i32 |: { (self > 0) && (self < 10) }

procedure accept_bounded(value: PositiveUnderTenI32) -> i32 {
    return value
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let positive: PositiveI32 = 7
    let out: i32 = accept_bounded(positive)
    return out
}
'@
}

function New-Issue521FileSystemCreateWriteSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let created_ok: bool = if ctx.fs~>create_write("probe.txt") is {
        writer: unique File@Write {
            let closed: File@Closed = writer~>close()
            let _ = closed
            true
        }
        create_err: IoError {
            let _ = create_err
            false
        }
    }
    let _ = created_ok
    return 0
}
'@
}

function New-Issue522FileSystemReadFileSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let read_length: i32 = if ctx.fs~>read_file("probe.txt") is {
        read_text: string@Managed {
            let read_view: string@View = string::as_view(read_text)
            string::length(read_view) as i32
        }
        read_err: IoError {
            let _ = read_err
            0
        }
    }
    let _ = read_length
    return 0
}
"@
}

function New-Issue523FileSystemReadBytesSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let read_length: i32 = if ctx.fs~>read_bytes("probe.bin") is {
        read_bytes: unique bytes@Managed {
            let read_view: bytes@View = bytes::as_view(read_bytes)
            bytes::length(read_view) as i32
        }
        read_err: IoError {
            let _ = read_err
            0
        }
    }
    let _ = read_length
    return 0
}
"@
}

function New-Issue524FileSystemWriteFileSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let payload: bytes@View = bytes::view_string("payload")
    let wrote_ok: bool = if ctx.fs~>write_file("probe.bin", payload) is {
        write_unit: () {
            let _ = write_unit
            true
        }
        write_err: IoError {
            let _ = write_err
            false
        }
    }
    let _ = wrote_ok
    return 0
}
"@
}

function New-Issue525FileSystemWriteStdoutSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let wrote_ok: bool = if ctx.fs~>write_stdout("issue525-file-system-write-stdout") is {
        write_unit: () {
            let _ = write_unit
            true
        }
        write_err: IoError {
            let _ = write_err
            false
        }
    }
    let _ = wrote_ok
    return 0
}
"@
}

function New-Issue526FileSystemWriteStderrSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let wrote_ok: bool = if ctx.fs~>write_stderr("issue526-file-system-write-stderr") is {
        write_unit: () {
            let _ = write_unit
            true
        }
        write_err: IoError {
            let _ = write_err
            false
        }
    }
    let _ = wrote_ok
    return 0
}
"@
}

function New-Issue527FileSystemExistsSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let fs: $FileSystem = ctx.fs
    let present: bool = fs~>exists("probe.txt")
    let missing: bool = fs~>exists("missing.txt")
    let restricted: $FileSystem = fs~>restrict("sandbox")
    let escaped_missing: bool = restricted~>exists("../outside.txt")
    let _ = present
    let _ = missing
    let _ = escaped_missing
    return 0
}
'@
}

function New-Issue528FileSystemRemoveSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let fs: $FileSystem = ctx.fs
    let removed_ok: bool = if fs~>remove("probe.txt") is {
        remove_unit: () {
            let _ = remove_unit
            true
        }
        remove_err: IoError {
            let _ = remove_err
            false
        }
    }
    let restricted: $FileSystem = fs~>restrict("sandbox")
    let escaped_remove_error: bool = if restricted~>remove("../outside.txt") is {
        restricted_unit: () {
            let _ = restricted_unit
            false
        }
        restricted_err: IoError {
            let _ = restricted_err
            true
        }
    }
    let _ = removed_ok
    let _ = escaped_remove_error
    return 0
}
'@
}

function New-Issue529FileSystemOpenDirSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let opened_ok: bool = if ctx.fs~>open_dir("probe-dir") is {
        iter: unique DirIter@Open {
            let next_result: DirEntry | () | IoError = iter~>next()
            let closed: DirIter@Closed = iter~>close()
            let _ = closed
            if next_result is {
                entry: DirEntry {
                    let entry_kind_ok: bool = if entry.kind is {
                        FileKind::File { true }
                        FileKind::Dir { true }
                        FileKind::Other { true }
                    }
                    let entry_name_view: string@View = string::as_view(entry.name)
                    let entry_path_view: string@View = string::as_view(entry.path)
                    let _ = entry_name_view
                    let _ = entry_path_view
                    entry_kind_ok
                }
                empty_entry: () {
                    let _ = empty_entry
                    true
                }
                next_err: IoError {
                    let _ = next_err
                    false
                }
            }
        }
        open_err: IoError {
            let _ = open_err
            false
        }
    }
    let _ = opened_ok
    return 0
}
'@
}

function New-Issue530FileSystemCreateDirSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let fs: $FileSystem = ctx.fs
    let created_ok: bool = if fs~>create_dir("probe-dir") is {
        created_unit: () {
            let _ = created_unit
            true
        }
        create_err: IoError {
            let _ = create_err
            false
        }
    }
    let _ = created_ok
    return 0
}
'@
}

function New-Issue531FileSystemEnsureDirSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let fs: $FileSystem = ctx.fs
    let ensured_ok: bool = if fs~>ensure_dir("probe-dir/nested") is {
        ensured_unit: () {
            let _ = ensured_unit
            true
        }
        ensure_err: IoError {
            let _ = ensure_err
            false
        }
    }
    let _ = ensured_ok
    return 0
}
'@
}

function New-Issue532FileSystemKindSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let fs: $FileSystem = ctx.fs
    let kind_ok: bool = if fs~>kind("probe-path") is {
        kind: FileKind {
            if kind is {
                FileKind::Dir { true }
                FileKind::File { true }
                FileKind::Other { true }
            }
        }
        kind_err: IoError {
            let _ = kind_err
            false
        }
    }
    let _ = kind_ok
    return 0
}
'@
}

function New-Issue533FileSystemRestrictSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let fs: $FileSystem = ctx.fs
    let sandbox: $FileSystem = fs~>restrict("sandbox")
    let nested: $FileSystem = sandbox~>restrict("nested")
    let parent_probe: bool = fs~>exists("sandbox")
    let child_probe: bool = nested~>exists("probe.txt")
    let _ = parent_probe
    let _ = child_probe
    return 0
}
'@
}

function New-Issue534FileModalReadConformanceSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let fs: $FileSystem = ctx.fs
    let read_ok: bool = if fs~>open_read("probe.txt") is {
        reader: unique File@Read {
            let text_result: string@Managed | IoError = reader~>read_all()
            let bytes_result: bytes@Managed | IoError = reader~>read_all_bytes()
            let closed: File@Closed = reader~>close()
            let _ = closed
            let text_ok: bool = if text_result is {
                _: string@Managed { true }
                _: IoError { false }
            }
            let bytes_ok: bool = if bytes_result is {
                _: bytes@Managed { true }
                _: IoError { false }
            }
            text_ok || bytes_ok
        }
        _: IoError { false }
    }
    let _ = read_ok
    return 0
}
'@
}

function New-Issue535FileModalWriteConformanceSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let fs: $FileSystem = ctx.fs
    let write_ok: bool = if fs~>create_write("probe.txt") is {
        writer: unique File@Write {
            let payload: bytes@View = bytes::view_string("probe")
            let write_result: () | IoError = writer~>write(payload)
            let flush_result: () | IoError = writer~>flush()
            let closed: File@Closed = writer~>close()
            let _ = closed
            let write_done: bool = if write_result is {
                _: () { true }
                _: IoError { false }
            }
            let flush_done: bool = if flush_result is {
                _: () { true }
                _: IoError { false }
            }
            write_done && flush_done
        }
        _: IoError { false }
    }
    let _ = write_ok
    return 0
}
'@
}

function New-Issue536FileModalAppendConformanceSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let fs: $FileSystem = ctx.fs
    let append_ok: bool = if fs~>open_append("probe.txt") is {
        appender: unique File@Append {
            let payload: bytes@View = bytes::view_string("probe")
            let write_result: () | IoError = appender~>write(payload)
            let flush_result: () | IoError = appender~>flush()
            let closed: File@Closed = appender~>close()
            let _ = closed
            let write_done: bool = if write_result is {
                _: () { true }
                _: IoError { false }
            }
            let flush_done: bool = if flush_result is {
                _: () { true }
                _: IoError { false }
            }
            write_done && flush_done
        }
        _: IoError { false }
    }
    let _ = append_ok
    return 0
}
'@
}

function New-Issue537DirIterPrimitiveConformanceSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let fs: $FileSystem = ctx.fs
    let dir_ok: bool = if fs~>open_dir("probe-dir") is {
        iter: unique DirIter@Open {
            let next_result: DirEntry | () | IoError = iter~>next()
            let closed: DirIter@Closed = iter~>close()
            let _ = closed
            if next_result is {
                _: DirEntry { true }
                _: () { true }
                _: IoError { false }
            }
        }
        _: IoError { false }
    }
    let _ = dir_ok
    return 0
}
'@
}

function New-Issue538NetworkRestrictHostConformanceSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let net: $Network = ctx.net
    let service_only: $Network = net~>restrict_to_host("service.example")
    let admin_only: $Network = net~>restrict_to_host("admin.example")
    let nested: $Network = service_only~>restrict_to_host("service.example")
    let _ = admin_only
    let _ = nested
    return 0
}
'@
}

function New-Issue520FileSystemOpenAppendSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let fs: $FileSystem = ctx.fs
    let appended_ok: bool = if fs~>open_append("probe.txt") is {
        appender: unique File@Append {
            let payload: bytes@View = bytes::view_string("-tail")
            let write_result: () | IoError = appender~>write(payload)
            let flush_result: () | IoError = appender~>flush()
            let closed: File@Closed = appender~>close()
            let _ = closed
            let write_ok: bool = if write_result is {
                write_unit: () {
                    let _ = write_unit
                    true
                }
                write_err: IoError {
                    let _ = write_err
                    false
                }
            }
            let flush_ok: bool = if flush_result is {
                flush_unit: () {
                    let _ = flush_unit
                    true
                }
                flush_err: IoError {
                    let _ = flush_err
                    false
                }
            }
            write_ok && flush_ok
        }
        append_err: IoError {
            let _ = append_err
            false
        }
    }
    let _ = appended_ok
    return 0
}
'@
}

function New-Issue27ExportCatchWrongAbiSource() {
    return @"
[[export("C"), unwind("catch")]]
public procedure exported_wrong_abi() -> i32 {
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue27ExportUnknownAbiSource() {
    return @"
[[export("NotARealAbi")]]
public procedure exported_unknown_abi() -> i32 {
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue27FfiSurfaceSource() {
    return @"
extern "C" {
    procedure imported_default() -> i32
}

[[export("C")]]
public procedure exported_default() -> i32 {
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue513MangleNonFfiSource() {
    return @'
[[mangle("plain_symbol")]]
procedure plain_symbol() -> i32 {
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return plain_symbol()
}
'@
}

function New-Issue513MangleInvalidSource() {
    return @'
[[mangle(""), export("C")]]
public procedure empty_symbol() -> i32 {
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513DynamicDispatchEligibilitySource() {
    return @'
public class DynBase {
    procedure only_static<T>(~, value: T) -> T {
        return value
    }
}

procedure call_on_dynamic(target: $DynBase) -> i32 {
    return target~>only_static(9)
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513UnknownExternVerificationAttrSource() {
    return @'
[[trust]]
extern "C" {
    procedure unknown_verification_probe(value: i32) -> i32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513WeakAttributeRejectedSource() {
    return @'
[[weak]]
procedure weak_probe() -> i32 {
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return weak_probe()
}
'@
}

function New-Issue513ExternMangleMalformedSource() {
    return @'
extern "C" {
    [[mangle]]
    procedure foreign_bad_mangle() -> i32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513ExternMangleMultiArgRejectedSource() {
    return @'
extern "C" {
    [[mangle(none, "conflict_symbol")]]
    procedure foreign_mangle_multi_arg() -> i32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513ExternDuplicateExplicitMangleSource() {
    return @'
extern "C" {
    [[mangle("dup_extern_symbol")]]
    procedure foreign_dup_a() -> i32

    [[mangle("dup_extern_symbol")]]
    procedure foreign_dup_b() -> i32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513ConflictingMangleDirectivesSource() {
    return @'
[[mangle(none), mangle("conflicting_symbol"), export("C")]]
public procedure conflicting_mangle() -> i32 {
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513ExternConflictingMangleDirectivesSource() {
    return @'
extern "C" {
    [[mangle(none), mangle("conflicting_extern_symbol")]]
    procedure conflicting_extern_mangle() -> i32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513MangleNoneExportWarningSource() {
    return @'
[[mangle(none), export("C")]]
public procedure redundant_mangle_warning() -> i32 {
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513UnwindAbortWarningSource() {
    return @'
[[export("C"), unwind("abort")]]
public procedure redundant_unwind_warning() -> i32 {
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513LibraryUnknownKindSource() {
    return @'
[[library(name: "ssl", kind: "mystery")]]
extern "C" {
    procedure unknown_library_kind() -> i32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513LibraryKnownKindsAcceptedSource() {
    return @'
[[library(name: "ssl")]]
extern "C" {
    procedure import_default_dylib() -> i32
}

[[library(name: "crypto", kind: "static")]]
extern "C" {
    procedure import_static() -> i32
}

[[library(name: "kernel32", kind: "raw-dylib")]]
extern "C" {
    procedure import_raw_dylib() -> i32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513LibraryUnsupportedFrameworkSource() {
    return @'
[[library(name: "CoreFoundation", kind: "framework")]]
extern "C" {
    procedure import_framework() -> i32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513LibraryMissingNameSource() {
    return @'
[[library(kind: "dylib")]]
extern "C" {
    procedure missing_library_name() -> i32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513UnwindNonFfiProcedureSource() {
    return @'
[[unwind("catch")]]
procedure internal_unwind_attr() -> i32 {
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return internal_unwind_attr()
}
'@
}

function New-Issue513UnwindInvalidStringModeSource() {
    return @'
[[export("C-unwind"), unwind("kaboom")]]
public procedure invalid_unwind_mode() -> i32 {
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513UnwindCatchWrongAbiRejectedSource() {
    return @'
[[export("C"), unwind("catch")]]
public procedure catch_wrong_abi() -> i32 {
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513FfiPassByValueMalformedSource() {
    return @'
[[ffi_pass_by_value("bad")]]
record BadByValueAttr {
    value: i32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513FfiPassByValueWrongTargetSource() {
    return @'
[[ffi_pass_by_value]]
procedure wrong_target_attr() -> i32 {
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return wrong_target_attr()
}
'@
}

function New-Issue513FfiPassByValueEnumAcceptedSource() {
    return @'
[[ffi_pass_by_value]]
enum ByValueEnum {
    Value
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let observed = ByValueEnum::Value
    let _ = observed
    return 0
}
'@
}

function New-Issue513ExportByValueDropTypeRejectedSource() {
    return @'
[[layout(C)]]
record ExportByValueDropType {
    value: i32

    procedure drop(~!) -> () {
        return ()
    }
}

[[export("C")]]
public procedure export_drop_by_value(value: ExportByValueDropType) -> i32 {
    let _ = value
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513ExportByValueDropTypeAcceptedSource() {
    return @'
[[layout(C), ffi_pass_by_value]]
record ExportByValueAllowed {
    value: unique i32

    procedure drop(~!) -> () {
        return ()
    }
}

[[export("C")]]
public procedure export_drop_by_value_allowed(value: ExportByValueAllowed) -> i32 {
    let _ = value
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513ExportGenericFfiSafeUnusedParamAcceptedSource() {
    return @'
[[layout(C)]]
record GoodWrapper<T; U>
|: FfiSafe(T)
{
    value: T
}

[[export("C")]]
public procedure export_good(value: GoodWrapper<i32, string@Managed>) -> i32 {
    let _ = value
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513ExportGenericFfiSafeMissingPredicateRejectedSource() {
    return @'
[[layout(C)]]
record BadWrapper<T>
|: Clone(T)
{
    value: T
}

[[export("C")]]
public procedure export_bad(value: BadWrapper<i32>) -> i32 {
    let _ = value
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513ExternNestedContextFieldRejectedSource() {
    return @'
[[layout(C)]]
record ContextWrapper {
    ctx: Context
}

extern "C" {
    procedure bad_extern(value: ContextWrapper) -> i32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue553ExternRegionLocalRawPtrArgRejectedSource() {
    return @'
extern "C" {
    procedure foreign_consume(ptr: *imm i32) -> i32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    region as r {
        let region_value: i32 = r ^ 7
        let region_ptr: Ptr<i32>@Valid = &region_value
        let region_raw: *imm i32 =
            unsafe { transmute<Ptr<i32>@Valid, *imm i32>(region_ptr) }
        let _ = unsafe { foreign_consume(region_raw) }
    }
    return 0
}
'@
}

function New-Issue553ExternStackRawPtrArgAllowedSource() {
    return @'
extern "C" {
    procedure foreign_consume(ptr: *imm i32) -> i32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let local_value: i32 = 7
    let local_ptr: Ptr<i32>@Valid = &local_value
    let local_raw: *imm i32 =
        unsafe { transmute<Ptr<i32>@Valid, *imm i32>(local_ptr) }
    let _ = unsafe { foreign_consume(local_raw) }
    return 0
}
'@
}

function New-Issue553ExportRegionLocalRawPtrReturnRejectedSource() {
    return @'
[[export("C")]]
public procedure export_region_ptr() -> *imm i32 {
    var escaped: *imm i32 = null
    region as r {
        let region_value: i32 = r ^ 7
        let region_ptr: Ptr<i32>@Valid = &region_value
        let region_raw: *imm i32 =
            unsafe { transmute<Ptr<i32>@Valid, *imm i32>(region_ptr) }
        escaped = region_raw
    }
    return escaped
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue553ExportStackRawPtrReturnAllowedSource() {
    return @'
[[export("C")]]
public procedure export_stack_ptr() -> *imm i32 {
    let local_value: i32 = 7
    let local_ptr: Ptr<i32>@Valid = &local_value
    let local_raw: *imm i32 =
        unsafe { transmute<Ptr<i32>@Valid, *imm i32>(local_ptr) }
    return local_raw
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue553ExportRegionLocalRawPtrReturnStillRejectedWhenReturningNullSource() {
    return @'
[[export("C")]]
public procedure export_region_ptr() -> *imm i32 {
    var escaped: *imm i32 = null
    region as r {
        let region_value: i32 = r ^ 7
        let region_ptr: Ptr<i32>@Valid = &region_value
        let region_raw: *imm i32 =
            unsafe { transmute<Ptr<i32>@Valid, *imm i32>(region_ptr) }
        escaped = region_raw
    }
    return null
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue553HostExportStackRawPtrReturnAllowedSource() {
    return @'
record HostedProbeContext {
    fs: $FileSystem
}

[[host_export("C")]]
public procedure export_stack_ptr(ctx: HostedProbeContext) -> *imm i32 {
    let _ = ctx
    let local_value: i32 = 7
    let local_ptr: Ptr<i32>@Valid = &local_value
    let local_raw: *imm i32 =
        unsafe { transmute<Ptr<i32>@Valid, *imm i32>(local_ptr) }
    return local_raw
}
'@
}

function New-Issue553HostExportRegionLocalRawPtrReturnStillRejectedWhenReturningStackSource() {
    return @'
record HostedProbeContext {
    fs: $FileSystem
}

[[host_export("C")]]
public procedure export_region_ptr(ctx: HostedProbeContext) -> *imm i32 {
    let _ = ctx
    var escaped: *imm i32 = null
    region as r {
        let region_value: i32 = r ^ 7
        let region_ptr: Ptr<i32>@Valid = &region_value
        let region_raw: *imm i32 =
            unsafe { transmute<Ptr<i32>@Valid, *imm i32>(region_ptr) }
        escaped = region_raw
    }
    let local_value: i32 = 7
    let local_ptr: Ptr<i32>@Valid = &local_value
    let local_raw: *imm i32 =
        unsafe { transmute<Ptr<i32>@Valid, *imm i32>(local_ptr) }
    return local_raw
}
'@
}

function New-Issue513KeyBlockMemoryOrderSpeculativeSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    var gate: shared i32 = 0
    [[relaxed]]
    # gate speculative write {
        gate = gate + 1
    }
    return 0
}
'@
}

function New-Issue513MalformedAttrSyntaxSource() {
    return @'
[[export(]]
public procedure malformed_export_attr() -> i32 {
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513DeriveMalformedArgsSource() {
    return @'
[[derive("Display")]]
public record DerivedProbe {
    value: i32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513VendorScopedUnknownSource() {
    return @'
[[com::vendor::probe]]
public procedure vendor_probe() -> i32 {
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513VendorShortScopedUnknownSource() {
    return @'
[[vendor::probe]]
public procedure vendor_short_probe() -> i32 {
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513VendorDottedUnknownSource() {
    return @'
[[com.vendor.probe]]
public procedure vendor_dotted_probe() -> i32 {
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513VendorMixedScopedDottedMalformedSource() {
    return @'
[[vendor::probe.extra]]
public procedure vendor_mixed_probe() -> i32 {
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513ReservedCursiveAttrSource() {
    return @'
[[cursive::probe]]
public procedure reserved_probe() -> i32 {
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513StaticOnNonForeignProcedureSource() {
    return @'
[[static]]
public procedure local_static_mode() -> i32 {
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return local_static_mode()
}
'@
}

function New-Issue513UnknownProcedureVerificationAttrSource() {
    return @'
[[trust]]
public procedure local_unknown_verification_mode() -> i32 {
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return local_unknown_verification_mode()
}
'@
}

function New-Issue513LibraryAttrOnExpressionSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let value: i32 = [[library("ffi")]] 1
    return value
}
'@
}

function New-Issue513ComptimeProcedureAttrTargetSource() {
    return @'
[[layout(C)]]
comptime procedure generated_helper() -> () {
    return
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513ComptimeExprMalformedAttrSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let value: i32 = [[files("unexpected")]] comptime { 1 }
    return value
}
'@
}

function New-Issue513ExprMemoryOrderDefaultAndOverrideSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    var gate: shared [i32; 4] = [0, 0, 0, 0]
    [[release]]
    # gate write {
        gate[0] = 1
        let observed: i32 = [[acquire]] gate[0]
        let _ = observed
    }
    return 0
}
'@
}

function New-Issue513ExprMemoryOrderSubtreeSharedAccessSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    var gate: shared [i32; 4] = [0, 0, 0, 0]
    # gate read {
        let observed: i32 = [[acquire]] (gate[0] + 1)
        let _ = observed
    }
    return 0
}
'@
}

function New-Issue513ExprMemoryOrderDuplicateRejectedSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    var gate: shared [i32; 4] = [0, 0, 0, 0]
    # gate read {
        let observed: i32 = [[acquire, release]] gate[0]
        let _ = observed
    }
    return 0
}
'@
}

function New-Issue513KeyBlockMemoryOrderDuplicateRejectedSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    var gate: shared [i32; 4] = [0, 0, 0, 0]
    [[acquire, release]]
    # gate read {
        let observed: i32 = gate[0]
        let _ = observed
    }
    return 0
}
'@
}

function New-Issue513ExprMemoryOrderInvalidPlacementSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let value: i32 = [[acquire]] (1 + 2)
    return value
}
'@
}

function New-Issue513ExprMemoryOrderSpeculativeRejectedSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    var gate: shared [i32; 4] = [0, 0, 0, 0]
    # gate speculative write {
        let observed: i32 = [[acquire]] gate[0]
        let _ = observed
    }
    return 0
}
'@
}

function New-Issue513InlineHintRejectedSource() {
    return @'
[[inline(hint)]]
public procedure inline_hint_probe() -> i32 {
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513ColdHintRejectedSource() {
    return @'
[[cold(unlikely)]]
public procedure cold_hint_probe() -> i32 {
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513UnwindIdentifierModeRejectedSource() {
    return @'
[[export("C-unwind"), unwind(catch)]]
public procedure unwind_identifier_mode() -> i32 {
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513ExternUnwindIdentifierModeRejectedSource() {
    return @'
extern "C-unwind" {
    [[unwind(catch)]]
    procedure bad_unwind_mode() -> i32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513ClassDynamicInheritedMethodSource() {
    return @'
public class DynamicClassScope {
    [[dynamic]]
    procedure probe(~, idx: usize) -> i32 {
        var gate: shared [i32; 4] = [0, 0, 0, 0]
        # gate write {
            gate[idx] = 1
        }
        return gate[idx]
    }
}

public record DynamicClassImpl <: DynamicClassScope {
    marker: i32
    override procedure probe(~, idx: usize) -> i32 {
        var gate: shared [i32; 4] = [0, 0, 0, 0]
        # gate write {
            gate[idx] = 1
        }
        return gate[idx]
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513ClassDynamicStaticOverrideSource() {
    return @'
public class DynamicClassScope {
    [[dynamic]]
    procedure probe(~, idx: usize) -> i32 {
        var gate: shared [i32; 4] = [0, 0, 0, 0]
        # gate write {
            gate[idx] = 1
        }
        return gate[idx]
    }
}

public record DynamicClassImpl <: DynamicClassScope {
    marker: i32
    [[static]]
    override procedure probe(~, idx: usize) -> i32 {
        var gate: shared [i32; 4] = [0, 0, 0, 0]
        # gate write {
            gate[idx] = 1
        }
        return gate[idx]
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513LogExpectedMovedBindingSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    var value: unique i32 = 7
    let observed: i32 = move value
    let checked: i32 = [[log(label: "issue513-log-moved", expected: value)]] observed
    return checked
}
'@
}

function New-Issue513LogStandaloneStatementSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    [[log(label: "issue513-standalone-stmt", expected: 0)]]
    let value: i32 = 4
    return value
}
'@
}

function New-Issue513LogBindingExpectedMovedSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    var value: unique i32 = 7
    let observed: i32 = move value
    [[log(label: "issue513-binding-log-moved", expected: value)]] let checked: i32 = observed
    return checked
}
'@
}

function New-Issue513BindingDeprecatedWarningSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    [[deprecated("prefer replacement binding")]]
    let old_value: i32 = 7
    let copied: i32 = old_value
    return copied
}
'@
}

function New-Issue513MethodDeprecatedWarningSource() {
    return @'
record Issue513DeprecatedMethodProbe {
    [[deprecated("use ping_v2")]]
    procedure ping(~) -> i32 {
        return 7
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let probe: Issue513DeprecatedMethodProbe = Issue513DeprecatedMethodProbe{}
    return probe~>ping()
}
'@
}

function New-Issue513FieldDeprecatedWarningSource() {
    return @'
record Issue513DeprecatedFieldProbe {
    [[deprecated("use next_value")]]
    value: i32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let probe: Issue513DeprecatedFieldProbe = Issue513DeprecatedFieldProbe{ value: 5 }
    return probe.value
}
'@
}

function New-Issue513RecordDeprecatedWarningSource() {
    return @'
[[deprecated("use ReplacementRecord")]]
record Issue513DeprecatedRecordProbe {
    value: i32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let probe: Issue513DeprecatedRecordProbe = Issue513DeprecatedRecordProbe{ value: 5 }
    return probe.value
}
'@
}

function New-Issue513EnumDeprecatedWarningSource() {
    return @'
[[deprecated("use ReplacementEnum")]]
enum Issue513DeprecatedEnumProbe {
    Value
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let probe = Issue513DeprecatedEnumProbe::Value
    return if probe is {
        Issue513DeprecatedEnumProbe::Value { 1 }
    }
}
'@
}

function New-Issue513InlineAlwaysAddressTakenWarningSource() {
    return @'
[[inline(always)]]
procedure issue513_inline_target() -> i32 {
    return 1
}

procedure issue513_inline_ref() -> i32 {
    let callee: () -> i32 = issue513_inline_target
    return callee()
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return issue513_inline_ref()
}
'@
}

function New-Issue513DynamicNoRuntimeWarningSource() {
    return @'
[[dynamic]]
public procedure DynamicNoOp(value: i32) -> i32
    |: (value == value)
{
    return value
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return DynamicNoOp(1)
}
'@
}

function New-Issue513LogMalformedArgsSource() {
    return @'
[[log(bad: 1)]]
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513LogPositionalArgSource() {
    return @'
[[log("bad")]]
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513LogMultipleArgsSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let expected_value: i32 = 7
    let observed: i32 = [[log(label: "multi-arg", expected: expected_value)]] 7
    if (observed != expected_value) {
        return 1
    }
    return 0
}
'@
}

function New-Issue513LogNeverReturnSource() {
    return @'
[[log(expected: 0)]]
procedure NeverProc() -> ! {
    loop {
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513DynamicClauseDirectSource() {
    return @'
public procedure DynClauseDirect(x: i32) -> i32
    |: [[dynamic]] (x == x)
{
    return x
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return DynClauseDirect(1)
}
'@
}

function New-Issue513DynamicClauseRepeatedAttrListsSource() {
    return @'
public procedure DynClauseRepeatedAttrLists(x: i32) -> i32
    |: [[dynamic]][[dynamic]] (x == x)
{
    return x
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return DynClauseRepeatedAttrLists(1)
}
'@
}

function New-Issue513DynamicTypeAliasSource() {
    return @'
[[dynamic]]
type Alias = i32

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let value: Alias = 1
    return value
}
'@
}

function New-Issue513DynamicFieldTargetSource() {
    return @'
public record DynamicFieldTarget {
    [[dynamic]]
    value: i32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = DynamicFieldTarget { value: 1 }
    return 0
}
'@
}

function New-Issue513ExprLogNewlineContinuationSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let x: i32 = [[log(label: "issue513-expr-newline", expected: 1)]]
        1
    return x
}
'@
}

function New-Issue513StaleWarningYieldReleaseSource() {
    return @'
public procedure stale_probe(move ctx: Context) -> Async<i32, i32, i32, i32> {
    let _ = ctx
    var gate: shared [i32; 1] = [0]
    # gate write {
        let incoming: i32 = yield release 1
        gate[0] = incoming
    }
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue513AbiInlineColdLlSource() {
    return @'
[[mangle("vectorcall_export"), export("vectorcall")]]
public procedure vectorcall_export(value: i32) -> i32 {
    return value
}

[[inline(always), cold]]
procedure helper_always_cold() -> i32 {
    return 1
}

[[inline(never)]]
procedure helper_never() -> i32 {
    return 2
}

public procedure call_vector() -> i32 {
    return vectorcall_export(7) + helper_always_cold() + helper_never()
}
'@
}

function New-Issue513LayoutAlignWarningSource() {
    return @'
[[layout(align(1))]]
record AlignWarnRecord {
    value: i32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let instance = AlignWarnRecord{ value: 7 }
    return instance.value
}
'@
}

function New-Issue513LayoutLlSource() {
    return @'
[[layout(packed)]]
record PackedPair {
    tag: u8
    value: i32
}

[[layout(align(16))]]
record Align16Record {
    value: u8
}

[[layout(u16)]]
enum DiscU16Enum {
    None
    Some { value: i32 }
}

public procedure layout_probe() -> i32 {
    let packed = PackedPair{ tag: 1u8, value: 9 }
    let aligned = Align16Record{ value: packed.tag }
    let _ = aligned
    let tagged: DiscU16Enum = DiscU16Enum::Some { value: packed.value }
    return if tagged is {
        DiscU16Enum::None { 0 }
        DiscU16Enum::Some { value: v } { v }
    }
}
'@
}

function New-Issue5131AttrTestUnknownSource() {
    return @'
[[test]]
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue5131AttrBenchUnknownSource() {
    return @'
[[bench]]
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue5131AttrDebugContractUnknownSource() {
    return @'
[[debug_contract]]
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue5131AttrTrailingCommaSingleLineSource() {
    return @'
[[export("C",)]]
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue5131AttrSpecTrailingCommaSingleLineSource() {
    return @'
[[cold,]]
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue5131AttrSpecTrailingCommaMultilineSource() {
    return @'
[[
    cold,
]]
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue5131VendorDotColonPolicySource() {
    return @'
[[com.vendor::probe]]
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue5131MethodInlineAllowedSource() {
    return @'
record Issue5131MethodInlineProbe {
    [[inline(always)]]
    procedure ping(~) -> i32 {
        return 7
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let probe: Issue5131MethodInlineProbe = Issue5131MethodInlineProbe{}
    return probe~>ping()
}
'@
}

function New-Issue5131MethodDeprecatedAllowedSource() {
    return @'
record Issue5131MethodDeprecatedProbe {
    [[deprecated("use replacement")]]
    procedure ping(~) -> i32 {
        return 7
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let probe: Issue5131MethodDeprecatedProbe = Issue5131MethodDeprecatedProbe{}
    return probe~>ping()
}
'@
}

function New-Issue5131MethodMangleRejectedSource() {
    return @'
record Issue5131MethodMangleProbe {
    [[mangle("method_symbol")]]
    procedure ping(~) -> i32 {
        return 7
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let probe: Issue5131MethodMangleProbe = Issue5131MethodMangleProbe{}
    return probe~>ping()
}
'@
}

function New-Issue5131MultiAttrListsItemConcatAllowedSource() {
    return @'
[[inline(always)]]
[[cold]]
procedure issue5131_concat_probe() -> i32 {
    return 1
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return issue5131_concat_probe()
}
'@
}

function New-Issue5131MethodColdAllowedSource() {
    return @'
record Issue5131MethodColdProbe {
    [[cold]]
    procedure ping(~) -> i32 {
        return 7
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let probe: Issue5131MethodColdProbe = Issue5131MethodColdProbe{}
    return probe~>ping()
}
'@
}

function New-Issue5131MethodDynamicAllowedSource() {
    return @'
record Issue5131MethodDynamicProbe {
    [[dynamic]]
    procedure ping(~) -> i32 {
        return 7
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let probe: Issue5131MethodDynamicProbe = Issue5131MethodDynamicProbe{}
    return probe~>ping()
}
'@
}

function New-Issue5131MethodLogAllowedSource() {
    return @'
record Issue5131MethodLogProbe {
    [[log(label: "ping", expected: 7)]]
    procedure ping(~) -> i32 {
        return 7
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let probe: Issue5131MethodLogProbe = Issue5131MethodLogProbe{}
    return probe~>ping()
}
'@
}

function New-Issue5131MethodStaticRejectedSource() {
    return @'
record Issue5131MethodStaticProbe {
    [[static]]
    procedure ping(~) -> i32 {
        return 7
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let probe: Issue5131MethodStaticProbe = Issue5131MethodStaticProbe{}
    return probe~>ping()
}
'@
}

function New-Issue27ImportUnwindCodegenSource() {
    return @"
extern "C-unwind" {
    [[unwind("abort")]]
    procedure foreign_abort() -> i32

    [[unwind("catch")]]
    procedure foreign_catch() -> i32
}

public procedure probe_import_unwind() -> i32 {
    unsafe {
        let a: i32 = foreign_abort()
        let b: i32 = foreign_catch()
        let _sum: i32 = a + b
    }
    return 0
}
"@
}

function New-Issue32UnsuffixedFloatDefaultF32Source() {
    return @"
procedure takes_f32(value: f32) -> f32 {
    return value
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let inferred = 1.25
    let checked: f32 = takes_f32(inferred)
    let _ = checked
    return 0
}
"@
}

function New-Issue32DeclaredFloatTypePrecedenceSource() {
    return @"
procedure takes_f64(value: f64) -> f64 {
    return value
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let declared: f64 = 1.25
    let checked: f64 = takes_f64(declared)
    let _ = checked
    return 0
}
"@
}

function New-Issue32UntypedIntLiteralNotFloatSource() {
    return @"
procedure takes_i32(value: i32) -> i32 {
    return value
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let inferred = 7
    let checked: i32 = takes_i32(inferred)
    return checked
}
"@
}

function New-Issue32DecimalToIntRejectedSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let bad: i32 = 1.25
    return bad
}
"@
}

function New-Issue32ExplicitSuffixMismatchSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let bad: f32 = 1.25f64
    let _ = bad
    return 0
}
"@
}

function New-Issue32TupleAccessDotDisambiguationSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let pair: (i32, (i32, i32)) = (1, (2, 3))
    let left: i32 = pair.1.0
    let right: i32 = pair.1.1
    return left + right
}
"@
}

function New-Issue547TupleLoweringTraceSource() {
    return @"
public procedure tuple_projection_metric() -> i32 {
    let pair: (i32, i32) = (1, 2)
    let nested: ((i32, i32), (i32;)) = ((3, 4), (5;))
    let direct_metric: i32 = ((6, 7), (8;)).0.1 + ((6, 7), (8;)).1.0
    return pair.0 + pair.1 + nested.0.0 + nested.1.0 + direct_metric
}
"@
}

function New-Issue548TupleAccessEvalSigmaSource() {
    return @"
procedure Advance(counter: *mut i32, move digit: i32) -> i32 {
    var counter_mut: *mut i32 = counter
    let next: i32 = unsafe {
        let prior: i32 = *counter_mut
        let advanced: i32 = (prior * 10) + digit
        *counter_mut = advanced
        advanced
    }
    return next
}

procedure BuildTuple(counter: *mut i32) -> (i32, i32) {
    var counter_mut: *mut i32 = counter
    return (
        Advance(counter_mut, 7),
        Advance(counter_mut, 8)
    )
}

public procedure TupleAccessEvalMetric(counter: *mut i32) -> i32 {
    return BuildTuple(counter).1
}

public procedure TupleAccessCtrlMetric(counter: *mut i32) -> i32 {
    let projected: i32 = {
        if (true) {
            let _ = Advance(counter, 4)
            return 17
        }
        (
            Advance(counter, 5),
            Advance(counter, 6)
        )
    }.0
    return projected
}
"@
}

function New-Issue549ArrayIndexNonConstSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let values: [i32; 3] = [1, 2, 3]
    let idx: usize = 1usize
    let observed: i32 = values[idx]
    return observed
}
"@
}

function New-Issue549ArrayIndexOobSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let values: [i32; 3] = [1, 2, 3]
    let observed: i32 = values[3usize]
    return observed
}
"@
}

function New-Issue549ArrayIndexDynamicSource() {
    return @"
[[dynamic]]
public procedure main(move ctx: Context) -> i32
    |: true
{
    let _ = ctx
    var values: [i32; 3] = [1, 2, 3]
    let idx: usize = 1usize
    values[idx] = values[idx] + 10
    return values[idx]
}
"@
}

function New-Issue549ArrayIndexDynamicExprSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let values: [i32; 4] = [10, 20, 30, 40]
    let idx: usize = 2usize
    let observed: i32 = [[dynamic]] values[idx]
    return observed
}
"@
}

function New-Issue549ArrayIndexDynamicStmtSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    var values: [i32; 4] = [10, 20, 30, 40]
    let idx: usize = 2usize
    [[dynamic]]
    values[idx] = values[idx] + 7
    return [[dynamic]] values[idx]
}
"@
}

function New-Issue560IfStmtNonUnitBranchSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    if true {
        1
    }
    return 0
}
"@
}

function New-Issue560LoopIfStmtNonUnitBranchSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    var idx: i32 = 0
    loop idx < 2 {
        if true {
            1
        }
        idx = idx + 1
    }
    return 0
}
"@
}

function New-Issue554CallTempNoProvenanceSource() {
    return @"
procedure borrow(value: i32) -> i32 {
    return value
}

procedure consume(move value: i32) -> i32 {
    return value
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let borrowed: i32 = borrow(1 + 2)
    let consumed: i32 = consume(3 + 4)
    return borrowed + consumed
}
"@
}

function New-Issue550ArrayEvalSigmaSource() {
    return @"
procedure Advance(counter: *mut i32, move digit: i32) -> i32 {
    var counter_mut: *mut i32 = counter
    let next: i32 = unsafe {
        let prior: i32 = *counter_mut
        let advanced: i32 = (prior * 10) + digit
        *counter_mut = advanced
        advanced
    }
    return next
}

procedure BuildArray(counter: *mut i32) -> [i32; 2] {
    var counter_mut: *mut i32 = counter
    return [
        Advance(counter_mut, 7),
        Advance(counter_mut, 8)
    ]
}

public procedure ArrayEvalMetric(counter: *mut i32) -> i32 {
    let values: [i32; 2] = BuildArray(counter)
    return values[0] + values[1]
}

public procedure ArrayCtrlMetric(counter: *mut i32) -> i32 {
    let _probe: [i32; 3] = [
        Advance(counter, 4),
        if (true) { return 17 } else { Advance(counter, 5) },
        Advance(counter, 6)
    ]
    return 0
}
"@
}

function New-Issue551IndexEvalSigmaSource() {
    return @"
procedure Advance(counter: *mut i32, move digit: i32) -> i32 {
    var counter_mut: *mut i32 = counter
    let next: i32 = unsafe {
        let prior: i32 = *counter_mut
        let advanced: i32 = (prior * 10) + digit
        *counter_mut = advanced
        digit
    }
    return next
}

procedure BuildValues(counter: *mut i32) -> [i32; 2] {
    return [
        Advance(counter, 7),
        Advance(counter, 8)
    ]
}

procedure BuildIndex(counter: *mut i32) -> usize {
    let _ = Advance(counter, 1)
    return 1usize
}

[[dynamic]]
public procedure IndexEvalMetric(counter: *mut i32) -> i32
    |: true
{
    let values: [i32; 2] = BuildValues(counter)
    let idx: usize = BuildIndex(counter)
    return values[idx]
}

[[dynamic]]
public procedure IndexCtrlMetric(counter: *mut i32) -> i32
    |: true
{
    let values: [i32; 2] = BuildValues(counter)
    let idx: usize = if (true) { return 17 } else { BuildIndex(counter) }
    return values[idx]
}

[[dynamic]]
public procedure IndexOobMetric(index: usize) -> i32
    |: true
{
    let values: [i32; 2] = [4, 7]
    return values[index]
}
"@
}

function New-Issue554IfCaseTempOwnershipSource() {
    return @"
[[dynamic]]
public procedure IfCaseOwnedMetric(ctx: Context) -> i32
    |: true
{
    return if string::to_managed("owned", ctx.heap) is {
        text: string@Managed {
            let view: string@View = string::as_view(text)
            string::length(view) as i32
        }
        _: AllocationError { 0 }
    }
}
"@
}

function New-Issue555SliceIndexEvalSigmaSource() {
    return @"
procedure Advance(counter: *mut i32, move digit: i32) -> i32 {
    var counter_mut: *mut i32 = counter
    let next: i32 = unsafe {
        let prior: i32 = *counter_mut
        let advanced: i32 = (prior * 10) + digit
        *counter_mut = advanced
        digit
    }
    return next
}

procedure BuildSlice(counter: *mut i32) -> [u8] {
    let _ = Advance(counter, 7)
    let view: bytes@View = bytes::view_string("abc")
    return bytes::as_slice(view)
}

procedure BuildSliceIndex(counter: *mut i32) -> usize {
    let _ = Advance(counter, 1)
    return 1usize
}

[[dynamic]]
public procedure SliceIndexEvalMetric(counter: *mut i32) -> i32
    |: true
{
    let slice: [u8] = BuildSlice(counter)
    let idx: usize = BuildSliceIndex(counter)
    return slice[idx] as i32
}

[[dynamic]]
public procedure SliceIndexOobMetric(index: usize) -> i32
    |: true
{
    let view: bytes@View = bytes::view_string("abc")
    let slice: [u8] = bytes::as_slice(view)
    return slice[index] as i32
}
"@
}

function New-Issue556RangeIndexEvalSigmaSource() {
    return @"
procedure Advance(counter: *mut i32, move digit: i32) -> i32 {
    var counter_mut: *mut i32 = counter
    let next: i32 = unsafe {
        let prior: i32 = *counter_mut
        let advanced: i32 = (prior * 10) + digit
        *counter_mut = advanced
        digit
    }
    return next
}

procedure BuildArray(counter: *mut i32) -> [i32; 4] {
    return [
        Advance(counter, 7),
        Advance(counter, 8),
        Advance(counter, 9),
        Advance(counter, 0)
    ]
}

procedure BuildSlice(counter: *mut i32) -> [u8] {
    let _ = Advance(counter, 4)
    let view: bytes@View = bytes::view_string("abcd")
    return bytes::as_slice(view)
}

procedure BuildRangeStart(counter: *mut i32) -> usize {
    let _ = Advance(counter, 1)
    return 1usize
}

procedure BuildRangeEnd(counter: *mut i32) -> usize {
    let _ = Advance(counter, 2)
    return 3usize
}

public procedure RangeIndexArrayMetric(counter: *mut i32) -> i32 {
    let values: [i32; 4] = BuildArray(counter)
    let window: [i32] = values[BuildRangeStart(counter)..BuildRangeEnd(counter)]
    return window[0usize] + window[1usize]
}

public procedure RangeIndexSliceMetric(counter: *mut i32) -> i32 {
    let values: [u8] = BuildSlice(counter)
    let window: [u8] = values[BuildRangeStart(counter)..BuildRangeEnd(counter)]
    return (window[0usize] as i32) + (window[1usize] as i32)
}

public procedure RangeIndexOobMetric(end: usize) -> i32 {
    let values: [i32; 4] = [4, 7, 9, 11]
    let window: [i32] = values[1usize..end]
    return window[0usize]
}
"@
}

function New-Issue557RangeEvalSigmaSource() {
    return @"
procedure Advance(counter: *mut i32, move digit: i32) -> i32 {
    var counter_mut: *mut i32 = counter
    let next: i32 = unsafe {
        let prior: i32 = *counter_mut
        let advanced: i32 = (prior * 10) + digit
        *counter_mut = advanced
        advanced
    }
    return next
}

procedure BuildRangeStart(counter: *mut i32) -> usize {
    let _ = Advance(counter, 1)
    return 1usize
}

procedure BuildRangeEnd(counter: *mut i32) -> usize {
    let _ = Advance(counter, 2)
    return 4usize
}

procedure BuildRangeValue(counter: *mut i32) -> Range<usize> {
    return BuildRangeStart(counter)..BuildRangeEnd(counter)
}

public procedure RangeValueMetric(counter: *mut i32) -> i32 {
    let values: [i32; 5] = [3, 5, 7, 11, 13]
    let range_value: Range<usize> = BuildRangeValue(counter)
    let window: [i32] = values[range_value]
    return window[0usize] + window[2usize]
}
"@
}

function New-Issue558RecordEvalSigmaSource() {
    return @"
procedure Advance(counter: *mut i32, move digit: i32) -> i32 {
    var counter_mut: *mut i32 = counter
    let next: i32 = unsafe {
        let prior: i32 = *counter_mut
        let advanced: i32 = (prior * 10) + digit
        *counter_mut = advanced
        advanced
    }
    return next
}

record EvalRecordProbe {
    first: i32
    second: i32
    third: i32
}

public procedure RecordEvalMetric(counter: *mut i32) -> i32 {
    let value: EvalRecordProbe = EvalRecordProbe {
        first: Advance(counter, 1),
        second: Advance(counter, 2),
        third: Advance(counter, 3)
    }
    return value.first + value.second + value.third
}

public procedure RecordCtrlMetric(counter: *mut i32) -> i32 {
    let _probe: EvalRecordProbe = EvalRecordProbe {
        first: Advance(counter, 4),
        second: if (true) { return 17 } else { Advance(counter, 5) },
        third: Advance(counter, 6)
    }
    return 0
}

procedure DefaultFieldOne() -> i32 {
    return 5
}

procedure DefaultFieldTwo() -> i32 {
    return 7
}

record DefaultRecordProbe {
    first: i32 = DefaultFieldOne()
    second: i32 = DefaultFieldTwo()
}

public procedure RecordCtorMetric() -> i32 {
    let value: DefaultRecordProbe = DefaultRecordProbe()
    return value.first + value.second
}
"@
}

function New-Issue32ImportStdSource() {
    return @"
import std

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue32UsingStdSource() {
    return @"
using std::thing

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue51UsingListDuplicateNameSource() {
    return @"
record Alpha {
    value: i32
}

using probe::{Alpha, Alpha}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue51UsingListDuplicateAliasSource() {
    return @"
record Left {
    value: i32
}

record Right {
    value: i32
}

using probe::{Left as Same, Right as Same}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue51UsingListSelfAliasCollisionSource() {
    return @"
record Item {
    value: i32
}

using probe::{Item as Same, Item as Same}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue51UsingBarePathRejectedSource() {
    return @"
using probe

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue51UsingBareAliasRejectedSource() {
    return @"
using probe as Alias

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue51UsingSelfOrdinaryNameSource() {
    return @"
record Item {
    value: i32
}

using probe::{self, Item as probe}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue51PublicExternBlockVisibilitySource() {
    return @"
public extern "C" {
    public procedure puts(s: *imm i8) -> i32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue51ExternProcTerminatorRequiredSource() {
    return @"
extern "C" {
    procedure foo() -> i32 procedure bar() -> i32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function Invoke-Issue51FfiAbiProfileConformanceCase {
    $externStdcallSysv = Invoke-CheckWithConformance `
        -CaseId "issue51_extern_stdcall_sysv_rejected" `
        -Source @'
extern "stdcall" {
    procedure foreign_value() -> i32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@ `
        -ExtraArgs @("--target-profile", "x86_64-sysv") `
        -AllowMissingConformance `
        -ConformanceFileName "issue51_extern_stdcall_sysv_rejected.log"

    if ($externStdcallSysv.ExitCode -ne 1) {
        throw "Case 'issue51_extern_stdcall_sysv_rejected' expected exit 1 but got $($externStdcallSysv.ExitCode)."
    }
    $externStdcallSysvDiag = @($externStdcallSysv.DiagJson.diagnostics | Where-Object {
        $_.code -eq "E-SYS-3352"
    }).Count
    if ($externStdcallSysvDiag -lt 1) {
        throw "Case 'issue51_extern_stdcall_sysv_rejected' expected E-SYS-3352."
    }

    $vectorcallAapcs = Invoke-CheckWithConformance `
        -CaseId "issue51_export_vectorcall_aapcs_rejected" `
        -Source @'
[[export("vectorcall")]]
public procedure foreign_export(value: i32) -> i32 {
    return value
}
'@ `
        -ExtraArgs @("--target-profile", "aarch64-aapcs64") `
        -AllowMissingConformance `
        -ConformanceFileName "issue51_export_vectorcall_aapcs_rejected.log"

    if ($vectorcallAapcs.ExitCode -ne 1) {
        throw "Case 'issue51_export_vectorcall_aapcs_rejected' expected exit 1 but got $($vectorcallAapcs.ExitCode)."
    }
    $vectorcallAapcsDiag = @($vectorcallAapcs.DiagJson.diagnostics | Where-Object {
        $_.code -eq "E-SYS-3352"
    }).Count
    if ($vectorcallAapcsDiag -lt 1) {
        throw "Case 'issue51_export_vectorcall_aapcs_rejected' expected E-SYS-3352."
    }

    $externStdcallWin64 = Invoke-CheckWithConformance `
        -CaseId "issue51_extern_stdcall_win64_allowed" `
        -Source @'
extern "stdcall" {
    procedure foreign_value() -> i32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@ `
        -ExtraArgs @("--target-profile", "x86_64-win64") `
        -AllowMissingConformance `
        -ConformanceFileName "issue51_extern_stdcall_win64_allowed.log"

    if ($externStdcallWin64.ExitCode -ne 0) {
        throw "Case 'issue51_extern_stdcall_win64_allowed' expected exit 0 but got $($externStdcallWin64.ExitCode)."
    }
    $externStdcallWin64Errors = @($externStdcallWin64.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($externStdcallWin64Errors -ne 0) {
        throw "Case 'issue51_extern_stdcall_win64_allowed' expected zero errors but observed $externStdcallWin64Errors."
    }

    Write-Host "[compiler-static] issue51_ffi_abi_profiles: sysv_rejected=$externStdcallSysvDiag aapcs_rejected=$vectorcallAapcsDiag win64_allowed=1"
}

function New-Issue51UsingItemParseTraceSource() {
    return @"
record Alpha {
    value: i32
}

using probe::Alpha as AliasAlpha

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let item: AliasAlpha = AliasAlpha { value: 7 }
    let _ = item
    return 0
}
"@
}

function New-Issue51UsingListParseTraceSource() {
    return @"
record Alpha {
    value: i32
}

record Beta {
    value: i32
}

using probe::{Alpha as AliasAlpha, Beta}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let item: AliasAlpha = AliasAlpha { value: 7 }
    let other: Beta = Beta { value: 5 }
    let _ = other
    return item.value
}
"@
}

function New-Issue51UsingWildcardParseTraceSource() {
    return @"
public record Alpha {
    value: i32
}

public enum Choice {
    Up
}

public using probe::*

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let item: Alpha = Alpha { value: 7 }
    let choice: Choice = Choice::Up
    let _ = choice
    return item.value
}
"@
}

function New-Issue51PublicUsingItemRejectMainSource() {
    return @"
import mod

public using mod::hidden_value

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue51PublicUsingItemAcceptMainSource() {
    return @"
import mod

public using mod::visible_value as exported_visible

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let observed: i32 = exported_visible()
    let _ = observed
    return 0
}
"@
}

function New-Issue51PublicUsingItemModuleSource() {
    return @"
procedure hidden_value() -> i32 {
    return 7
}

public procedure visible_value() -> i32 {
    return 9
}
"@
}

function New-Issue33ClassMemberTerminatorRequiredSource() {
    return @"
class MissingTerminatorClass { procedure f(~) -> i32}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue33NestedGenericCloseSource() {
    return @"
record Inner<T> {
    value: T
}

record Outer<T> {
    value: T
}

type Nested = Outer<Inner<i32>>

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue33UsingAttrTargetRejectedSource() {
    return @"
[[export("C")]]
using foo::bar

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue33ImportAttrTargetRejectedSource() {
    return @"
[[export("C")]]
import foo::bar

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue33ImportAttrMultipleBlocksRejectedSource() {
    return @"
[[export("C")]]
[[export("C")]]
import foo::bar

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue33StaticAttrTargetRejectedSource() {
    return @"
[[export("C")]]
public let bad_static: i32 = 1

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue625StaticDeclAttributeListMultipleBlocksSource() {
    return @"
[[export("C")]]
[[deprecated("still a static target error, not a parse error")]]
public let bad_static: i32 = 1

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue33ClassDuplicateParamNamesSource() {
    return @"
class BadClassDupParams {
    procedure f(~, x: i32, x: i32) -> i32;
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue33ClassSelfParamForbiddenSource() {
    return @"
class BadClassSelfParam {
    procedure f(~, self: i32) -> i32;
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue33ModalDuplicateParamNamesSource() {
    return @"
modal BadModalDup {
    @S {
        procedure f(~, x: i32, x: i32) -> i32 {
            return 0
        }
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue33ModalSelfParamForbiddenSource() {
    return @"
modal BadModalSelf {
    @S {
        procedure f(~, self: i32) -> i32 {
            return 0
        }
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue33EnumMissingClassMethodImplSource() {
    return @"
class RequiresMethod {
    procedure required(~) -> i32;
}

enum MissingImpl <: RequiresMethod {
    A
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue33EnumItemSeparatedSource() {
    return @"
public enum EnumItemSeparated {
    A
    B(i32, i32)
    C { value: i32 }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let value: EnumItemSeparated = EnumItemSeparated::C { value: 7 }
    return if value is {
        EnumItemSeparated::A { 0 }
        EnumItemSeparated::B(left, right) { left + right }
        EnumItemSeparated::C { value: observed } { observed }
    }
}
"@
}

function New-Issue33EnumSemicolonSeparatedSource() {
    return @"
public enum EnumSemicolonSeparated { A; B(i32, i32); C { value: i32 }; }

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let value: EnumSemicolonSeparated = EnumSemicolonSeparated::B(2, 5)
    return if value is {
        EnumSemicolonSeparated::A { 0 }
        EnumSemicolonSeparated::B(left, right) { left + right }
        EnumSemicolonSeparated::C { value: observed } { observed }
    }
}
"@
}

function New-Issue33EnumCommaSeparatedRejectedSource() {
    return @"
public enum EnumCommaSeparated {
    A,
    B
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue33EnumMissingTerminatorSource() {
    return @"
public enum EnumMissingTerminator { A B }

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue33RecordAssociatedTypeMemberSource() {
    return @"
class AssocCarrier {
    type Item;
}

record AssocImpl <: AssocCarrier {
    type Item = i32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue33CallTypeArgsExplicitSource() {
    return @"
procedure id<T>(value: T) -> T {
    return value
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let out: i32 = id<i32>(7)
    return out
}
"@
}

function New-Issue33RangeTypeFamilySource() {
    return @"
record RangeCarrier {
    r: Range<i32>;
    ri: RangeInclusive<i32>;
    rf: RangeFrom<i32>;
    rt: RangeTo<i32>;
    rti: RangeToInclusive<i32>;
    full: RangeFull;
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue33TransmuteAngleSyntaxSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let out: i32 = unsafe { transmute<i32, i32>(1) }
    return out
}
"@
}

function New-Issue33TransmuteColonColonRejectedSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let out: i32 = unsafe { transmute::<i32, i32>(1) }
    return out
}
"@
}

function New-Issue33ExprHelperTraceSource() {
    return @"
procedure id<T>(value: T) -> T {
    return value
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let p = ^1
    let y: i32 = unsafe { transmute<i32, i32>(1) }
    let z: i32 = id<i32>(y)
    return z
}
"@
}

function New-Issue33AllocTypingTraceSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    region as r {
        let implicit_value: i32 = ^2
        let explicit_value: i32 = r ^ 5
        let _ = implicit_value
        let _ = explicit_value
    }
    return 0
}
"@
}

function New-Issue33AllocImplicitNoRegionSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let bad: i32 = ^1
    return bad
}
"@
}

function New-Issue33AllocExplicitFrozenRegionSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    region as r {
        r~>freeze()
        let bad: i32 = r ^ 1
        let _ = bad
    }
    return 0
}
"@
}

function New-Issue33AllocLoweringBuildSource() {
    return @"
public procedure alloc_probe() -> () {
    region as r {
        let implicit_value: i32 = ^2
        let explicit_value: i32 = r ^ 5
        let _ = implicit_value
        let _ = explicit_value
    }
}
"@
}

function New-Issue33QualifiedNameResolutionPipelineSource() {
    return @"
procedure helper() -> i32 {
    return 1
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let f = probe::helper
    let _ = f
    let out = probe::helper()
    return out
}
"@
}

function New-Issue52PackedFieldRefArgSource() {
    return @"
[[layout(packed)]]
record PackedBox {
    value: i32
}

procedure read_ref(value: i32) -> i32 {
    return value
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let box = PackedBox{ value: 7 }
    let out: i32 = read_ref(box.value)
    return out
}
"@
}

function New-Issue52PackedFieldRefArgUnsafeSource() {
    return @"
[[layout(packed)]]
record PackedBox {
    value: i32
}

procedure read_ref(value: i32) -> i32 {
    return value
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let box = PackedBox{ value: 7 }
    let out: i32 = unsafe { read_ref(box.value) }
    return out
}
"@
}

function New-Issue52IfCaseSectionDriftSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let number: i32 = 1
    let value = if (number == 1) { 10 } else { 20 }
    let output = if number is {
        0 { 0 }
        _: i32 { value }
    }
    return output
}
"@
}

function New-Issue52IfCaseClauseUnreachableIrrefutableSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let value: i32 = 1
    let observed: i32 = if value is {
        _ { 0 }
        1 { 1 }
    }
    return observed
}
"@
}

function New-Issue52IfCaseClauseUnreachableEnumDuplicateSource() {
    return @"
enum DuplicateEnumProbe {
    A
    B
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let value: DuplicateEnumProbe = DuplicateEnumProbe::A
    let observed: i32 = if value is {
        DuplicateEnumProbe::A { 1 }
        DuplicateEnumProbe::A { 2 }
        DuplicateEnumProbe::B { 3 }
    }
    return observed
}
"@
}

function New-Issue52IfCaseClauseUnreachableUnionDuplicateSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let value: i32 | bool = 1
    let observed: i32 = if value is {
        first: i32 { first }
        second: i32 { second + 1 }
        flag: bool { if (flag) { 1 } else { 0 } }
    }
    return observed
}
"@
}

function New-Issue52GenericDefaultTypeArgsSource() {
    return @"
procedure defaulted<T; U = i32>(first: T, second: U) -> U {
    let _ = first
    return second
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let out: i32 = defaulted<bool>(true, 7)
    return out
}
"@
}

function New-Issue52SolveStmtInferenceSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let inferred = 1
    var running = inferred + 2

    var seed: i32 = 3
    {
        using seed as seed_alias
        let seed_plus = seed_alias + running
        let _ = seed_plus
    }

    var counter: i32 = running
    {
        using counter as counter_alias
        counter = counter + 1
        let _ = counter_alias
    }

    return running
}
"@
}

function New-Issue52FieldAccessRecordTraceSource() {
    return @"
record FieldAccessProbe {
    plain: i32
    readonly: const i32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let unique_seed: unique i32 = 7
    let readonly_seed: const i32 = unique_seed
    let probe = FieldAccessProbe{ plain: 3, readonly: readonly_seed }
    let plain_value: i32 = probe.plain
    let probe_unique: unique FieldAccessProbe =
        FieldAccessProbe{ plain: 4, readonly: readonly_seed }
    let readonly_value: const i32 = probe_unique.readonly
    let _ = readonly_value
    return plain_value
}
"@
}

function New-Issue52FieldAccessEnumErrorSource() {
    return @"
enum EnumFieldAccessProbe {
    A
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let value: EnumFieldAccessProbe = EnumFieldAccessProbe::A
    let bad = value.missing
    let _ = bad
    return 0
}
"@
}

function New-Issue52RangeLiftTraceSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let range_value: Range<usize> = 0usize..4usize
    let _ = range_value
    return 0
}
"@
}

function New-Issue52TransmuteInvalidTargetWarningSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let raw_value: u8 = 1u8
    let converted: bool = unsafe { transmute<u8, bool>(raw_value) }
    if (converted) {
        return 1
    }
    return 0
}
"@
}

function New-Issue52AsyncTrySuccessSource() {
    return @"
procedure MaybeValue(flag: bool) -> i32 | bool {
    if (flag == true) {
        return 9
    }
    return false
}

procedure AsyncTryCarrier() -> Async<(), (), i32, bool> {
    let value: i32 = MaybeValue(true)?
    return value
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let state: Async<(), (), i32, bool> = AsyncTryCarrier()
    let _ = state
    return 0
}
"@
}

function New-Issue52AsyncTryInfallibleErrSource() {
    return @"
procedure MaybeValue(flag: bool) -> i32 | bool {
    if (flag == true) {
        return 9
    }
    return false
}

procedure AsyncTryInfallible() -> Async<(), (), i32, !> {
    let value: i32 = MaybeValue(true)?
    return value
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let state: Async<(), (), i32, !> = AsyncTryInfallible()
    let _ = state
    return 0
}
"@
}

function New-Issue52TransitionBorrowNoCallSource() {
    return @"
public modal TransitionBindProbe {
    @Cold {
        value: i32

        public transition warm(step: i32) -> @Warm {
            return TransitionBindProbe@Warm { value: self.value + step }
        }
    }

    @Warm {
        value: i32
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let idle: TransitionBindProbe@Cold = TransitionBindProbe@Cold { value: 3 }
    let _ = idle
    return 0
}
"@
}

function New-Issue52TransitionBorrowCallSource() {
    return @"
public modal TransitionBindProbe {
    @Cold {
        value: i32

        public transition warm(step: i32) -> @Warm {
            return TransitionBindProbe@Warm { value: self.value + step }
        }
    }

    @Warm {
        value: i32
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    var cold_state: unique TransitionBindProbe@Cold =
        TransitionBindProbe@Cold { value: 3 }
    let warm_state: TransitionBindProbe@Warm = cold_state~>warm(2)
    return warm_state.value
}
"@
}

function New-Issue52ExportVisErrSource() {
    return @"
[[export("C")]]
procedure exported_private() -> i32 {
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue52ExportUnknownAbiTraceSource() {
    return New-Issue27ExportUnknownAbiSource
}

function New-Issue52ExternUnknownAbiSource() {
    return @"
extern "NotARealAbi" {
    procedure foreign_unknown() -> i32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue52ExternGenericErrSource() {
    return @"
extern "C" {
    procedure foreign_generic<T>(value: T) -> T
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue52ExternByValueErrSource() {
    return @"
[[layout(C)]]
record ExternByValueDropType {
    value: i32

    procedure drop(~!) -> () {
        return ()
    }
}

extern "C" {
    procedure foreign_by_value(value: ExternByValueDropType) -> i32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue52ExternWfTraceSource() {
    return @"
extern "C" {
    procedure foreign_add(lhs: i32, rhs: i32) -> i32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue544ExternItemListParseTraceSource() {
    return @"
extern "C" {

    procedure foreign_add(lhs: i32, rhs: i32) -> i32;
    procedure foreign_sub(lhs: i32, rhs: i32) -> i32

}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue52SynCallErrSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let not_callable = 7
    let _ = not_callable()
    return 0
}
"@
}

function New-Issue53RecordSelfParamSource() {
    return @"
record RecordSelfParam {
    procedure f(~, self: i32) -> i32 {
        return self
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue53RecordDupParamSource() {
    return @"
record RecordDupParam {
    procedure f(~, x: i32, x: i32) -> i32 {
        return x
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue53RecordNoExplicitReturnSource() {
    return @"
record RecordNoExplicitReturn {
    procedure f(~) -> i32 {
        1
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue53RecordMethodCallTraceSource() {
    return @"
record RecordCall {
    procedure f(~) -> i32 {
        return 1
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let r = RecordCall{}
    let out: i32 = r~>f()
    return out
}
"@
}

function New-Issue53ClassCycleSource() {
    return @"
class A <: B {
}

class B <: A {
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue54ModalNoStatesSource() {
    return @"
modal EmptyModal {
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue54ModalDupStateSource() {
    return @"
modal DupModal {
    @S {
        value: i32
    }

    @S {
        other: i32
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue54ModalStateNameCollisionSource() {
    return @"
modal Clash {
    @Clash {
        value: i32
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue54ModalPayloadDupFieldSource() {
    return @"
modal PayloadDup {
    @S {
        a: i32
        a: i32
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue54ModalStateIntroTraceSource() {
    return @"
modal IntroModal {
    @S {
        value: i32
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let state: IntroModal@S = IntroModal@S { value: 1 }
    let _ = state
    return 0
}
"@
}

function New-Issue54ModalClassParseTraceSource() {
    return @"
modal class ParseOnlyModalClass {
    @Ready {
        value: i32
    }
}

class ParseOnlyPlainClass {
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue54RegionCanonicalSurfaceSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let opts: RegionOptions =
        RegionOptions { stack_size: 0usize, name: "issue54-region-canonical-surface" }
    var r: unique Region@Active = Region::new_scoped(opts)
    let alloc_before: i32 = r~>alloc(7)
    let _ = alloc_before
    unsafe {
        r~>reset_unchecked()
    }
    let alloc_after_reset: i32 = r~>alloc(2)
    let _ = alloc_after_reset
    r~>freeze()
    r~>thaw()
    let alloc_after_thaw: i32 = r~>alloc(3)
    let _ = alloc_after_thaw
    unsafe {
        r~>free_unchecked()
    }
    return 0
}
"@
}

function New-Issue54RegionMarkNotSurfaceSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let opts: RegionOptions = RegionOptions { stack_size: 0usize, name: "issue54-mark" }
    var r: unique Region@Active = Region::new_scoped(opts)
    let _: usize = r~>mark()
    return 0
}
"@
}

function New-Issue54RegionResetToNotSurfaceSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let opts: RegionOptions = RegionOptions { stack_size: 0usize, name: "issue54-reset-to" }
    var r: unique Region@Active = Region::new_scoped(opts)
    let _: () = r~>reset_to(0usize)
    return 0
}
"@
}

function New-Issue54RegionResetUncheckedUnsafeErrSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let opts: RegionOptions =
        RegionOptions { stack_size: 0usize, name: "issue54-reset-unsafe" }
    var r: unique Region@Active = Region::new_scoped(opts)
    let _: Region@Active = r~>reset_unchecked()
    return 0
}
"@
}

function New-Issue54RegionFreeUncheckedUnsafeErrSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let opts: RegionOptions =
        RegionOptions { stack_size: 0usize, name: "issue54-free-unsafe" }
    var r: unique Region@Active = Region::new_scoped(opts)
    let _: Region@Freed = r~>free_unchecked()
    return 0
}
"@
}

function New-Issue54RegionUncheckedUnsafeAllowedSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let reset_opts: RegionOptions =
        RegionOptions { stack_size: 0usize, name: "issue54-reset-allowed" }
    var r_reset: unique Region@Active = Region::new_scoped(reset_opts)
    let reset_region: Region@Active = unsafe {
        r_reset~>reset_unchecked()
    }
    let _ = reset_region

    let free_opts: RegionOptions =
        RegionOptions { stack_size: 0usize, name: "issue54-free-allowed" }
    var r_free: unique Region@Active = Region::new_scoped(free_opts)
    let freed_region: Region@Freed = unsafe {
        r_free~>free_unchecked()
    }
    let _ = freed_region
    return 0
}
"@
}

function New-Issue54RegionAllocLoweringBuildSource() {
    return @"
public procedure alloc_probe() -> () {
    let opts: RegionOptions =
        RegionOptions { stack_size: 0usize, name: "issue54-alloc-lowering" }
    var r: unique Region@Active = Region::new_scoped(opts)
    let value: i32 = r~>alloc(3)
    let _ = value
}
"@
}

function New-Issue54RegionResetLoweringBuildSource() {
    return @"
public procedure reset_probe() -> () {
    let opts: RegionOptions =
        RegionOptions { stack_size: 0usize, name: "issue54-reset-lowering" }
    var r: unique Region@Active = Region::new_scoped(opts)
    let before_reset: i32 = r~>alloc(7)
    let _ = before_reset
    unsafe {
        r~>reset_unchecked()
    }
    r~>freeze()
    return ()
}
"@
}

function New-Issue54RegionFreezeThawFreeLoweringBuildSource() {
    return @"
public procedure lifecycle_probe() -> () {
    let thaw_opts: RegionOptions =
        RegionOptions { stack_size: 0usize, name: "issue54-freeze-thaw-lowering" }
    var thaw_region: unique Region@Active = Region::new_scoped(thaw_opts)
    thaw_region~>freeze()
    thaw_region~>thaw()
    let thaw_value: i32 = thaw_region~>alloc(4)
    let _ = thaw_value

    let free_opts: RegionOptions =
        RegionOptions { stack_size: 0usize, name: "issue54-free-frozen-lowering" }
    var free_region: unique Region@Active = Region::new_scoped(free_opts)
    free_region~>freeze()
    let freed_region: Region@Freed = unsafe {
        free_region~>free_unchecked()
    }
    let _ = freed_region
    return ()
}
"@
}

function New-Issue54ResolveDuplicateBindingDiagnosticSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let duplicate_name: i32 = 1
    let duplicate_name: i32 = 2
    return 0
}
"@
}

function New-Issue55ModalFieldTypingTraceSource() {
    return @"
public modal Issue55ModalFieldTrace {
    @Ready {
        value: i32
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let ready: Issue55ModalFieldTrace@Ready = Issue55ModalFieldTrace@Ready { value: 9 }
    let direct: i32 = ready.value
    let perm_ready: const Issue55ModalFieldTrace@Ready = ready
    let via_perm: const i32 = perm_ready.value
    let _ = via_perm
    return direct
}
"@
}

function New-Issue55ModalFieldMissingDiagSource() {
    return @"
public modal Issue55ModalFieldMissing {
    @Ready {
        value: i32
    }
    @Idle {
        code: i32
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let ready: Issue55ModalFieldMissing@Ready = Issue55ModalFieldMissing@Ready { value: 1 }
    let bad = ready.code
    let _ = bad
    return 0
}
"@
}

function New-Issue55ModalFieldGeneralDiagSource() {
    return @"
public modal Issue55ModalFieldGeneral {
    @Ready {
        value: i32
    }
    @Idle {
        code: i32
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let general: Issue55ModalFieldGeneral = widen(Issue55ModalFieldGeneral@Ready { value: 1 })
    let bad = general.value
    let _ = bad
    return 0
}
"@
}

function New-Issue55ModalFieldCrossModuleMainSource() {
    return @"
import mod

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let remote: mod::Issue55RemoteModal@Ready = mod::make_remote()
    let value: i32 = remote.value
    return value
}
"@
}

function New-Issue55ModalFieldCrossModuleModSource() {
    return @"
public modal Issue55RemoteModal {
    @Ready {
        value: i32
    }
}

public procedure make_remote() -> Issue55RemoteModal@Ready {
    return Issue55RemoteModal@Ready { value: 11 }
}
"@
}

function New-Issue56ModalCallTraceSource() {
    return @"
public modal Issue56Trace {
    @Idle {
        value: i32

        public procedure read(~) -> i32 {
            return self.value
        }

        public transition step(delta: i32) -> @Active {
            return Issue56Trace@Active { value: self.value + delta }
        }
    }

    @Active {
        value: i32

        public procedure read(~) -> i32 {
            return self.value
        }
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    var idle: unique Issue56Trace@Idle = Issue56Trace@Idle { value: 4 }
    let active: Issue56Trace@Active = idle~>step(3)
    let out: i32 = active~>read()
    return out
}
"@
}

function New-Issue56TransitionTargetDiagSource() {
    return @"
public modal Issue56TargetDiag {
    @Idle {
        value: i32

        public transition step() -> @Missing {
            return Issue56TargetDiag@Idle { value: self.value + 1 }
        }
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue56TransitionSourcePermDiagSource() {
    return @"
public modal Issue56PermDiag {
    @Idle {
        value: i32

        public transition step() -> @Idle {
            return Issue56PermDiag@Idle { value: self.value + 1 }
        }
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let base: Issue56PermDiag@Idle = Issue56PermDiag@Idle { value: 1 }
    let view: const Issue56PermDiag@Idle = base
    let next: Issue56PermDiag@Idle = view~>step()
    let _ = next
    return 0
}
"@
}

function New-Issue56VisibilitySameModuleSource() {
    return @"
public modal Issue56VisibilitySameModule {
    @Idle {
        value: i32

        private procedure read_private(~) -> i32 {
            return self.value
        }

        procedure read_internal(~) -> i32 {
            return self.value + 1
        }

        transition promote() -> @Ready {
            return Issue56VisibilitySameModule@Ready { value: self.value + 3 }
        }
    }

    @Ready {
        value: i32

        procedure read_ready(~) -> i32 {
            return self.value
        }
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    var idle: unique Issue56VisibilitySameModule@Idle =
        Issue56VisibilitySameModule@Idle { value: 9 }
    let p: i32 = idle~>read_private()
    let r: i32 = idle~>read_internal()
    let ready: Issue56VisibilitySameModule@Ready = idle~>promote()
    let s: i32 = ready~>read_ready()
    return p + r + s
}
"@
}

function New-Issue56VisibilityCrossModuleMainSource() {
    return @"
import mod

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let remote: mod::Issue56Remote@Idle = mod::new_remote()
    let value: i32 = remote~>hidden()
    return value
}
"@
}

function New-Issue56VisibilityCrossModuleInternalMainSource() {
    return @"
import mod

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let remote: mod::Issue56Remote@Idle = mod::new_remote()
    let value: i32 = remote~>internal_ok()
    return value
}
"@
}

function New-Issue56VisibilityCrossModuleModSource() {
    return @"
public modal Issue56Remote {
    @Idle {
        value: i32

        private procedure hidden(~) -> i32 {
            return self.value
        }

        procedure internal_ok(~) -> i32 {
            return self.value + 1
        }
    }
}

public procedure new_remote() -> Issue56Remote@Idle {
    return Issue56Remote@Idle { value: 5 }
}
"@
}

function New-Issue617ProtectedVisibilityRejectedSource() {
    return @"
protected procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
}

function New-Issue618ParseItemsConsTraceSource() {
    return @"
procedure first_helper() -> i32 {
    return 1
}

procedure second_helper(value: i32) -> i32 {
    return value + 1
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return second_helper(first_helper())
}
"@
}

function New-Issue620QuoteStmtProbeSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    comptime {
        let quoted = quote {
            return 1
        }
        let _ = quoted
    }

    let _ = ctx
    return 0
}
"@
}

function New-Issue621ReservedKeywordIdentifierSource() {
    return @"
using foo::procedure

public procedure record(move ctx: Context) -> i32 {
    let _ = ctx
    let if: i32 = 0
    return 0
}
"@
}

function New-Issue57ConfusableIdentifierSource() {
    $cyrillicScope =
        ([string][char]0x0455) +
        ([string][char]0x0441) +
        ([string][char]0x043E) +
        ([string][char]0x0440) +
        ([string][char]0x0435)
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let scope: i32 = 1
    let ${cyrillicScope}: i32 = 2
    return scope
}
"@
}

function New-Issue57MixedScriptIdentifierSource() {
    $cyrillicA = [string][char]0x0430
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let p${cyrillicA}yload: i32 = 1
    return 0
}
"@
}

function New-Issue57SingleScriptUnicodeIdentifierSource() {
    $greekEta = [string][char]0x0397
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let ${greekEta}: i32 = 1
    return ${greekEta}
}
"@
}

function New-Issue58BytesSpecSurfaceSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let view: bytes@View = bytes::view_string("abc")
    let len: usize = bytes::length(view)
    let empty: bool = bytes::is_empty(view)
    let _ = len
    let _ = empty
    return 0
}
"@
}

function New-Issue58BytesAsSliceSource() {
    return @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let view: bytes@View = bytes::view_string("abc")
    let slice = bytes::as_slice(view)
    let _ = slice
    return 0
}
"@
}

function New-Issue59CapabilitiesSurfaceSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let exists: bool = ctx.fs~>exists("Compiler")
    let heap_quota: $HeapAllocator = ctx.heap~>with_quota(64usize)
    let cpu: $ExecutionDomain = ctx~>cpu()
    let name: string@View = cpu~>name()
    let status: i32 = ctx.sys~>run("echo issue59")
    let _ = exists
    let _ = heap_quota
    let _ = name
    return status
}
'@
}

function New-Issue59AllocRawOutsideUnsafeSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let heap: $HeapAllocator = ctx.heap
    let _ptr: *mut u8 = heap~>alloc_raw(8usize)
    return 0
}
'@
}

function New-Issue59DeallocRawOutsideUnsafeSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let heap: $HeapAllocator = ctx.heap
    var ptr: *mut u8 = null

    unsafe {
        ptr = heap~>alloc_raw(8usize)
    }

    heap~>dealloc_raw(ptr, 8usize)
    return 0
}
'@
}

function New-Issue59MethodCapabilityLeakSource() {
    return @'
record Runner {
    procedure run(~) -> i32 {
        let sys: System = unsafe { transmute<(), System>(()) }
        return sys~>run("echo issue59 method leak")
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let runner = Runner{}
    return runner~>run()
}
'@
}

function New-Issue59MethodCapabilityFlowAllowedSource() {
    return @'
record Runner {
    procedure run(~, sys: System) -> i32 {
        return sys~>run("echo issue59 method ok")
    }
}

public procedure main(move ctx: Context) -> i32 {
    let runner = Runner{}
    return runner~>run(ctx.sys)
}
'@
}

function New-Issue60TypeAnnotOptParseTraceSource() {
    return @'
procedure identity(value: i32) -> i32 {
    return value
}

public procedure main(move ctx: Context) -> i32 {
    let inferred = identity(1)
    let annotated: i32 = identity(2)
    let _ = ctx
    let _ = inferred
    return annotated
}
'@
}

function New-Issue61KeyPathResolutionSuccessSource() {
    return @'
record Issue61KeyPathProbe {
    leaf: i32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    var gate: shared Issue61KeyPathProbe = Issue61KeyPathProbe{ leaf: 4 }
    var slots: shared [i32; 2] = [7, 9]

    # gate.leaf read {
        let _ = gate.leaf
    }

    # slots[1usize] read {
        let _ = slots[1usize]
    }
    return 0
}
'@
}

function New-Issue61KeyBlockUnresolvedRootSource() {
    return @'
record Issue61KeyPathProbe {
    leaf: i32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    # missing_root.leaf read {
        let _ = 0
    }
    return 0
}
'@
}

function New-Issue61KeyBlockUnresolvedIndexSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    var slots: shared [i32; 2] = [3, 5]
    # slots[missing_index] read {
        let _ = slots[0usize]
    }
    return 0
}
'@
}

function New-Issue61DispatchKeyClauseUnresolvedIndexSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    var slots: shared [i32; 2] = [3, 5]
    let reduced: i32 = dispatch i in 0usize..2usize key slots[missing_index] read [reduce: +] {
        let _ = i
        1
    }
    return reduced
}
'@
}

function New-Issue514ListSmallStepParseTraceSource() {
    return @'
procedure pair_head(left: i32, right: i32) -> i32 {
    let _ = right
    return left
}

procedure noop() -> i32 {
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let array_values = [1, 2, 3]
    let tuple_values = (4, 5, 6)
    let selected = pair_head(7, 8)
    let zero = noop()
    let _ = array_values
    let _ = tuple_values
    let _ = zero
    return selected
}
'@
}

function New-Issue514TrailingCommaEndSetConformanceSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue514TrailingCommaErrSingleLineCallSource() {
    return @'
procedure issue514_sum(left: i32, right: i32) -> i32 {
    return left + right
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return issue514_sum(1, 2,)
}
'@
}

function New-Issue514TupleExprSingletonCommaRejectedSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return (1,)
}
'@
}

function New-Issue510EnumDiscHexSource() {
    return @'
enum Issue510Hex {
    A = 0x10
    B
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue510EnumDiscBinarySource() {
    return @'
enum Issue510Binary {
    A = 0b1010
    B
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue510EnumDiscOctalSource() {
    return @'
enum Issue510Octal {
    A = 0o12
    B
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue510EnumDiscUnderscoreSource() {
    return @'
enum Issue510Underscore {
    A = 1_000
    B
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue510EnumDiscSuffixSource() {
    return @'
enum Issue510Suffix {
    A = 42u32
    B
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue510EnumDiscU64MaxSource() {
    return @'
enum Issue510U64Max {
    A = 18446744073709551615u64
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue510EnumDiscOverU64Source() {
    return @'
enum Issue510OverU64 {
    A = 18446744073709551616u128
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue510EnumDiscDuplicateSource() {
    return @'
enum Issue510Duplicate {
    A = 0x10
    B = 16u8
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue510EnumDiscU64MaxPlusImplicitSource() {
    return @'
enum Issue510U64Overflow {
    A = 18446744073709551615u64
    B
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue559EnumEmptyRejectedSource() {
    return @'
public enum Issue559Empty {
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue559EnumNonEmptyAcceptedSource() {
    return @'
public enum Issue559NonEmpty {
    A
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let value: Issue559NonEmpty = Issue559NonEmpty::A
    return if value is {
        Issue559NonEmpty::A { 0 }
    }
}
'@
}

function New-Issue511EqClassBoundSuccessSource() {
    return @'
procedure requires_eq<T <: Eq>(value: T) -> i32 {
    let _ = value
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return requires_eq<i32>(1)
}
'@
}

function New-Issue511BitcopyClassBoundSuccessSource() {
    return @'
procedure requires_bitcopy<T <: Bitcopy>(value: T) -> T {
    return value
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let observed: i32 = requires_bitcopy<i32>(7)
    return observed - 7
}
'@
}

function New-Issue511PipeRefinementAndInvariantSyntaxSource() {
    return @'
type PositiveProbe = i32 |: { self > 0 }

procedure loop_probe() -> i32 {
    var index: i32 = 0
    loop index < 3 |: { true } {
        index = index + 1
    }
    return index
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let _n: PositiveProbe = 1
    return loop_probe() - 3
}
'@
}

function New-Issue511PipeLoopIteratorInvariantSyntaxSource() {
    return @'
procedure loop_probe() -> i32 {
    var sum: i32 = 0
    loop value in [1, 2, 3] |: { true } {
        sum = sum + value
    }
    return sum
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return loop_probe() - 6
}
'@
}

function New-Issue511PipeLoopInfiniteInvariantSyntaxSource() {
    return @'
procedure loop_probe() -> i32 {
    var index: i32 = 0
    loop |: { true } {
        index = index + 1
        break
    }
    return index
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return loop_probe() - 1
}
'@
}

function New-Issue511CloneSignatureMismatchSource() {
    return @'
record Issue511BadClone <: Clone {
    value: i32

    procedure clone(~) -> i32 {
        return self.value
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue511StepSignatureMismatchSource() {
    return @'
record Issue511BadStep <: Step {
    value: i32

    procedure successor(~) -> i32 {
        return self.value
    }

    procedure predecessor(~) -> i32 {
        return self.value
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue511LegacyWhereConstraintRejectedSource() {
    return @'
procedure legacy_bound<T>(value: T) -> i32
where Bitcopy(T)
{
    let _ = value
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue511LegacyWhereLoopInvariantRejectedSource() {
    return @'
procedure loop_probe() -> i32 {
    var index: i32 = 0
    loop index < 3 where { true } {
        index = index + 1
    }
    return index
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return loop_probe() - 3
}
'@
}

function New-Issue511UserAssocProjectionSuccessSource() {
    return @'
class Issue511IterLike {
    type Item

    procedure next(~!) -> Self::Item | ()
}

record Issue511IterGood <: Issue511IterLike {
    value: i32
    type Item = i32

    procedure next(~!) -> i32 | () {
        return self.value
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue511UserAssocProjectionMismatchSource() {
    return @'
class Issue511IterLike {
    type Item

    procedure next(~!) -> Self::Item | ()
}

record Issue511IterBad <: Issue511IterLike {
    type Item = i32

    procedure next(~!) -> bool | () {
        return true
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue511BuiltinIteratorAssocProjectionSuccessSource() {
    return @'
record Issue511IteratorGood <: Iterator {
    type Item = i32

    procedure next(~!) -> i32 | () {
        return ()
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue511BuiltinIteratorAssocProjectionMismatchSource() {
    return @'
record Issue511IteratorBad <: Iterator {
    type Item = i32

    procedure next(~!) -> bool | () {
        return true
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue511BuiltinIteratorMissingAssocBindingSource() {
    return @'
record Issue511IteratorMissingAssoc <: Iterator {
    procedure next(~!) -> i32 | () {
        return ()
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue512TypeRefApplySource() {
    return @'
record Issue512Box<T> {
    value: T
}

record Issue512Payload {
    value: i32
}

type Issue512Applied = Issue512Box<Issue512Payload>

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue512CallTypeArgsSource() {
    return @'
procedure issue512_id<T>(value: T) -> T {
    return value
}

procedure issue512_zero<T>() -> i32 {
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let a: i32 = issue512_id<i32>(7)
    let b: i32 = issue512_zero<i32>()
    let _ = a
    let _ = b
    return 0
}
'@
}

function New-Issue512RangeFamilySource() {
    return @'
record Issue512RangeCarrier {
    r: Range<i32>;
    ri: RangeInclusive<i32>;
    rf: RangeFrom<i32>;
    rt: RangeTo<i32>;
    rti: RangeToInclusive<i32>;
    full: RangeFull;
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue512TopoCycleMainSource() {
    return @'
import a
import b

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue512TopoCycleModuleASource() {
    return @'
import b

public let a_val: i32 = b::b_val
'@
}

function New-Issue512TopoCycleModuleBSource() {
    return @'
import a

public let b_val: i32 = a::a_val
'@
}

function New-Issue512ModulePrefixCurrentMainSource() {
    return @'
import consumer

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return consumer::ReadObserved()
}
'@
}

function New-Issue512ModulePrefixCurrentConsumerSource() {
    return @'
public let Observed: i32 = sub::Support::ReadSupportSeed() + 2

public procedure ReadObserved() -> i32 {
    return Observed
}
'@
}

function New-Issue512ModulePrefixCurrentSupportSource() {
    return @'
procedure SupportSeedBase() -> i32 {
    return 11
}

public let SupportSeed: i32 = SupportSeedBase()

public procedure ReadSupportSeed() -> i32 {
    return SupportSeed
}
'@
}

function New-Issue545ResolveModulePathDirectMainSource() {
    return @'
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return probe::consumer::ReadObserved()
}
'@
}

function New-Issue545ResolveModulePathDirectConsumerSource() {
    return @'
public let Observed: i32 = probe::sub::Support::ReadSupportSeed() + 2

public procedure ReadObserved() -> i32 {
    return Observed
}
'@
}

function New-Issue545ResolveModulePathDirectSupportSource() {
    return @'
procedure SupportSeedBase() -> i32 {
    return 11
}

public let SupportSeed: i32 = SupportSeedBase()

public procedure ReadSupportSeed() -> i32 {
    return SupportSeed
}
'@
}

function New-Issue546ResolveImportCurrentLocalMainSource() {
    return @'
import consumer

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return consumer::ReadLocalSupport()
}
'@
}

function New-Issue546ResolveImportCurrentLocalConsumerSource() {
    return @'
using sub::Support::ReadLocalSupportValue

public procedure ReadLocalSupport() -> i32 {
    return ReadLocalSupportValue()
}
'@
}

function New-Issue546ResolveImportCurrentLocalSupportSource() {
    return @'
public procedure ReadLocalSupportValue() -> i32 {
    return 13
}
'@
}

function New-Issue546ImportCoveredMainSource() {
    return @'
import dep::Support
using dep::Support::ReadImportedMetric

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return ReadImportedMetric()
}
'@
}

function New-Issue546ImportMissingMainSource() {
    return @'
using dep::Support::ReadImportedMetric

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return ReadImportedMetric()
}
'@
}

function New-Issue546ResolveImportErrMainSource() {
    return @'
import missing

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
}

function New-Issue546DepSupportSource() {
    return @'
public procedure ReadImportedMetric() -> i32 {
    return 9
}
'@
}

function New-Issue546TransitiveVisibilityMainSource() {
    return @'
import liba

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return libb::ReadTransitiveMetric()
}
'@
}

function New-Issue546TransitiveVisibilityLibASource() {
    return @'
import libb

public procedure ReadLibAValue() -> i32 {
    return libb::ReadTransitiveMetric()
}
'@
}

function New-Issue546TransitiveVisibilityLibBSource() {
    return @'
public procedure ReadTransitiveMetric() -> i32 {
    return 21
}
'@
}

function Invoke-ConformanceCase {
    param(
        [string]$Id,
        [string[]]$ExtraArgs,
        [int]$ExpectExit,
        [int]$MinErrors,
        [int]$MaxErrors
    )

    $caseRoot = Join-Path $workRoot $Id
    New-Item -ItemType Directory -Path $caseRoot -Force | Out-Null

    $manifestPath = Join-Path $caseRoot "Cursive.toml"
    $sourcePath = Join-Path $caseRoot "Main.cursive"
    [System.IO.File]::WriteAllLines($manifestPath, $manifestLines)
    [System.IO.File]::WriteAllText($sourcePath, (New-BulkErrorSource 160))

    $diagJsonPath = Join-Path $caseRoot "diag.json"
    $stderrPath = Join-Path $caseRoot "stderr.txt"

    Push-Location $caseRoot
    try {
        $args = @("--incremental", "off", "--check", "--diag-json", "--quiet")
        $args += $ExtraArgs
        $args += @("Main.cursive")
        & $CompilerPath @args 1> $diagJsonPath 2> $stderrPath
        $exitCode = $LASTEXITCODE
    } finally {
        Pop-Location
    }

    if ($exitCode -ne $ExpectExit) {
        throw "Case '$Id' expected exit $ExpectExit but got $exitCode."
    }

    if (-not (Test-Path $diagJsonPath)) {
        throw "Case '$Id' did not produce diagnostic JSON output."
    }

    $jsonText = Get-Content -Path $diagJsonPath -Raw
    if ([string]::IsNullOrWhiteSpace($jsonText)) {
        throw "Case '$Id' produced empty diagnostic JSON output."
    }

    $diagJson = $jsonText | ConvertFrom-Json
    if ($null -eq $diagJson.diagnostics) {
        throw "Case '$Id' diagnostic JSON missing 'diagnostics' array."
    }

    $errorCount = @($diagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count

    if ($errorCount -lt $MinErrors -or $errorCount -gt $MaxErrors) {
        throw "Case '$Id' expected error count in [$MinErrors, $MaxErrors], observed $errorCount."
    }

    Write-Host "[compiler-static] ${Id}: exit=$exitCode errors=$errorCount"
}

function Invoke-StaticCheckCase {
    $caseRoot = Join-Path $workRoot "static_check_mapping"
    New-Item -ItemType Directory -Path $caseRoot -Force | Out-Null

    [System.IO.File]::WriteAllLines((Join-Path $caseRoot "Cursive.toml"), $manifestLines)
    [System.IO.File]::WriteAllText((Join-Path $caseRoot "Main.cursive"), (New-StaticCheckSource))

    $diagJsonPath = Join-Path $caseRoot "diag.json"
    $stderrPath = Join-Path $caseRoot "stderr.txt"
    Push-Location $caseRoot
    try {
        & $CompilerPath --incremental off --check --diag-json --quiet Main.cursive 1> $diagJsonPath 2> $stderrPath
        $exitCode = $LASTEXITCODE
    } finally {
        Pop-Location
    }
    if ($exitCode -ne 1) {
        throw "Case 'static_check_mapping' expected exit 1 but got $exitCode."
    }
    $diagJson = (Get-Content -Path $diagJsonPath -Raw) | ConvertFrom-Json
    $errorCount = @($diagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -lt 1) {
        throw "Case 'static_check_mapping' expected compile-time static errors."
    }
    Write-Host "[compiler-static] static_check_mapping: exit=$exitCode errors=$errorCount"
}

function Invoke-ExpectedDiagCodeCase {
    param(
        [string]$Id,
        [string]$Source,
        [string[]]$ExpectedCodes,
        [string[]]$ExtraArgs = @()
    )

    $caseRoot = Join-Path $workRoot $Id
    New-Item -ItemType Directory -Path $caseRoot -Force | Out-Null

    [System.IO.File]::WriteAllLines((Join-Path $caseRoot "Cursive.toml"), $manifestLines)
    [System.IO.File]::WriteAllText((Join-Path $caseRoot "Main.cursive"), $Source)

    $diagJsonPath = Join-Path $caseRoot "diag.json"
    $stderrPath = Join-Path $caseRoot "stderr.txt"
    Push-Location $caseRoot
    try {
        $args = @("--incremental", "off", "--check", "--diag-json", "--quiet")
        $args += $ExtraArgs
        $args += @("Main.cursive")
        & $CompilerPath @args 1> $diagJsonPath 2> $stderrPath
        $exitCode = $LASTEXITCODE
    } finally {
        Pop-Location
    }

    if ($exitCode -ne 1) {
        throw "Case '$Id' expected exit 1 but got $exitCode."
    }

    $diagJson = (Get-Content -Path $diagJsonPath -Raw) | ConvertFrom-Json
    $errorCount = @($diagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -lt 1) {
        throw "Case '$Id' expected at least one compile-time error."
    }

    foreach ($expectedCode in $ExpectedCodes) {
        $ifIsCount = @($diagJson.diagnostics | Where-Object {
            $_.code -eq $expectedCode
        }).Count
        if ($ifIsCount -lt 1) {
            throw "Case '$Id' expected diagnostic code '$expectedCode'."
        }
    }

    Write-Host "[compiler-static] ${Id}: exit=$exitCode errors=$errorCount matched_codes=$($ExpectedCodes -join ',')"
}

function Invoke-ExpectedDiagCodeCaseWithForbiddenCodes {
    param(
        [string]$Id,
        [string]$Source,
        [string[]]$ExpectedCodes,
        [string[]]$ForbiddenCodes
    )

    $caseRoot = Join-Path $workRoot $Id
    New-Item -ItemType Directory -Path $caseRoot -Force | Out-Null

    [System.IO.File]::WriteAllLines((Join-Path $caseRoot "Cursive.toml"), $manifestLines)
    [System.IO.File]::WriteAllText((Join-Path $caseRoot "Main.cursive"), $Source)

    $diagJsonPath = Join-Path $caseRoot "diag.json"
    $stderrPath = Join-Path $caseRoot "stderr.txt"
    Push-Location $caseRoot
    try {
        & $CompilerPath --incremental off --check --diag-json --quiet Main.cursive 1> $diagJsonPath 2> $stderrPath
        $exitCode = $LASTEXITCODE
    } finally {
        Pop-Location
    }

    if ($exitCode -ne 1) {
        throw "Case '$Id' expected exit 1 but got $exitCode."
    }

    $diagJson = (Get-Content -Path $diagJsonPath -Raw) | ConvertFrom-Json
    $errorCount = @($diagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -lt 1) {
        throw "Case '$Id' expected at least one compile-time error."
    }

    foreach ($expectedCode in $ExpectedCodes) {
        $ifIsCount = @($diagJson.diagnostics | Where-Object {
            $_.code -eq $expectedCode
        }).Count
        if ($ifIsCount -lt 1) {
            throw "Case '$Id' expected diagnostic code '$expectedCode'."
        }
    }

    foreach ($forbiddenCode in $ForbiddenCodes) {
        $ifIsCount = @($diagJson.diagnostics | Where-Object {
            $_.code -eq $forbiddenCode
        }).Count
        if ($ifIsCount -gt 0) {
            throw "Case '$Id' unexpectedly emitted forbidden diagnostic code '$forbiddenCode'."
        }
    }

    Write-Host "[compiler-static] ${Id}: exit=$exitCode errors=$errorCount matched_codes=$($ExpectedCodes -join ',') forbidden_absent=$($ForbiddenCodes -join ',')"
}

function Invoke-ExpectedSuccessCase {
    param(
        [string]$Id,
        [string]$Source,
        [string[]]$ExtraArgs = @()
    )

    $caseRoot = Join-Path $workRoot $Id
    New-Item -ItemType Directory -Path $caseRoot -Force | Out-Null

    [System.IO.File]::WriteAllLines((Join-Path $caseRoot "Cursive.toml"), $manifestLines)
    [System.IO.File]::WriteAllText((Join-Path $caseRoot "Main.cursive"), $Source)

    $diagJsonPath = Join-Path $caseRoot "diag.json"
    $stderrPath = Join-Path $caseRoot "stderr.txt"
    Push-Location $caseRoot
    try {
        $args = @("--incremental", "off", "--check", "--diag-json", "--quiet")
        $args += $ExtraArgs
        $args += @("Main.cursive")
        & $CompilerPath @args 1> $diagJsonPath 2> $stderrPath
        $exitCode = $LASTEXITCODE
    } finally {
        Pop-Location
    }

    if ($exitCode -ne 0) {
        throw "Case '$Id' expected exit 0 but got $exitCode."
    }

    $diagJson = (Get-Content -Path $diagJsonPath -Raw) | ConvertFrom-Json
    $errorCount = @($diagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case '$Id' expected zero compile-time errors, observed $errorCount."
    }

    Write-Host "[compiler-static] ${Id}: exit=$exitCode errors=$errorCount"
}

function Invoke-ExpectedWarningCodeCase {
    param(
        [string]$Id,
        [string]$Source,
        [string[]]$ExpectedWarningCodes
    )

    $caseRoot = Join-Path $workRoot $Id
    New-Item -ItemType Directory -Path $caseRoot -Force | Out-Null

    [System.IO.File]::WriteAllLines((Join-Path $caseRoot "Cursive.toml"), $manifestLines)
    [System.IO.File]::WriteAllText((Join-Path $caseRoot "Main.cursive"), $Source)

    $diagJsonPath = Join-Path $caseRoot "diag.json"
    $stderrPath = Join-Path $caseRoot "stderr.txt"
    Push-Location $caseRoot
    try {
        & $CompilerPath --incremental off --check --diag-json --quiet Main.cursive 1> $diagJsonPath 2> $stderrPath
        $exitCode = $LASTEXITCODE
    } finally {
        Pop-Location
    }

    if ($exitCode -ne 0) {
        throw "Case '$Id' expected exit 0 but got $exitCode."
    }

    $diagJson = (Get-Content -Path $diagJsonPath -Raw) | ConvertFrom-Json
    $errorCount = @($diagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case '$Id' expected zero compile-time errors, observed $errorCount."
    }

    foreach ($expectedCode in $ExpectedWarningCodes) {
        $ifIsCount = @($diagJson.diagnostics | Where-Object {
            $_.severity -eq "warning" -and $_.code -eq $expectedCode
        }).Count
        if ($ifIsCount -lt 1) {
            throw "Case '$Id' expected warning code '$expectedCode'."
        }
    }

    Write-Host "[compiler-static] ${Id}: exit=$exitCode errors=$errorCount matched_warnings=$($ExpectedWarningCodes -join ',')"
}

function Invoke-ExpectedFailureCase {
    param(
        [string]$Id,
        [string]$Source
    )

    $caseRoot = Join-Path $workRoot $Id
    New-Item -ItemType Directory -Path $caseRoot -Force | Out-Null

    [System.IO.File]::WriteAllLines((Join-Path $caseRoot "Cursive.toml"), $manifestLines)
    [System.IO.File]::WriteAllText((Join-Path $caseRoot "Main.cursive"), $Source)

    $diagJsonPath = Join-Path $caseRoot "diag.json"
    $stderrPath = Join-Path $caseRoot "stderr.txt"
    Push-Location $caseRoot
    try {
        & $CompilerPath --incremental off --check --diag-json --quiet Main.cursive 1> $diagJsonPath 2> $stderrPath
        $exitCode = $LASTEXITCODE
    } finally {
        Pop-Location
    }

    if ($exitCode -ne 1) {
        throw "Case '$Id' expected exit 1 but got $exitCode."
    }

    $diagJson = (Get-Content -Path $diagJsonPath -Raw) | ConvertFrom-Json
    $errorCount = @($diagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -lt 1) {
        throw "Case '$Id' expected at least one compile-time error."
    }

    Write-Host "[compiler-static] ${Id}: exit=$exitCode errors=$errorCount"
}

function Invoke-RegistryCompletenessCase {
    $caseRoot = Join-Path $workRoot "registry_completeness"
    New-Item -ItemType Directory -Path $caseRoot -Force | Out-Null

    $generatorPath = Join-Path $workspaceRoot "cursive\\tools\\generate_static_rule_registry.ps1"
    $mappingPath = Join-Path $workspaceRoot "cursive\\tools\\static_rule_mapping.json"
    $registryOutPath = Join-Path $caseRoot "static_rule_registry.inc"
    $reportOutPath = Join-Path $caseRoot "static_rule_registry_report.json"

    if (-not (Test-Path $generatorPath)) {
        throw "Case 'registry_completeness' missing generator script: $generatorPath"
    }
    if (-not (Test-Path $mappingPath)) {
        throw "Case 'registry_completeness' missing mapping file: $mappingPath"
    }

    & powershell -NoProfile -ExecutionPolicy Bypass -File $generatorPath -RepoRoot $workspaceRoot -MappingPath $mappingPath -OutputPath $registryOutPath -ReportPath $reportOutPath -Strict
    $exitCode = $LASTEXITCODE
    if ($exitCode -ne 0) {
        throw "Case 'registry_completeness' expected generator exit 0 but got $exitCode."
    }
    if (-not (Test-Path $reportOutPath)) {
        throw "Case 'registry_completeness' missing report output."
    }

    $report = (Get-Content -Path $reportOutPath -Raw) | ConvertFrom-Json
    $unmappedCount = @($report.unmapped_rules).Count
    $conflictCount = @($report.family_conflicts).Count
    if ($unmappedCount -ne 0 -or $conflictCount -ne 0) {
        throw "Case 'registry_completeness' expected zero unmapped/conflicts; observed unmapped=$unmappedCount conflicts=$conflictCount."
    }

    Write-Host "[compiler-static] registry_completeness: rules=$($report.rule_count) unmapped=$unmappedCount conflicts=$conflictCount"
}

function Find-PatternHits {
    param(
        [string[]]$Roots,
        [string]$Pattern,
        [string[]]$AllowedRelativePaths = @()
    )

    $allowedFullPaths = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
    foreach ($relativePath in $AllowedRelativePaths) {
        $fullPath = [System.IO.Path]::GetFullPath((Join-Path $workspaceRoot $relativePath))
        [void]$allowedFullPaths.Add($fullPath)
    }

    $hits = New-Object System.Collections.Generic.List[object]
    foreach ($root in $Roots) {
        if (-not (Test-Path $root)) {
            continue
        }

        $files = Get-ChildItem -Path $root -Recurse -File -Include *.cpp,*.h,*.hpp,*.inc
        foreach ($file in $files) {
            $fullPath = [System.IO.Path]::GetFullPath($file.FullName)
            $lineNumber = 0
            foreach ($line in Get-Content -Path $fullPath) {
                $lineNumber += 1
                if ($line.TrimStart().StartsWith("//")) {
                    continue
                }
                if ($line -match $Pattern) {
                    if ($allowedFullPaths.Contains($fullPath)) {
                        continue
                    }
                    $hits.Add([PSCustomObject]@{
                        File = $fullPath
                        Line = $lineNumber
                        Text = $line.Trim()
                    }) | Out-Null
                }
            }
        }
    }

    return $hits
}

function Invoke-Issue16DiagnosticSpecSyncCase {
    $generatorPath = Join-Path $workspaceRoot "cursive\\tools\\generate_diagnostic_registry.ps1"
    $validatorPath = Join-Path $workspaceRoot "cursive\\tools\\validate_diagnostic_spec_sync.ps1"

    if (-not (Test-Path $generatorPath)) {
        throw "Case 'issue16_diagnostic_spec_sync' missing generator script: $generatorPath"
    }
    if (-not (Test-Path $validatorPath)) {
        throw "Case 'issue16_diagnostic_spec_sync' missing validator script: $validatorPath"
    }

    & powershell -NoProfile -ExecutionPolicy Bypass -File $generatorPath -RepoRoot $workspaceRoot
    $generatorExit = $LASTEXITCODE
    if ($generatorExit -ne 0) {
        throw "Case 'issue16_diagnostic_spec_sync' expected generator exit 0 but got $generatorExit."
    }

    & powershell -NoProfile -ExecutionPolicy Bypass -File $validatorPath -RepoRoot $workspaceRoot
    $validatorExit = $LASTEXITCODE
    if ($validatorExit -ne 0) {
        throw "Case 'issue16_diagnostic_spec_sync' expected validator exit 0 but got $validatorExit."
    }

    Write-Host "[compiler-static] issue16_diagnostic_spec_sync: registry_exit=$generatorExit sync_exit=$validatorExit"
}

function Invoke-Issue16InvalidCastCodeCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue16_invalid_cast_code" `
        -Source @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let value: bool = true
    let bad: string@View = value as string@View
    let _ = bad
    return 0
}
"@ `
        -ConformanceFileName "issue16_invalid_cast_code.log"

    if ($result.ExitCode -ne 1) {
        throw "Case 'issue16_invalid_cast_code' expected exit 1 but got $($result.ExitCode)."
    }

    $codeCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.code -eq "E-SEM-2528"
    }).Count
    if ($codeCount -lt 1) {
        throw "Case 'issue16_invalid_cast_code' expected diagnostic code 'E-SEM-2528'."
    }

    $legacyRuleIdCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.code -eq "T-Cast-Invalid"
    }).Count
    if ($legacyRuleIdCount -ne 0) {
        throw "Case 'issue16_invalid_cast_code' must not emit raw rule id 'T-Cast-Invalid' as a diagnostic code."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $castInvalidTraceCount = @($logLines | Where-Object { $_ -like "*`tT-Cast-Invalid`t*" }).Count
    if ($castInvalidTraceCount -lt 1) {
        throw "Case 'issue16_invalid_cast_code' expected T-Cast-Invalid in conformance trace."
    }

    Write-Host "[compiler-static] issue16_invalid_cast_code: exit=$($result.ExitCode) code=$codeCount trace=$castInvalidTraceCount"
}

function Invoke-Issue16NoLegacyDiagnosticApiCase {
    $srcRoot = Join-Path $workspaceRoot "cursive\\src"
    $includeRoot = Join-Path $workspaceRoot "cursive\\include"

    $legacyMakeDiagnosticHits = @(Find-PatternHits `
        -Roots @($srcRoot, $includeRoot) `
        -Pattern '\bMakeDiagnostic\s*\(' `
        -AllowedRelativePaths @(
            "cursive\\src\\00_core\\diagnostic_messages.cpp",
            "cursive\\include\\00_core\\diagnostic_messages.h"
        ))

    if ($legacyMakeDiagnosticHits.Count -gt 0) {
        $sample = $legacyMakeDiagnosticHits | Select-Object -First 10
        foreach ($hit in $sample) {
            Write-Host "[compiler-static] issue16_no_legacy_diagnostic_api hit: $($hit.File):$($hit.Line) $($hit.Text)"
        }
        throw "Case 'issue16_no_legacy_diagnostic_api' found non-core MakeDiagnostic(...) usage; use MakeDiagnosticById(...) instead."
    }

    $rawCodeAssignmentHits = @(Find-PatternHits `
        -Roots @($srcRoot, $includeRoot) `
        -Pattern '\.code\s*=(?!=)' `
        -AllowedRelativePaths @(
            "cursive\\src\\00_core\\diagnostic_messages.cpp",
            "cursive\\src\\04_analysis\\caps\\callgraph_caps.cpp"
        ))
    if ($rawCodeAssignmentHits.Count -gt 0) {
        $sample = $rawCodeAssignmentHits | Select-Object -First 10
        foreach ($hit in $sample) {
            Write-Host "[compiler-static] issue16_no_legacy_diagnostic_api code-assignment hit: $($hit.File):$($hit.Line) $($hit.Text)"
        }
        throw "Case 'issue16_no_legacy_diagnostic_api' found raw '.code =' assignments outside approved locations."
    }

    $removedLegacyMappingPath = Join-Path $workspaceRoot "cursive\\tools\\diagnostic_spec_mapping.json"
    if (Test-Path $removedLegacyMappingPath) {
        throw "Case 'issue16_no_legacy_diagnostic_api' found removed legacy mapping file: $removedLegacyMappingPath"
    }

    Write-Host "[compiler-static] issue16_no_legacy_diagnostic_api: make_diagnostic_hits=0 code_assignment_hits=0 legacy_mapping_present=0"
}

function Invoke-Issue16EmittedCodesCoveredBySpecCase {
    $caseRoot = Join-Path $workRoot "issue16_emitted_codes_covered_by_spec"
    New-Item -ItemType Directory -Path $caseRoot -Force | Out-Null

    [System.IO.File]::WriteAllLines((Join-Path $caseRoot "Cursive.toml"), $manifestLines)
    [System.IO.File]::WriteAllText((Join-Path $caseRoot "Main.cursive"), (New-BulkErrorSource 48))

    $diagJsonPath = Join-Path $caseRoot "diag.json"
    $stderrPath = Join-Path $caseRoot "stderr.txt"

    Push-Location $caseRoot
    try {
        & $CompilerPath --incremental off --check --diag-json --quiet --max-errors inf Main.cursive 1> $diagJsonPath 2> $stderrPath
        $exitCode = $LASTEXITCODE
    } finally {
        Pop-Location
    }

    if ($exitCode -ne 1) {
        throw "Case 'issue16_emitted_codes_covered_by_spec' expected exit 1 but got $exitCode."
    }

    $diagJson = (Get-Content -Path $diagJsonPath -Raw) | ConvertFrom-Json
    $observedCodes = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
    foreach ($diag in @($diagJson.diagnostics)) {
        $code = [string]$diag.code
        if (-not [string]::IsNullOrWhiteSpace($code)) {
            [void]$observedCodes.Add($code)
        }
    }

    $specPath = $canonicalSpecPath
    if (-not (Test-Path $specPath)) {
        throw "Case 'issue16_emitted_codes_covered_by_spec' missing spec file: $specPath"
    }
    $specText = Get-Content -Path $specPath -Raw
    $codePattern = [regex]'[EWIP]-[A-Z]{3}-[0-9]{4}'
    $specCodes = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
    foreach ($m in $codePattern.Matches($specText)) {
        [void]$specCodes.Add($m.Value)
    }

    $missing = New-Object System.Collections.Generic.List[string]
    foreach ($code in $observedCodes) {
        if (-not $specCodes.Contains($code)) {
            $missing.Add($code)
        }
    }
    if ($missing.Count -gt 0) {
        $ordered = @($missing | Sort-Object)
        throw "Case 'issue16_emitted_codes_covered_by_spec' found emitted codes missing from canonical language spec: $($ordered -join ', ')"
    }

    Write-Host "[compiler-static] issue16_emitted_codes_covered_by_spec: observed_codes=$($observedCodes.Count) spec_codes=$($specCodes.Count)"
}

function Invoke-ParseAbortCase {
    $caseRoot = Join-Path $workRoot "phase_abort_parse_cap_1"
    New-Item -ItemType Directory -Path $caseRoot -Force | Out-Null
    [System.IO.File]::WriteAllLines((Join-Path $caseRoot "Cursive.toml"), $manifestLines)
    [System.IO.File]::WriteAllText((Join-Path $caseRoot "Main.cursive"), (New-ParseErrorSource 40))
    $diagJsonPath = Join-Path $caseRoot "diag.json"
    $stderrPath = Join-Path $caseRoot "stderr.txt"
    Push-Location $caseRoot
    try {
        & $CompilerPath --incremental off --check --diag-json --quiet --max-errors 1 Main.cursive 1> $diagJsonPath 2> $stderrPath
        $exitCode = $LASTEXITCODE
    } finally {
        Pop-Location
    }
    if ($exitCode -ne 1) {
        throw "Case 'phase_abort_parse_cap_1' expected exit 1 but got $exitCode."
    }
    $diagJson = (Get-Content -Path $diagJsonPath -Raw) | ConvertFrom-Json
    $errorCount = @($diagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 1) {
        throw "Case 'phase_abort_parse_cap_1' expected 1 error, observed $errorCount."
    }
    Write-Host "[compiler-static] phase_abort_parse_cap_1: exit=$exitCode errors=$errorCount"
}

function Invoke-ResolveAbortCase {
    $caseRoot = Join-Path $workRoot "phase_abort_resolve_cap_1"
    New-Item -ItemType Directory -Path $caseRoot -Force | Out-Null
    [System.IO.File]::WriteAllLines((Join-Path $caseRoot "Cursive.toml"), $manifestLines)
    [System.IO.File]::WriteAllText((Join-Path $caseRoot "Main.cursive"), (New-ResolveErrorSource 40))
    $diagJsonPath = Join-Path $caseRoot "diag.json"
    $stderrPath = Join-Path $caseRoot "stderr.txt"
    Push-Location $caseRoot
    try {
        & $CompilerPath --incremental off --check --diag-json --quiet --max-errors 1 Main.cursive 1> $diagJsonPath 2> $stderrPath
        $exitCode = $LASTEXITCODE
    } finally {
        Pop-Location
    }
    if ($exitCode -ne 1) {
        throw "Case 'phase_abort_resolve_cap_1' expected exit 1 but got $exitCode."
    }
    $diagJson = (Get-Content -Path $diagJsonPath -Raw) | ConvertFrom-Json
    $errorCount = @($diagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 1) {
        throw "Case 'phase_abort_resolve_cap_1' expected 1 error, observed $errorCount."
    }
    Write-Host "[compiler-static] phase_abort_resolve_cap_1: exit=$exitCode errors=$errorCount"
}

function Invoke-Issue541ResolveModulesParseErrorConformanceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue541_resolve_modules_parse_error_conformance" `
        -Source (New-ParseErrorSource 1) `
        -ConformanceFileName "issue541_resolve_modules_parse_error_conformance.log"

    if ($result.ExitCode -ne 1) {
        throw "Case 'issue541_resolve_modules_parse_error_conformance' expected exit 1 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -lt 1) {
        throw "Case 'issue541_resolve_modules_parse_error_conformance' expected at least one compile-time error."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $resolveModulesErrParseCount = @($logLines | Where-Object { $_ -like "*`tResolveModules-Err-Parse`t*" }).Count
    $resolveModulesErrResolveCount = @($logLines | Where-Object { $_ -like "*`tResolveModules-Err-Resolve`t*" }).Count
    $resolveModulesOkCount = @($logLines | Where-Object { $_ -like "*`tResolveModules-Ok`t*" }).Count

    if ($resolveModulesErrParseCount -lt 1) {
        throw "Case 'issue541_resolve_modules_parse_error_conformance' expected ResolveModules-Err-Parse in the conformance trace."
    }
    if ($resolveModulesErrResolveCount -ne 0) {
        throw "Case 'issue541_resolve_modules_parse_error_conformance' unexpectedly emitted ResolveModules-Err-Resolve for a parse failure."
    }
    if ($resolveModulesOkCount -ne 0) {
        throw "Case 'issue541_resolve_modules_parse_error_conformance' unexpectedly emitted ResolveModules-Ok for a parse failure."
    }

    $resolverHeaderPath = Join-Path $workspaceRoot "cursive\\include\\04_analysis\\resolve\\resolver.h"
    if (-not (Test-Path $resolverHeaderPath)) {
        throw "Case 'issue541_resolve_modules_parse_error_conformance' missing resolver header: $resolverHeaderPath"
    }
    $resolverHeaderText = Get-Content -Path $resolverHeaderPath -Raw
    if ($resolverHeaderText -notmatch 'bool parse_ok = true;' -or $resolverHeaderText -notmatch 'parse_diags') {
        throw "Case 'issue541_resolve_modules_parse_error_conformance' expected ResolveContext to carry parse status and parse diagnostics."
    }

    $resolveModulePath = Join-Path $workspaceRoot "cursive\\src\\04_analysis\\resolve\\resolve_module.cpp"
    if (-not (Test-Path $resolveModulePath)) {
        throw "Case 'issue541_resolve_modules_parse_error_conformance' missing resolver implementation: $resolveModulePath"
    }
    $resolveModuleText = Get-Content -Path $resolveModulePath -Raw
    if ($resolveModuleText -notmatch 'ctx\.parse_ok' -or $resolveModuleText -notmatch 'ctx\.parse_diags' -or $resolveModuleText -notmatch 'SPEC_RULE\("ResolveModules-Err-Parse"\)') {
        throw "Case 'issue541_resolve_modules_parse_error_conformance' expected resolve_module.cpp to short-circuit parse failures inside ResolveModules."
    }

    $driverPath = Join-Path $workspaceRoot "cursive\\src\\06_driver\\main.cpp"
    if (-not (Test-Path $driverPath)) {
        throw "Case 'issue541_resolve_modules_parse_error_conformance' missing driver implementation: $driverPath"
    }
    $driverText = Get-Content -Path $driverPath -Raw
    if ($driverText -notmatch 'parse_phase_diags' -or $driverText -notmatch 'parse_fail_res_ctx\.parse_diags = &parse_phase_diags;' -or $driverText -notmatch 'res_ctx\.parse_diags = &parse_phase_diags;') {
        throw "Case 'issue541_resolve_modules_parse_error_conformance' expected main.cpp to route parse diagnostics through ResolveModules."
    }

    Write-Host "[compiler-static] issue541_resolve_modules_parse_error_conformance: exit=$($result.ExitCode) errors=$errorCount resolve_modules_err_parse=$resolveModulesErrParseCount resolver_api=1 driver_wiring=1"
}

function Invoke-MainMissingStaticUndefinedCase {
    $caseRoot = Join-Path $workRoot "main_missing_static_undefined_mapping"
    New-Item -ItemType Directory -Path $caseRoot -Force | Out-Null
    [System.IO.File]::WriteAllLines((Join-Path $caseRoot "Cursive.toml"), $manifestLines)
    [System.IO.File]::WriteAllText((Join-Path $caseRoot "Main.cursive"), (New-MainMissingSource))
    $diagJsonPath = Join-Path $caseRoot "diag.json"
    $stderrPath = Join-Path $caseRoot "stderr.txt"
    Push-Location $caseRoot
    try {
        & $CompilerPath --incremental off --check --diag-json --quiet Main.cursive 1> $diagJsonPath 2> $stderrPath
        $exitCode = $LASTEXITCODE
    } finally {
        Pop-Location
    }
    if ($exitCode -ne 1) {
        throw "Case 'main_missing_static_undefined_mapping' expected exit 1 but got $exitCode."
    }
    $diagJson = (Get-Content -Path $diagJsonPath -Raw) | ConvertFrom-Json
    $errorCount = @($diagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -lt 1) {
        throw "Case 'main_missing_static_undefined_mapping' expected at least one error."
    }
    $mainMissingCodeCount = @($diagJson.diagnostics | Where-Object {
        $_.code -eq "E-MOD-2434"
    }).Count
    if ($mainMissingCodeCount -lt 1) {
        throw "Case 'main_missing_static_undefined_mapping' expected E-MOD-2434 from Main-Missing mapping."
    }
    Write-Host "[compiler-static] main_missing_static_undefined_mapping: exit=$exitCode errors=$errorCount mapped_main_missing=$mainMissingCodeCount"
}

function Write-CaseExtraFiles {
    param(
        [string]$CaseRoot,
        [hashtable]$ExtraFiles
    )

    foreach ($entry in $ExtraFiles.GetEnumerator()) {
        $relativePath = [string]$entry.Key
        if ([string]::IsNullOrWhiteSpace($relativePath)) {
            throw "Case file path must be non-empty."
        }
        if ([System.IO.Path]::IsPathRooted($relativePath)) {
            throw "Case file path must be relative: $relativePath"
        }

        $normalized = $relativePath.Replace('\', '/')
        if ($normalized -eq ".." -or $normalized.StartsWith("../") -or $normalized.Contains("/../")) {
            throw "Case file path must not escape case root: $relativePath"
        }

        $fullPath = Join-Path $CaseRoot $relativePath
        $dirPath = Split-Path -Parent $fullPath
        if (-not [string]::IsNullOrWhiteSpace($dirPath)) {
            New-Item -ItemType Directory -Path $dirPath -Force | Out-Null
        }
        [System.IO.File]::WriteAllText($fullPath, [string]$entry.Value)
    }
}

function Invoke-CheckWithConformance {
    param(
        [string]$CaseId,
        [string]$Source,
        [string]$ConformanceFileName,
        [string[]]$Manifest = $manifestLines,
        [string[]]$ExtraArgs = @(),
        [switch]$ForceReadBytesFail,
        [switch]$AllowMissingConformance,
        [hashtable]$ExtraFiles = @{}
    )

    $caseRoot = Join-Path $workRoot $CaseId
    New-Item -ItemType Directory -Path $caseRoot -Force | Out-Null
    [System.IO.File]::WriteAllLines((Join-Path $caseRoot "Cursive.toml"), $Manifest)
    [System.IO.File]::WriteAllText((Join-Path $caseRoot "Main.cursive"), $Source)
    Write-CaseExtraFiles -CaseRoot $caseRoot -ExtraFiles $ExtraFiles

    $diagJsonPath = Join-Path $caseRoot "diag.json"
    $stderrPath = Join-Path $caseRoot "stderr.txt"
    $savedErrorActionPreference = $ErrorActionPreference
    Push-Location $caseRoot
    try {
        $ErrorActionPreference = "Continue"
        if ($ForceReadBytesFail.IsPresent) {
            $env:CURSIVE_TEST_READ_BYTES_FAIL = "1"
        }
        $args = @("--incremental", "off", "--check", "--diag-json", "--quiet", "--conformance", $ConformanceFileName)
        $args += $ExtraArgs
        $args += @("Main.cursive")
        & $CompilerPath @args 1> $diagJsonPath 2> $stderrPath
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $savedErrorActionPreference
        if ($ForceReadBytesFail.IsPresent) {
            Remove-Item Env:CURSIVE_TEST_READ_BYTES_FAIL -ErrorAction SilentlyContinue
        }
        Pop-Location
    }

    if (-not (Test-Path $diagJsonPath)) {
        throw "Case '$CaseId' did not produce diagnostic JSON output."
    }
    $jsonText = Get-Content -Path $diagJsonPath -Raw
    if ([string]::IsNullOrWhiteSpace($jsonText)) {
        throw "Case '$CaseId' produced empty diagnostic JSON output."
    }
    $diagJson = $jsonText | ConvertFrom-Json
    if ($null -eq $diagJson.diagnostics) {
        throw "Case '$CaseId' diagnostic JSON missing 'diagnostics' array."
    }

    $conformancePath = Resolve-ConformancePath `
        -CaseRoot $caseRoot `
        -ConformanceFileName $ConformanceFileName
    if (-not (Test-Path $conformancePath) -and -not $AllowMissingConformance.IsPresent) {
        throw "Case '$CaseId' missing conformance trace: $conformancePath"
    }

    return [PSCustomObject]@{
        CaseId = $CaseId
        CaseRoot = $caseRoot
        ExitCode = $exitCode
        DiagJson = $diagJson
        DiagJsonPath = $diagJsonPath
        StderrPath = $stderrPath
        ConformancePath = $conformancePath
    }
}

function Invoke-StdoutModeWithConformance {
    param(
        [string]$CaseId,
        [string]$Source,
        [string]$ConformanceFileName,
        [string[]]$Manifest = $manifestLines,
        [string[]]$ExtraArgs = @(),
        [switch]$AllowMissingConformance,
        [hashtable]$ExtraFiles = @{}
    )

    $caseRoot = Join-Path $workRoot $CaseId
    New-Item -ItemType Directory -Path $caseRoot -Force | Out-Null
    [System.IO.File]::WriteAllLines((Join-Path $caseRoot "Cursive.toml"), $Manifest)
    [System.IO.File]::WriteAllText((Join-Path $caseRoot "Main.cursive"), $Source)
    Write-CaseExtraFiles -CaseRoot $caseRoot -ExtraFiles $ExtraFiles

    $stdoutPath = Join-Path $caseRoot "stdout.txt"
    $stderrPath = Join-Path $caseRoot "stderr.txt"
    $savedErrorActionPreference = $ErrorActionPreference
    Push-Location $caseRoot
    try {
        $ErrorActionPreference = "Continue"
        $args = @("--incremental", "off", "--check", "--quiet", "--conformance", $ConformanceFileName)
        $args += $ExtraArgs
        $args += @("Main.cursive")
        & $CompilerPath @args 1> $stdoutPath 2> $stderrPath
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $savedErrorActionPreference
        Pop-Location
    }

    if (-not (Test-Path $stdoutPath)) {
        throw "Case '$CaseId' did not produce stdout output."
    }
    $stdoutText = Get-Content -Path $stdoutPath -Raw
    if ([string]::IsNullOrWhiteSpace($stdoutText)) {
        throw "Case '$CaseId' produced empty stdout output."
    }

    $conformancePath = Resolve-ConformancePath `
        -CaseRoot $caseRoot `
        -ConformanceFileName $ConformanceFileName
    if (-not (Test-Path $conformancePath) -and -not $AllowMissingConformance.IsPresent) {
        throw "Case '$CaseId' missing conformance trace: $conformancePath"
    }

    return [PSCustomObject]@{
        CaseId = $CaseId
        CaseRoot = $caseRoot
        ExitCode = $exitCode
        StdoutPath = $stdoutPath
        StdoutText = $stdoutText
        StderrPath = $stderrPath
        ConformancePath = $conformancePath
    }
}

function Invoke-BuildWithConformance {
    param(
        [string]$CaseId,
        [string]$Source,
        [string]$ConformanceFileName,
        [string[]]$Manifest = $manifestLines,
        [string[]]$ExtraArgs = @(),
        [hashtable]$ExtraFiles = @{},
        [hashtable]$EnvOverrides = @{},
        [string]$CaseRootOverride = "",
        [string]$CompilerPathOverride = "",
        [switch]$AllowMissingConformance
    )

    $caseRoot = $CaseRootOverride
    if ([string]::IsNullOrWhiteSpace($caseRoot)) {
        $caseRoot = Join-Path $workRoot $CaseId
    }
    New-Item -ItemType Directory -Path $caseRoot -Force | Out-Null
    [System.IO.File]::WriteAllLines((Join-Path $caseRoot "Cursive.toml"), $Manifest)
    [System.IO.File]::WriteAllText((Join-Path $caseRoot "Main.cursive"), $Source)
    Write-CaseExtraFiles -CaseRoot $caseRoot -ExtraFiles $ExtraFiles

    $diagJsonPath = Join-Path $caseRoot "diag.json"
    $stderrPath = Join-Path $caseRoot "stderr.txt"
    $savedEnv = @{}
    $savedErrorActionPreference = $ErrorActionPreference
    $effectiveCompilerPath = $CompilerPath
    if (-not [string]::IsNullOrWhiteSpace($CompilerPathOverride)) {
        $effectiveCompilerPath = $CompilerPathOverride
    }

    Push-Location $caseRoot
    try {
        $ErrorActionPreference = "Continue"
        foreach ($name in $EnvOverrides.Keys) {
            $envName = [string]$name
            $existing = Get-Item -Path ("Env:" + $envName) -ErrorAction SilentlyContinue
            if ($null -ne $existing) {
                $savedEnv[$envName] = $existing.Value
            } else {
                $savedEnv[$envName] = $null
            }
            Set-Item -Path ("Env:" + $envName) -Value ([string]$EnvOverrides[$name])
        }

        $args = @("--incremental", "off", "--diag-json", "--quiet", "--conformance", $ConformanceFileName)
        $args += $ExtraArgs
        $args += @("Main.cursive")
        & $effectiveCompilerPath build @args 1> $diagJsonPath 2> $stderrPath
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $savedErrorActionPreference
        foreach ($name in $EnvOverrides.Keys) {
            $envName = [string]$name
            if ($savedEnv.ContainsKey($envName) -and $null -ne $savedEnv[$envName]) {
                Set-Item -Path ("Env:" + $envName) -Value ([string]$savedEnv[$envName])
            } else {
                Remove-Item -Path ("Env:" + $envName) -ErrorAction SilentlyContinue
            }
        }
        Pop-Location
    }

    if (-not (Test-Path $diagJsonPath)) {
        throw "Case '$CaseId' did not produce diagnostic JSON output."
    }
    $jsonText = Get-Content -Path $diagJsonPath -Raw
    if ([string]::IsNullOrWhiteSpace($jsonText)) {
        throw "Case '$CaseId' produced empty diagnostic JSON output."
    }
    $diagJson = $jsonText | ConvertFrom-Json
    if ($null -eq $diagJson.diagnostics) {
        throw "Case '$CaseId' diagnostic JSON missing 'diagnostics' array."
    }

    $conformancePath = Resolve-ConformancePath `
        -CaseRoot $caseRoot `
        -ConformanceFileName $ConformanceFileName
    if (-not (Test-Path $conformancePath) -and -not $AllowMissingConformance.IsPresent) {
        throw "Case '$CaseId' missing conformance trace: $conformancePath"
    }

    return [PSCustomObject]@{
        CaseId = $CaseId
        CaseRoot = $caseRoot
        ExitCode = $exitCode
        DiagJson = $diagJson
        DiagJsonPath = $diagJsonPath
        StderrPath = $stderrPath
        ConformancePath = $conformancePath
    }
}

function Resolve-ConformancePath {
    param(
        [string]$CaseRoot,
        [string]$ConformanceFileName
    )

    $directPath = Join-Path $CaseRoot ("build\\probe\\logs\\conformance\\" + $ConformanceFileName)
    if (Test-Path $directPath) {
        return $directPath
    }

    $buildRoot = Join-Path $CaseRoot "build"
    if (Test-Path $buildRoot) {
        $matches = @(Get-ChildItem `
            -Path $buildRoot `
            -Recurse `
            -File `
            -Filter $ConformanceFileName `
            -ErrorAction SilentlyContinue | Where-Object {
                $_.FullName -like "*\\logs\\conformance\\$ConformanceFileName"
            })
        if ($matches.Count -gt 0) {
            return $matches[0].FullName
        }
    }

    return $directPath
}

function Get-EmittedLlvmIrText {
    param(
        [string]$CaseId,
        [string]$CaseRoot
    )

    $llFile = Get-ChildItem -Path (Join-Path $CaseRoot "build\\probe") -Recurse -File -Filter *.ll | Select-Object -First 1
    if ($null -eq $llFile) {
        throw "Case '$CaseId' missing emitted LLVM IR output."
    }

    return Get-Content -Path $llFile.FullName -Raw
}

function Get-DiagnosticSignatureLines {
    param(
        [object[]]$Diagnostics
    )

    $lines = New-Object System.Collections.Generic.List[string]
    foreach ($diag in @($Diagnostics)) {
        $spanFile = "-"
        $startLine = 0
        $startCol = 0
        if ($null -ne $diag.span) {
            $spanFile = [System.IO.Path]::GetFileName([string]$diag.span.file)
            $startLine = [int]$diag.span.start_line
            $startCol = [int]$diag.span.start_col
        }
        $lines.Add("$($diag.code)|$($diag.severity)|$spanFile|$startLine|$startCol|$($diag.message)")
    }
    return $lines
}

function Invoke-Issue16TokenSpanTraceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue16_token_spans_trace" `
        -Source (New-StaticCheckSource) `
        -ConformanceFileName "issue16_token_spans.log"

    if ($result.ExitCode -ne 1) {
        throw "Case 'issue16_token_spans_trace' expected exit 1 but got $($result.ExitCode)."
    }
    $logLines = Get-Content -Path $result.ConformancePath
    $attachTokensCount = @($logLines | Where-Object { $_ -like "*`tAttach-Tokens-Ok`t*" }).Count
    $attachTokenCount = @($logLines | Where-Object { $_ -like "*`tAttach-Token-Ok`t*" }).Count
    $noUnknownCount = @($logLines | Where-Object { $_ -like "*`tNo-Unknown-Ok`t*" }).Count

    if ($attachTokensCount -lt 1 -or $attachTokenCount -lt 1 -or $noUnknownCount -lt 1) {
        throw "Case 'issue16_token_spans_trace' expected Attach-Tokens-Ok, Attach-Token-Ok, and No-Unknown-Ok in conformance trace."
    }

    Write-Host "[compiler-static] issue16_token_spans_trace: exit=$($result.ExitCode) attach_tokens=$attachTokensCount attach_token=$attachTokenCount no_unknown=$noUnknownCount"
}

function Invoke-Issue16CodeSelectionTraceCase {
    $resultC0 = Invoke-CheckWithConformance `
        -CaseId "issue16_code_selection_trace_c0" `
        -Source (New-StaticCheckSource) `
        -ConformanceFileName "issue16_code_selection_c0.log"

    if ($resultC0.ExitCode -ne 1) {
        throw "Case 'issue16_code_selection_trace_c0' expected exit 1 but got $($resultC0.ExitCode)."
    }

    $c0DiagCodeCount = @($resultC0.DiagJson.diagnostics | Where-Object { $_.code -eq "E-MOD-2402" }).Count
    if ($c0DiagCodeCount -lt 1) {
        throw "Case 'issue16_code_selection_trace_c0' expected diagnostic code 'E-MOD-2402'."
    }
    $c0LogLines = Get-Content -Path $resultC0.ConformancePath
    $codeC0Count = @($c0LogLines | Where-Object { $_ -like "*`tCode`t*" }).Count
    if ($codeC0Count -lt 1) {
        throw "Case 'issue16_code_selection_trace_c0' expected Code trace emission."
    }

    $resultSpec = Invoke-CheckWithConformance `
        -CaseId "issue16_code_selection_trace_spec" `
        -Source (New-SystemCtorSafeSource) `
        -ConformanceFileName "issue16_code_selection_spec.log"

    if ($resultSpec.ExitCode -ne 1) {
        throw "Case 'issue16_code_selection_trace_spec' expected exit 1 but got $($resultSpec.ExitCode)."
    }

    $specDiagCodeCount = @($resultSpec.DiagJson.diagnostics | Where-Object { $_.code -eq "E-CON-0020" }).Count
    if ($specDiagCodeCount -lt 1) {
        throw "Case 'issue16_code_selection_trace_spec' expected diagnostic code 'E-CON-0020'."
    }
    $specLogLines = Get-Content -Path $resultSpec.ConformancePath
    $codeSpecCount = @($specLogLines | Where-Object { $_ -like "*`tCode`t*" }).Count
    if ($codeSpecCount -lt 1) {
        throw "Case 'issue16_code_selection_trace_spec' expected Code trace emission."
    }

    Write-Host "[compiler-static] issue16_code_selection_trace: c0_exit=$($resultC0.ExitCode) c0_code=$c0DiagCodeCount c0_trace=$codeC0Count spec_exit=$($resultSpec.ExitCode) spec_code=$specDiagCodeCount spec_trace=$codeSpecCount"
}

function Invoke-Issue16ExternalNoSpanCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue16_external_no_span" `
        -Source @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@ `
        -ConformanceFileName "issue16_external_no_span.log" `
        -ForceReadBytesFail

    if ($result.ExitCode -ne 1) {
        throw "Case 'issue16_external_no_span' expected exit 1 but got $($result.ExitCode)."
    }

    $diagCodeCount = @($result.DiagJson.diagnostics | Where-Object { $_.code -eq "E-SRC-0102" }).Count
    if ($diagCodeCount -lt 1) {
        throw "Case 'issue16_external_no_span' expected E-SRC-0102 diagnostics."
    }

    $nonNullSpanCount = @($result.DiagJson.diagnostics | Where-Object { $null -ne $_.span }).Count
    if ($nonNullSpanCount -ne 0) {
        throw "Case 'issue16_external_no_span' expected all diagnostics to have null spans."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $readBytesErrCount = @($logLines | Where-Object { $_ -like "*`tReadBytes-Err`t*" }).Count
    $noSpanExternalCount = @($logLines | Where-Object { $_ -like "*`tNoSpan-External`t*" }).Count
    $diagEmitNoSpanCount = @($logLines | Where-Object { $_ -like "*`tDiag-Emit`t-`t-`t-`t-`t-`t*" }).Count

    if ($readBytesErrCount -lt 1 -or $noSpanExternalCount -lt 1 -or $diagEmitNoSpanCount -lt 1) {
        throw "Case 'issue16_external_no_span' expected ReadBytes-Err, NoSpan-External, and no-span Diag-Emit rows."
    }

    Write-Host "[compiler-static] issue16_external_no_span: exit=$($result.ExitCode) read_bytes_err=$readBytesErrCount no_span_external=$noSpanExternalCount diag_emit_no_span=$diagEmitNoSpanCount"
}

function Invoke-Issue16InternalSpanRetentionCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue16_internal_span_retention" `
        -Source (New-StaticCheckSource) `
        -ConformanceFileName "issue16_internal_span_retention.log"

    if ($result.ExitCode -ne 1) {
        throw "Case 'issue16_internal_span_retention' expected exit 1 but got $($result.ExitCode)."
    }

    $withSpanCount = @($result.DiagJson.diagnostics | Where-Object { $null -ne $_.span }).Count
    if ($withSpanCount -lt 1) {
        throw "Case 'issue16_internal_span_retention' expected at least one internal diagnostic with span."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $diagEmitWithSpanCount = @($logLines | Where-Object {
        $_ -like "*`tDiag-Emit`t*" -and $_ -notlike "*`tDiag-Emit`t-`t-`t-`t-`t-`t*"
    }).Count
    if ($diagEmitWithSpanCount -lt 1) {
        throw "Case 'issue16_internal_span_retention' expected at least one Diag-Emit row with source span fields."
    }

    Write-Host "[compiler-static] issue16_internal_span_retention: exit=$($result.ExitCode) json_with_span=$withSpanCount trace_with_span=$diagEmitWithSpanCount"
}

function Invoke-Issue16OrderingIdentityCase {
    $run1 = Invoke-CheckWithConformance `
        -CaseId "issue16_order_identity_run1" `
        -Source (New-OrderedDiagnosticsSource) `
        -ConformanceFileName "issue16_order_identity_run1.log" `
        -ExtraArgs @("--max-errors", "inf")
    $run2 = Invoke-CheckWithConformance `
        -CaseId "issue16_order_identity_run2" `
        -Source (New-OrderedDiagnosticsSource) `
        -ConformanceFileName "issue16_order_identity_run2.log" `
        -ExtraArgs @("--max-errors", "inf")

    if ($run1.ExitCode -ne 1 -or $run2.ExitCode -ne 1) {
        throw "Case 'issue16_order_identity' expected both runs to exit with 1; observed run1=$($run1.ExitCode), run2=$($run2.ExitCode)."
    }

    $errors1 = @($run1.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    })
    $errors2 = @($run2.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    })
    if ($errors1.Count -lt 2 -or $errors2.Count -lt 2) {
        throw "Case 'issue16_order_identity' expected at least two errors in each run."
    }

    $sig1 = Get-DiagnosticSignatureLines -Diagnostics $errors1
    $sig2 = Get-DiagnosticSignatureLines -Diagnostics $errors2
    $joined1 = [string]::Join("`n", $sig1)
    $joined2 = [string]::Join("`n", $sig2)
    if ($joined1 -ne $joined2) {
        throw "Case 'issue16_order_identity' expected identical ordered diagnostic streams across runs."
    }

    Write-Host "[compiler-static] issue16_order_identity: run1_errors=$($errors1.Count) run2_errors=$($errors2.Count) stable_order=1"
}

function Invoke-Issue26ManifestLlvmBinMissingNoFallbackCase {
    $manifest = @(
        "[toolchain]",
        "llvm_bin = ""C:/definitely/not/here""",
        "target_profile = ""x86_64-win64""",
        "",
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""executable""",
        "root = "".""",
        "out_dir = ""build/probe"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue26_manifest_llvm_bin_missing_no_fallback" `
        -Source (New-MinimalMainSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue26_manifest_llvm_bin_missing.log"

    if ($result.ExitCode -ne 1) {
        throw "Case 'issue26_manifest_llvm_bin_missing_no_fallback' expected exit 1 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -lt 1) {
        throw "Case 'issue26_manifest_llvm_bin_missing_no_fallback' expected at least one diagnostic."
    }
    $linkNotFoundDiag = @($result.DiagJson.diagnostics | Where-Object {
        $_.code -eq "E-OUT-0405"
    }).Count
    if ($linkNotFoundDiag -lt 1) {
        throw "Case 'issue26_manifest_llvm_bin_missing_no_fallback' expected E-OUT-0405."
    }

    if (Test-Path (Join-Path $result.CaseRoot "build\\probe\\bin\\probe.exe")) {
        throw "Case 'issue26_manifest_llvm_bin_missing_no_fallback' must not produce build\\probe\\bin\\probe.exe."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $resolveErr = @($logLines | Where-Object { $_ -like "*`tResolveTool-Err-Linker`t*" }).Count
    $resolveOk = @($logLines | Where-Object { $_ -like "*`tResolveTool-Ok`t*" }).Count
    $linkNotFound = @($logLines | Where-Object { $_ -like "*`tLink-NotFound`t*" }).Count
    $linkOk = @($logLines | Where-Object { $_ -like "*`tOut-Final-Link-Ok`t*" }).Count
    if ($resolveErr -lt 1 -or $resolveOk -ne 0 -or $linkNotFound -lt 1 -or $linkOk -ne 0) {
        throw "Case 'issue26_manifest_llvm_bin_missing_no_fallback' expected ResolveTool-Err-Linker and Link-NotFound with no successful link trace."
    }

    Write-Host "[compiler-static] issue26_manifest_llvm_bin_missing_no_fallback: exit=$($result.ExitCode) errors=$errorCount resolve_err=$resolveErr resolve_ok=$resolveOk link_not_found=$linkNotFound"
}

function Invoke-Issue26NoSiblingExternFallbackCase {
    $caseRoot = Join-Path $workRoot "issue26_no_sibling_extern_fallback"

    $systemRoot = $env:SystemRoot
    if ([string]::IsNullOrWhiteSpace($systemRoot)) {
        $systemRoot = "C:\\Windows"
    }
    $pathValue = Join-Path $systemRoot "System32"

    $result = Invoke-BuildWithConformance `
        -CaseId "issue26_no_sibling_extern_fallback" `
        -CaseRootOverride $caseRoot `
        -Source (New-MinimalMainSource) `
        -ConformanceFileName "issue26_no_sibling_extern_fallback.log" `
        -EnvOverrides @{ PATH = $pathValue }

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue26_no_sibling_extern_fallback' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue26_no_sibling_extern_fallback' expected zero diagnostics but observed $errorCount."
    }

    if (-not (Test-Path (Join-Path $result.CaseRoot "build\\probe\\bin\\probe.exe"))) {
        throw "Case 'issue26_no_sibling_extern_fallback' expected compiler sidecar tool resolution to produce build\\probe\\bin\\probe.exe."
    }
    $repoLlvmDir = Join-Path $result.CaseRoot "build\\probe\\llvm"
    if (Test-Path $repoLlvmDir) {
        throw "Case 'issue26_no_sibling_extern_fallback' must not materialize a repo LLVM staging directory: $repoLlvmDir"
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $resolveErr = @($logLines | Where-Object { $_ -like "*`tResolveTool-Err-Linker`t*" }).Count
    $resolveOk = @($logLines | Where-Object { $_ -like "*`tResolveTool-Ok`t*" }).Count
    $linkOk = @($logLines | Where-Object { $_ -like "*`tOut-Final-Link-Ok`t*" }).Count
    if ($resolveErr -ne 0 -or $resolveOk -lt 1 -or $linkOk -lt 1) {
        throw "Case 'issue26_no_sibling_extern_fallback' expected ResolveTool-Ok through compiler sidecar tools with no ResolveTool-Err-Linker trace."
    }

    Write-Host "[compiler-static] issue26_no_sibling_extern_fallback: exit=$($result.ExitCode) errors=$errorCount resolve_err=$resolveErr resolve_ok=$resolveOk link_ok=$linkOk"
}

function Invoke-Issue26RepoLlvmNoPathFallbackForLlvmAsCase {
    $caseRoot = Join-Path $workRoot "issue26_repo_llvm_no_path_fallback_for_llvm_as"
    $systemRoot = $env:SystemRoot
    if ([string]::IsNullOrWhiteSpace($systemRoot)) {
        $systemRoot = "C:\\Windows"
    }
    $pathValue = Join-Path $systemRoot "System32"

    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""bc"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue26_repo_llvm_no_path_fallback_for_llvm_as" `
        -CaseRootOverride $caseRoot `
        -Source (New-MinimalMainSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue26_repo_llvm_no_path_fallback_for_llvm_as.log" `
        -EnvOverrides @{ PATH = $pathValue }

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue26_repo_llvm_no_path_fallback_for_llvm_as' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue26_repo_llvm_no_path_fallback_for_llvm_as' expected zero diagnostics but observed $errorCount."
    }

    if (-not (Test-Path (Join-Path $result.CaseRoot "build\\probe\\ir\\probe.bc"))) {
        throw "Case 'issue26_repo_llvm_no_path_fallback_for_llvm_as' expected tool-driven IR assembly to emit build\\probe\\ir\\probe.bc."
    }
    $repoLlvmDir = Join-Path $result.CaseRoot "build\\probe\\llvm"
    if (Test-Path $repoLlvmDir) {
        throw "Case 'issue26_repo_llvm_no_path_fallback_for_llvm_as' must not materialize a repo LLVM staging directory: $repoLlvmDir"
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $resolveIrErr = @($logLines | Where-Object { $_ -like "*`tResolveTool-Err-IR`t*" }).Count
    $resolveOk = @($logLines | Where-Object { $_ -like "*`tResolveTool-Ok`t*" }).Count
    $assembleOk = @($logLines | Where-Object { $_ -like "*`tAssembleIR-Ok`t*" }).Count
    if ($resolveIrErr -ne 0 -or $resolveOk -lt 1 -or $assembleOk -lt 1) {
        throw "Case 'issue26_repo_llvm_no_path_fallback_for_llvm_as' expected compiler sidecar ResolveTool-Ok and AssembleIR-Ok with no ResolveTool-Err-IR trace."
    }

    Write-Host "[compiler-static] issue26_repo_llvm_no_path_fallback_for_llvm_as: exit=$($result.ExitCode) errors=$errorCount resolve_ir_err=$resolveIrErr resolve_ok=$resolveOk assemble_ok=$assembleOk"
}

function Invoke-Issue26AssembleIrErrTransitionCase {
    $caseRoot = Join-Path $workRoot "issue26_assemble_ir_err_transition"
    $fakeLlvmBin = Join-Path $caseRoot "fake_llvm"
    New-Item -ItemType Directory -Path $fakeLlvmBin -Force | Out-Null
    New-FakeLlvmAsExecutable `
        -OutputPath (Join-Path $fakeLlvmBin "llvm-as.exe") `
        -ReportedVersion "21.1.8" `
        -FailAssemble
    $fakeLlvmTomlPath = ($fakeLlvmBin -replace "\\", "/")

    $manifest = @(
        "[toolchain]",
        "llvm_bin = ""$fakeLlvmTomlPath""",
        "target_profile = ""x86_64-win64""",
        "",
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""bc"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue26_assemble_ir_err_transition" `
        -CaseRootOverride $caseRoot `
        -Source (New-MinimalMainSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue26_assemble_ir_err_transition.log"

    if ($result.ExitCode -ne 1) {
        throw "Case 'issue26_assemble_ir_err_transition' expected exit 1 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -lt 1) {
        throw "Case 'issue26_assemble_ir_err_transition' expected at least one diagnostic."
    }
    $emitIrDiag = @($result.DiagJson.diagnostics | Where-Object {
        $_.code -eq "E-OUT-0403"
    }).Count
    if ($emitIrDiag -lt 1) {
        throw "Case 'issue26_assemble_ir_err_transition' expected E-OUT-0403."
    }

    if (Test-Path (Join-Path $result.CaseRoot "build\\probe\\ir\\probe.bc")) {
        throw "Case 'issue26_assemble_ir_err_transition' must not emit build\\probe\\ir\\probe.bc' after AssembleIR-Err."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $resolveOk = @($logLines | Where-Object { $_ -like "*`tResolveTool-Ok`t*" }).Count
    $resolveIrErr = @($logLines | Where-Object { $_ -like "*`tResolveTool-Err-IR`t*" }).Count
    $assembleOk = @($logLines | Where-Object { $_ -like "*`tAssembleIR-Ok`t*" }).Count
    $assembleErr = @($logLines | Where-Object { $_ -like "*`tAssembleIR-Err`t*" }).Count
    $outIrErr = @($logLines | Where-Object { $_ -like "*`tOut-IR-Err`t*" }).Count
    if ($resolveOk -lt 1 -or $resolveIrErr -ne 0 -or $assembleOk -ne 0 -or $assembleErr -lt 1 -or $outIrErr -lt 1) {
        throw "Case 'issue26_assemble_ir_err_transition' expected ResolveTool-Ok followed by AssembleIR-Err and Out-IR-Err."
    }

    Write-Host "[compiler-static] issue26_assemble_ir_err_transition: exit=$($result.ExitCode) errors=$errorCount resolve_ir_err=$resolveIrErr resolve_ok=$resolveOk assemble_err=$assembleErr out_ir_err=$outIrErr"
}

function Invoke-Issue26LlvmAsWrongVersionRejectedCase {
    $caseRoot = Join-Path $workRoot "issue26_llvm_as_wrong_version_rejected"
    $fakeLlvmBin = Join-Path $caseRoot "fake_llvm"
    New-Item -ItemType Directory -Path $fakeLlvmBin -Force | Out-Null
    New-FakeLlvmAsExecutable `
        -OutputPath (Join-Path $fakeLlvmBin "llvm-as.exe") `
        -ReportedVersion "0.0.0"
    $fakeLlvmTomlPath = ($fakeLlvmBin -replace "\\", "/")

    $manifest = @(
        "[toolchain]",
        "llvm_bin = ""$fakeLlvmTomlPath""",
        "target_profile = ""x86_64-win64""",
        "",
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""bc"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue26_llvm_as_wrong_version_rejected" `
        -CaseRootOverride $caseRoot `
        -Source (New-MinimalMainSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue26_llvm_as_wrong_version_rejected.log"

    if ($result.ExitCode -ne 1) {
        throw "Case 'issue26_llvm_as_wrong_version_rejected' expected exit 1 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -lt 1) {
        throw "Case 'issue26_llvm_as_wrong_version_rejected' expected at least one diagnostic."
    }
    $emitIrDiag = @($result.DiagJson.diagnostics | Where-Object {
        $_.code -eq "E-OUT-0403"
    }).Count
    if ($emitIrDiag -lt 1) {
        throw "Case 'issue26_llvm_as_wrong_version_rejected' expected E-OUT-0403."
    }

    if (Test-Path (Join-Path $result.CaseRoot "build\\probe\\ir\\probe.bc")) {
        throw "Case 'issue26_llvm_as_wrong_version_rejected' must not emit build\\probe\\ir\\probe.bc."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $resolveOk = @($logLines | Where-Object { $_ -like "*`tResolveTool-Ok`t*" }).Count
    $resolveIrErr = @($logLines | Where-Object { $_ -like "*`tResolveTool-Err-IR`t*" }).Count
    $assembleErr = @($logLines | Where-Object { $_ -like "*`tAssembleIR-Err`t*" }).Count
    $emitLlvmErr = @($logLines | Where-Object { $_ -like "*`tEmitLLVM-Err`t*" }).Count
    $outIrErr = @($logLines | Where-Object { $_ -like "*`tOut-IR-Err`t*" }).Count
    if ($resolveOk -ne 0 -or $resolveIrErr -lt 1 -or $assembleErr -ne 0 -or $emitLlvmErr -ne 0 -or $outIrErr -lt 1) {
        throw "Case 'issue26_llvm_as_wrong_version_rejected' expected ResolveTool-Err-IR and Out-IR-Err with no ResolveTool-Ok, AssembleIR-Err, or EmitLLVM-Err trace."
    }

    Write-Host "[compiler-static] issue26_llvm_as_wrong_version_rejected: exit=$($result.ExitCode) errors=$errorCount resolve_ir_err=$resolveIrErr out_ir_err=$outIrErr"
}

function Invoke-Issue26CliRejectsLlvmBinFlagCase {
    $caseRoot = Join-Path $workRoot "issue26_cli_rejects_llvm_bin_flag"
    New-Item -ItemType Directory -Path $caseRoot -Force | Out-Null
    [System.IO.File]::WriteAllLines((Join-Path $caseRoot "Cursive.toml"), $manifestLines)
    [System.IO.File]::WriteAllText((Join-Path $caseRoot "Main.cursive"), (New-MinimalMainSource))

    $stdoutPath = Join-Path $caseRoot "stdout.txt"
    $stderrPath = Join-Path $caseRoot "stderr.txt"

    Push-Location $caseRoot
    try {
        $cmdLine = "`"$CompilerPath`" build --incremental off --diag-json --quiet --llvm-bin C:/definitely/not/here Main.cursive 1> `"$stdoutPath`" 2> `"$stderrPath`""
        cmd /c $cmdLine
        $exitCode = $LASTEXITCODE
    } finally {
        Pop-Location
    }

    if ($exitCode -ne 2) {
        throw "Case 'issue26_cli_rejects_llvm_bin_flag' expected exit 2 but got $exitCode."
    }
    $stderrText = Get-Content -Path $stderrPath -Raw
    if ($stderrText -notmatch "unknown option: --llvm-bin") {
        throw "Case 'issue26_cli_rejects_llvm_bin_flag' expected unknown option error for --llvm-bin."
    }

    Write-Host "[compiler-static] issue26_cli_rejects_llvm_bin_flag: exit=$exitCode"
}

function Invoke-Issue66EmitLlvmErrLlTransitionCase {
    $caseRoot = Join-Path $workRoot "issue66_emit_llvm_err_ll_transition"
    $fakeLlvmBin = Join-Path $caseRoot "fake_llvm"
    New-Item -ItemType Directory -Path $fakeLlvmBin -Force | Out-Null
    New-FakeLlvmAsExecutable `
        -OutputPath (Join-Path $fakeLlvmBin "llvm-as.exe") `
        -ReportedVersion "21.1.8" `
        -FailAssemble
    $fakeLlvmTomlPath = ($fakeLlvmBin -replace "\\", "/")

    $manifest = @(
        "[toolchain]",
        "llvm_bin = ""$fakeLlvmTomlPath""",
        "target_profile = ""x86_64-win64""",
        "",
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue66_emit_llvm_err_ll_transition" `
        -CaseRootOverride $caseRoot `
        -Source (New-MinimalMainSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue66_emit_llvm_err_ll_transition.log"

    if ($result.ExitCode -ne 1) {
        throw "Case 'issue66_emit_llvm_err_ll_transition' expected exit 1 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -lt 1) {
        throw "Case 'issue66_emit_llvm_err_ll_transition' expected at least one diagnostic."
    }
    $emitIrDiag = @($result.DiagJson.diagnostics | Where-Object {
        $_.code -eq "E-OUT-0403"
    }).Count
    if ($emitIrDiag -lt 1) {
        throw "Case 'issue66_emit_llvm_err_ll_transition' expected E-OUT-0403."
    }

    if (Test-Path (Join-Path $result.CaseRoot "build\\probe\\ir\\probe.ll")) {
        throw "Case 'issue66_emit_llvm_err_ll_transition' must not emit build\\probe\\ir\\probe.ll after EmitLLVM-Err."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $resolveOk = @($logLines | Where-Object { $_ -like "*`tResolveTool-Ok`t*" }).Count
    $resolveIrErr = @($logLines | Where-Object { $_ -like "*`tResolveTool-Err-IR`t*" }).Count
    $assembleOk = @($logLines | Where-Object { $_ -like "*`tAssembleIR-Ok`t*" }).Count
    $assembleErr = @($logLines | Where-Object { $_ -like "*`tAssembleIR-Err`t*" }).Count
    $emitLlvmErr = @($logLines | Where-Object { $_ -like "*`tEmitLLVM-Err`t*" }).Count
    $outIrErr = @($logLines | Where-Object { $_ -like "*`tOut-IR-Err`t*" }).Count
    if ($resolveOk -lt 1 -or $resolveIrErr -ne 0 -or $assembleOk -ne 0 -or $assembleErr -lt 1 -or $emitLlvmErr -lt 1 -or $outIrErr -lt 1) {
        throw "Case 'issue66_emit_llvm_err_ll_transition' expected ResolveTool-Ok, AssembleIR-Err, EmitLLVM-Err, and Out-IR-Err on ll emission failure."
    }

    Write-Host "[compiler-static] issue66_emit_llvm_err_ll_transition: exit=$($result.ExitCode) errors=$errorCount resolve_ir_err=$resolveIrErr resolve_ok=$resolveOk assemble_err=$assembleErr emit_llvm_err=$emitLlvmErr out_ir_err=$outIrErr"
}

function Invoke-Issue66EmitLlvmErrLowerBoundaryConformanceCase {
    $pipelinePath = Join-Path $workspaceRoot "cursive\\src\\06_driver\\pipeline.cpp"
    if (-not (Test-Path $pipelinePath)) {
        throw "Case 'issue66_emit_llvm_err_lower_boundary_conformance' missing pipeline source: $pipelinePath"
    }

    $pipelineText = Get-Content -Path $pipelinePath -Raw
    $bundleBranch = [regex]::Match(
        $pipelineText,
        '(?ms)auto bundle = EmitLLVMModule\(cache, module, project\);\s*if \(!bundle\) \{(?<body>.*?)\}')
    if (-not $bundleBranch.Success) {
        throw "Case 'issue66_emit_llvm_err_lower_boundary_conformance' could not locate EmitIRForModule lower-ir failure branch."
    }
    $bundleBody = $bundleBranch.Groups['body'].Value
    if ($bundleBody -match 'EmitLLVM-Err') {
        throw "Case 'issue66_emit_llvm_err_lower_boundary_conformance' found EmitLLVM-Err in the pre-render lower-ir failure branch."
    }
    if ($bundleBody -notmatch 'stage=lower-ir') {
        throw "Case 'issue66_emit_llvm_err_lower_boundary_conformance' expected lower-ir branch logging in EmitIRForModule."
    }

    $renderHelper = [regex]::Match(
        $pipelineText,
        '(?ms)std::optional<std::string>\s+RenderLLVMText\(.*?return text;\s*\}')
    if (-not $renderHelper.Success) {
        throw "Case 'issue66_emit_llvm_err_lower_boundary_conformance' missing RenderLLVMText helper."
    }
    $renderBody = $renderHelper.Value
    if ($renderBody -notmatch 'AssembleIR\(assembler,\s*text\)') {
        throw "Case 'issue66_emit_llvm_err_lower_boundary_conformance' expected RenderLLVMText to validate rendered text with AssembleIR."
    }
    if ($renderBody -notmatch 'SPEC_RULE\("EmitLLVM-Err"\)') {
        throw "Case 'issue66_emit_llvm_err_lower_boundary_conformance' expected EmitLLVM-Err to be emitted from RenderLLVMText."
    }

    $emitIrBody = [regex]::Match(
        $pipelineText,
        '(?ms)std::optional<std::string>\s+EmitIRForModule\(.*?return out;\s*\}')
    if (-not $emitIrBody.Success) {
        throw "Case 'issue66_emit_llvm_err_lower_boundary_conformance' could not locate EmitIRForModule body."
    }
    $emitIrText = $emitIrBody.Value
    if ($emitIrText -notmatch 'ResolveTool\(project,\s*"llvm-as"\)') {
        throw "Case 'issue66_emit_llvm_err_lower_boundary_conformance' expected EmitIRForModule to resolve llvm-as before RenderLLVMText."
    }

    Write-Host "[compiler-static] issue66_emit_llvm_err_lower_boundary_conformance: lower_boundary=1 render_helper=1"
}

function Invoke-Issue66LlvmResolveBoundaryCase {
    $caseRoot = Join-Path $workRoot "issue66_llvm_resolve_boundary"
    $fakeLlvmBin = Join-Path $caseRoot "fake_llvm"
    New-Item -ItemType Directory -Path $fakeLlvmBin -Force | Out-Null
    New-FakeLlvmAsExecutable `
        -OutputPath (Join-Path $fakeLlvmBin "llvm-as.exe") `
        -ReportedVersion "0.0.0"
    $fakeLlvmTomlPath = ($fakeLlvmBin -replace "\\", "/")

    $manifest = @(
        "[toolchain]",
        "llvm_bin = ""$fakeLlvmTomlPath""",
        "target_profile = ""x86_64-win64""",
        "",
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue66_llvm_resolve_boundary" `
        -CaseRootOverride $caseRoot `
        -Source (New-MinimalMainSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue66_llvm_resolve_boundary.log"

    if ($result.ExitCode -ne 1) {
        throw "Case 'issue66_llvm_resolve_boundary' expected exit 1 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -lt 1) {
        throw "Case 'issue66_llvm_resolve_boundary' expected at least one diagnostic."
    }

    $emitIrDiag = @($result.DiagJson.diagnostics | Where-Object {
        $_.code -eq "E-OUT-0403"
    }).Count
    if ($emitIrDiag -lt 1) {
        throw "Case 'issue66_llvm_resolve_boundary' expected E-OUT-0403."
    }

    if (Test-Path (Join-Path $result.CaseRoot "build\\probe\\ir\\probe.ll")) {
        throw "Case 'issue66_llvm_resolve_boundary' must not emit build\\probe\\ir\\probe.ll."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $resolveOk = @($logLines | Where-Object { $_ -like "*`tResolveTool-Ok`t*" }).Count
    $resolveIrErr = @($logLines | Where-Object { $_ -like "*`tResolveTool-Err-IR`t*" }).Count
    $assembleErr = @($logLines | Where-Object { $_ -like "*`tAssembleIR-Err`t*" }).Count
    $emitLlvmErr = @($logLines | Where-Object { $_ -like "*`tEmitLLVM-Err`t*" }).Count
    $outIrErr = @($logLines | Where-Object { $_ -like "*`tOut-IR-Err`t*" }).Count
    if ($resolveOk -ne 0 -or $resolveIrErr -lt 1 -or $assembleErr -ne 0 -or $emitLlvmErr -ne 0 -or $outIrErr -lt 1) {
        throw "Case 'issue66_llvm_resolve_boundary' expected ResolveTool-Err-IR and Out-IR-Err with no ResolveTool-Ok, AssembleIR-Err, or EmitLLVM-Err trace."
    }

    Write-Host "[compiler-static] issue66_llvm_resolve_boundary: exit=$($result.ExitCode) errors=$errorCount resolve_ir_err=$resolveIrErr out_ir_err=$outIrErr"
}

function Invoke-Issue27FfiSurfaceTraceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue27_ffi_surface_trace" `
        -Source (New-Issue27FfiSurfaceSource) `
        -ConformanceFileName "issue27_ffi_surface_trace.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue27_ffi_surface_trace' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue27_ffi_surface_trace' expected zero compile-time errors, observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $ffiBoundaryCount = @($logLines | Where-Object { $_ -like "*`tFFIBoundary`t*" }).Count
    $unwindDefaultCount = @($logLines | Where-Object { $_ -like "*`tUnwindMode-Default`t*" }).Count
    if ($ffiBoundaryCount -lt 2 -or $unwindDefaultCount -lt 2) {
        throw "Case 'issue27_ffi_surface_trace' expected FFIBoundary and UnwindMode-Default traces for import/export boundaries."
    }

    Write-Host "[compiler-static] issue27_ffi_surface_trace: exit=$($result.ExitCode) ffi_boundary=$ffiBoundaryCount unwind_default=$unwindDefaultCount"
}

function Invoke-Issue27ImportUnwindCodegenTraceCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue27_import_unwind_codegen_trace" `
        -Source (New-Issue27ImportUnwindCodegenSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue27_import_unwind_codegen_trace.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue27_import_unwind_codegen_trace' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue27_import_unwind_codegen_trace' expected zero diagnostics, observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $abortImport = @($logLines | Where-Object { $_ -like "*`tCodeGen-UnwindAbort-Import`t*" }).Count
    $catchImport = @($logLines | Where-Object { $_ -like "*`tCodeGen-UnwindCatch-Import`t*" }).Count
    if ($abortImport -lt 1 -or $catchImport -lt 1) {
        throw "Case 'issue27_import_unwind_codegen_trace' expected CodeGen-UnwindAbort-Import and CodeGen-UnwindCatch-Import traces."
    }

    Write-Host "[compiler-static] issue27_import_unwind_codegen_trace: exit=$($result.ExitCode) abort_import=$abortImport catch_import=$catchImport"
}

function Invoke-Issue513BindingDeprecatedWarningCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue513_binding_deprecated_warning" `
        -Source (New-Issue513BindingDeprecatedWarningSource) `
        -ConformanceFileName "issue513_binding_deprecated_warning.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue513_binding_deprecated_warning' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue513_binding_deprecated_warning' expected zero compile-time errors, observed $errorCount."
    }

    $warningCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "warning" -and $_.code -eq "W-CNF-0601"
    }).Count
    if ($warningCount -lt 1) {
        throw "Case 'issue513_binding_deprecated_warning' expected warning code W-CNF-0601."
    }

    Write-Host "[compiler-static] issue513_binding_deprecated_warning: exit=$($result.ExitCode) errors=$errorCount w_cnf_0601=$warningCount"
}

function Invoke-Issue513StaleWarningCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue513_stale_warning_yield_release" `
        -Source (New-Issue513StaleWarningYieldReleaseSource) `
        -ConformanceFileName "issue513_stale_warning_yield_release.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue513_stale_warning_yield_release' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue513_stale_warning_yield_release' expected zero compile-time errors, observed $errorCount."
    }

    $warningCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "warning" -and $_.code -eq "W-CON-0011"
    }).Count
    if ($warningCount -lt 1) {
        throw "Case 'issue513_stale_warning_yield_release' expected warning code W-CON-0011."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $staleRuleCount = @($logLines | Where-Object { $_ -like "*`tK-Check-Staleness`t*" }).Count
    if ($staleRuleCount -lt 1) {
        throw "Case 'issue513_stale_warning_yield_release' expected K-Check-Staleness in conformance trace."
    }

    Write-Host "[compiler-static] issue513_stale_warning_yield_release: exit=$($result.ExitCode) errors=$errorCount w_con_0011=$warningCount stale_rule=$staleRuleCount"
}

function Invoke-Issue513LlvmAbiInlineWiringCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue513_llvm_abi_inline_wiring" `
        -Source (New-Issue513AbiInlineColdLlSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue513_llvm_abi_inline_wiring.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue513_llvm_abi_inline_wiring' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue513_llvm_abi_inline_wiring' expected zero diagnostics, observed $errorCount."
    }

    $irPath = Join-Path $result.CaseRoot "build\\probe\\ir\\probe.ll"
    if (-not (Test-Path $irPath)) {
        throw "Case 'issue513_llvm_abi_inline_wiring' missing LLVM IR file: $irPath"
    }

    $irText = Get-Content -Path $irPath -Raw
    $vectorDefine = ([regex]::Matches($irText, "define\s+x86_vectorcallcc\s+i32\s+@vectorcall_export")).Count
    $vectorCall = ([regex]::Matches($irText, "call\s+x86_vectorcallcc\s+i32\s+@vectorcall_export")).Count
    $alwaysInlineCount = ([regex]::Matches($irText, "\balwaysinline\b")).Count
    $noInlineCount = ([regex]::Matches($irText, "\bnoinline\b")).Count
    $coldCount = ([regex]::Matches($irText, "\bcold\b")).Count

    if ($vectorDefine -lt 1 -or $vectorCall -lt 1) {
        throw "Case 'issue513_llvm_abi_inline_wiring' expected vectorcall calling convention on declaration and call site."
    }
    if ($alwaysInlineCount -lt 1 -or $noInlineCount -lt 1 -or $coldCount -lt 1) {
        throw "Case 'issue513_llvm_abi_inline_wiring' expected alwaysinline, noinline, and cold attributes in LLVM IR."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $cgProcCount = @($logLines | Where-Object { $_ -like "*`tCG-Item-Procedure`t*" }).Count
    if ($cgProcCount -lt 3) {
        throw "Case 'issue513_llvm_abi_inline_wiring' expected multiple CG-Item-Procedure traces."
    }

    Write-Host "[compiler-static] issue513_llvm_abi_inline_wiring: exit=$($result.ExitCode) vector_define=$vectorDefine vector_call=$vectorCall alwaysinline=$alwaysInlineCount noinline=$noInlineCount cold=$coldCount cg_proc=$cgProcCount"
}

function Invoke-Issue513LayoutAlignWarningCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue513_layout_align_warning" `
        -Source (New-Issue513LayoutAlignWarningSource) `
        -ConformanceFileName "issue513_layout_align_warning.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue513_layout_align_warning' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue513_layout_align_warning' expected zero compile-time errors, observed $errorCount."
    }

    $warningCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "warning" -and $_.code -eq "W-MOD-2451"
    }).Count
    if ($warningCount -lt 1) {
        throw "Case 'issue513_layout_align_warning' expected warning code W-MOD-2451."
    }

    Write-Host "[compiler-static] issue513_layout_align_warning: exit=$($result.ExitCode) errors=$errorCount w_mod_2451=$warningCount"
}

function Invoke-Issue513LayoutLlWiringCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue513_layout_ll_wiring" `
        -Source (New-Issue513LayoutLlSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue513_layout_ll_wiring.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue513_layout_ll_wiring' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue513_layout_ll_wiring' expected zero diagnostics, observed $errorCount."
    }

    $irPath = Join-Path $result.CaseRoot "build\\probe\\ir\\probe.ll"
    if (-not (Test-Path $irPath)) {
        throw "Case 'issue513_layout_ll_wiring' missing LLVM IR file: $irPath"
    }

    $irText = Get-Content -Path $irPath -Raw
    $packedRecordCount = ([regex]::Matches($irText, "<\{\s*i8,\s*i32\s*\}>")).Count
    $alignedRecordCount = ([regex]::Matches($irText, "\{\s*i8,\s*\[15 x i8\],\s*\[0 x i128\]\s*\}")).Count
    $discEnumCount = ([regex]::Matches($irText, "\{\s*i16,\s*\[2 x i8\],\s*\[4 x i8\]\s*\}")).Count
    $align16AllocaCount = ([regex]::Matches($irText, "alloca\s+\{\s*i8,\s*\[15 x i8\],\s*\[0 x i128\]\s*\},\s*align\s+16")).Count
    $packedAlign1AllocaCount = ([regex]::Matches($irText, "alloca\s+<\{\s*i8,\s*i32\s*\}>,\s*align\s+1")).Count

    if ($packedRecordCount -lt 1) {
        throw "Case 'issue513_layout_ll_wiring' expected packed record LLVM representation '<{ i8, i32 }>'."
    }
    if ($alignedRecordCount -lt 1) {
        throw "Case 'issue513_layout_ll_wiring' expected align(16) record LLVM representation with tail padding + alignment marker."
    }
    if ($discEnumCount -lt 1) {
        throw "Case 'issue513_layout_ll_wiring' expected enum layout(u16) discriminant lowering in LLVM representation."
    }
    if ($align16AllocaCount -lt 1) {
        throw "Case 'issue513_layout_ll_wiring' expected at least one align(16) alloca site."
    }
    if ($packedAlign1AllocaCount -lt 1) {
        throw "Case 'issue513_layout_ll_wiring' expected at least one packed alloca site with align 1."
    }

    Write-Host "[compiler-static] issue513_layout_ll_wiring: exit=$($result.ExitCode) packed=$packedRecordCount align16=$alignedRecordCount enum_u16=$discEnumCount align16_alloca=$align16AllocaCount packed_align1_alloca=$packedAlign1AllocaCount"
}

function Invoke-Issue513InlineHintParseTraceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue513_inline_hint_parse_trace" `
        -Source (New-Issue513InlineHintRejectedSource) `
        -ConformanceFileName "issue513_inline_hint_parse_trace.log"

    if ($result.ExitCode -ne 1) {
        throw "Case 'issue513_inline_hint_parse_trace' expected exit 1 but got $($result.ExitCode)."
    }

    $diagCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.code -eq "E-MOD-2450"
    }).Count
    if ($diagCount -lt 1) {
        throw "Case 'issue513_inline_hint_parse_trace' expected diagnostic code E-MOD-2450."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $parseAttrSpecCount = @($logLines | Where-Object { $_ -like "*`tParse-AttrSpec`t*" }).Count
    $attrListWfCount = @($logLines | Where-Object { $_ -like "*`tAttrListWf`t*" }).Count
    $attrValidationCount = @($logLines | Where-Object { $_ -like "*`tAttrValidation`t*" }).Count

    if ($parseAttrSpecCount -lt 1) {
        throw "Case 'issue513_inline_hint_parse_trace' expected Parse-AttrSpec in the conformance trace."
    }
    if ($attrListWfCount -ne 0 -or $attrValidationCount -ne 0) {
        throw "Case 'issue513_inline_hint_parse_trace' must fail during parsing before attribute validation."
    }

    $parserPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\item\\attribute_list.cpp"
    if (-not (Test-Path $parserPath)) {
        throw "Case 'issue513_inline_hint_parse_trace' missing parser implementation file: $parserPath"
    }

    $parserText = Get-Content -Path $parserPath -Raw
    $requiredPatterns = @(
        'static bool UsesInlineArgGrammar\(const AttrName& name\)',
        'static bool IsInlineModeLexeme\(std::string_view lexeme\)',
        'ParseElemResult<std::vector<AttributeArg>> ParseInlineArgList\(Parser parser\)',
        'else if \(UsesInlineArgGrammar\(item\.name\)\)\s*\{\s*args = ParseInlineArgList\(after_open\);'
    )

    foreach ($pattern in $requiredPatterns) {
        if ($parserText -notmatch $pattern) {
            throw "Case 'issue513_inline_hint_parse_trace' missing expected inline-attribute parser pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue513_inline_hint_parse_trace: exit=$($result.ExitCode) e_mod_2450=$diagCount parse_attr_spec=$parseAttrSpecCount attr_list_wf=$attrListWfCount attr_validation=$attrValidationCount"
}

function Invoke-Issue513ColdHintParseTraceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue513_cold_hint_parse_trace" `
        -Source (New-Issue513ColdHintRejectedSource) `
        -ConformanceFileName "issue513_cold_hint_parse_trace.log"

    if ($result.ExitCode -ne 1) {
        throw "Case 'issue513_cold_hint_parse_trace' expected exit 1 but got $($result.ExitCode)."
    }

    $diagCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.code -eq "E-MOD-2450"
    }).Count
    if ($diagCount -lt 1) {
        throw "Case 'issue513_cold_hint_parse_trace' expected diagnostic code E-MOD-2450."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $parseAttrSpecCount = @($logLines | Where-Object { $_ -like "*`tParse-AttrSpec`t*" }).Count
    $attrListWfCount = @($logLines | Where-Object { $_ -like "*`tAttrListWf`t*" }).Count
    $attrValidationCount = @($logLines | Where-Object { $_ -like "*`tAttrValidation`t*" }).Count

    if ($parseAttrSpecCount -lt 1) {
        throw "Case 'issue513_cold_hint_parse_trace' expected Parse-AttrSpec in the conformance trace."
    }
    if ($attrListWfCount -ne 0 -or $attrValidationCount -ne 0) {
        throw "Case 'issue513_cold_hint_parse_trace' must fail during parsing before attribute validation."
    }

    $parserPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\item\\attribute_list.cpp"
    if (-not (Test-Path $parserPath)) {
        throw "Case 'issue513_cold_hint_parse_trace' missing parser implementation file: $parserPath"
    }

    $parserText = Get-Content -Path $parserPath -Raw
    $requiredPatterns = @(
        'static bool UsesBareMarkerAttrGrammar\(const AttrName& name\)',
        'return !name.vendor_prefix_opt.has_value\(\) && name.leaf_name == "cold";',
        'if \(UsesBareMarkerAttrGrammar\(item\.name\)\)\s*\{\s*Parser after_open = next;\s*Advance\(after_open\);\s*SkipNewlines\(after_open\);\s*item\.span = SpanBetween\(parser, after_open\);\s*EmitAttrSyntaxErr\(after_open, item\.span\);'
    )

    foreach ($pattern in $requiredPatterns) {
        if ($parserText -notmatch $pattern) {
            throw "Case 'issue513_cold_hint_parse_trace' missing expected cold-attribute parser pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue513_cold_hint_parse_trace: exit=$($result.ExitCode) e_mod_2450=$diagCount parse_attr_spec=$parseAttrSpecCount attr_list_wf=$attrListWfCount attr_validation=$attrValidationCount"
}

function Invoke-Issue513ExportByValueTraceCase {
    $rejected = Invoke-CheckWithConformance `
        -CaseId "issue513_export_by_value_rejected" `
        -Source (New-Issue513ExportByValueDropTypeRejectedSource) `
        -ConformanceFileName "issue513_export_by_value_rejected.log"

    if ($rejected.ExitCode -ne 1) {
        throw "Case 'issue513_export_by_value_rejected' expected exit 1 but got $($rejected.ExitCode)."
    }

    $rejectedErrorCount = @($rejected.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($rejectedErrorCount -lt 1) {
        throw "Case 'issue513_export_by_value_rejected' expected at least one compile-time error."
    }

    $rejectedDiagCount = @($rejected.DiagJson.diagnostics | Where-Object { $_.code -eq "E-TYP-2630" }).Count
    if ($rejectedDiagCount -lt 1) {
        throw "Case 'issue513_export_by_value_rejected' expected diagnostic code E-TYP-2630."
    }

    $rejectedLog = Get-Content -Path $rejected.ConformancePath
    $exportByValueErrCount = @($rejectedLog | Where-Object { $_ -like "*`tExport-ByValue-Err`t*" }).Count
    if ($exportByValueErrCount -lt 1) {
        throw "Case 'issue513_export_by_value_rejected' expected Export-ByValue-Err in conformance trace."
    }

    $accepted = Invoke-CheckWithConformance `
        -CaseId "issue513_export_by_value_accepted" `
        -Source (New-Issue513ExportByValueDropTypeAcceptedSource) `
        -ConformanceFileName "issue513_export_by_value_accepted.log"

    if ($accepted.ExitCode -ne 0) {
        throw "Case 'issue513_export_by_value_accepted' expected exit 0 but got $($accepted.ExitCode)."
    }

    $acceptedErrorCount = @($accepted.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($acceptedErrorCount -ne 0) {
        throw "Case 'issue513_export_by_value_accepted' expected zero compile-time errors, observed $acceptedErrorCount."
    }

    Write-Host "[compiler-static] issue513_export_by_value_trace: rejected_errors=$rejectedErrorCount e_typ_2630=$rejectedDiagCount export_byvalue_err=$exportByValueErrCount accepted_errors=$acceptedErrorCount"
}

function Invoke-Issue513DualPathDuplicateSymbolWiringCase {
    $presentChecks = @(
        @{ Path = $canonicalSpecPath; Pattern = 'Duplicate symbol names within a compilation unit are ill-formed and MUST be diagnosed at compile-time or link-time\.'; Absolute = $true },
        @{ Path = $canonicalSpecPath; Pattern = '\| `E-SYS-3342` \| Error\s+\| Compile-time or Link-time \| Duplicate symbol name in compilation unit'; Absolute = $true },
        @{ Path = "cursive\src\04_analysis\typing\typecheck.cpp"; Pattern = 'EmitTypecheckDiag\(diags,\s*"E-SYS-3342"' },
        @{ Path = "cursive\src\01_project\link.cpp"; Pattern = 'DuplicateDefinedExternalSymbolsForObjectInputs' },
        @{ Path = "cursive\src\01_project\link.cpp"; Pattern = 'MakeExternalDiagnostic\("E-SYS-3342"\)' }
    )

    $absentChecks = @(
        @{ Path = $canonicalSpecPath; Pattern = 'Duplicate symbol names within a compilation unit are link-time errors\.'; Absolute = $true }
    )

    foreach ($check in $presentChecks) {
        $isAbsolute = $check.ContainsKey("Absolute") -and [bool]$check.Absolute
        $fullPath = if ($isAbsolute) { $check.Path } else { Join-Path $workspaceRoot $check.Path }
        if (-not (Test-Path $fullPath)) {
            throw "Case 'issue513_dual_path_duplicate_symbol_wiring' missing file: $fullPath"
        }
        $text = Get-Content -Path $fullPath -Raw
        if ($text -notmatch $check.Pattern) {
            throw "Case 'issue513_dual_path_duplicate_symbol_wiring' missing expected pattern '$($check.Pattern)' in $fullPath."
        }
    }

    foreach ($check in $absentChecks) {
        $isAbsolute = $check.ContainsKey("Absolute") -and [bool]$check.Absolute
        $fullPath = if ($isAbsolute) { $check.Path } else { Join-Path $workspaceRoot $check.Path }
        if (-not (Test-Path $fullPath)) {
            throw "Case 'issue513_dual_path_duplicate_symbol_wiring' missing file: $fullPath"
        }
        $text = Get-Content -Path $fullPath -Raw
        if ($text -match $check.Pattern) {
            throw "Case 'issue513_dual_path_duplicate_symbol_wiring' found stale/non-canonical pattern '$($check.Pattern)' in $fullPath."
        }
    }

    Write-Host "[compiler-static] issue513_dual_path_duplicate_symbol_wiring: present_checks=$($presentChecks.Count) absent_checks=$($absentChecks.Count)"
}

function Invoke-NoAmbientExplicitContextFlowCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "no_ambient_explicit_context_flow" `
        -Source (New-NoAmbientExplicitContextFlowSource) `
        -ConformanceFileName "no_ambient_explicit_context_flow.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'no_ambient_explicit_context_flow' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'no_ambient_explicit_context_flow' expected zero compile-time errors, observed $errorCount."
    }

    $ambientCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.code -eq "E-CON-0020"
    }).Count
    if ($ambientCount -ne 0) {
        throw "Case 'no_ambient_explicit_context_flow' unexpectedly emitted E-CON-0020."
    }

    Write-Host "[compiler-static] no_ambient_explicit_context_flow: exit=$($result.ExitCode) errors=$errorCount e_con_0020=$ambientCount"
}

function Invoke-Issue32NoStdReservedBypassCase {
    $files = @(
        (Join-Path $workspaceRoot "cursive\\src\\04_analysis\\typing\\item\\import_decl.cpp"),
        (Join-Path $workspaceRoot "cursive\\src\\04_analysis\\typing\\item\\using_decl.cpp")
    )
    $stdBypassPattern = '\|\|\s*path\[0\]\s*==\s*"std"'

    foreach ($file in $files) {
        if (-not (Test-Path $file)) {
            throw "Case 'issue32_no_std_reserved_bypass' missing file: $file"
        }
        $text = Get-Content -Path $file -Raw
        if ($text -match $stdBypassPattern) {
            throw "Case 'issue32_no_std_reserved_bypass' found typing reserved-path std bypass in $file."
        }
    }

    Write-Host "[compiler-static] issue32_no_std_reserved_bypass: files=$($files.Count) std_bypass=0"
}

function Invoke-Issue33FixedIdentifiersCoverageCase {
    $keywordsPath = Join-Path $workspaceRoot "cursive\\include\\00_core\\keywords.h"
    if (-not (Test-Path $keywordsPath)) {
        throw "Case 'issue33_fixed_identifiers_coverage' missing file: $keywordsPath"
    }

    $required = @(
        "read", "write", "dynamic", "speculative", "release",
        "cancel", "name", "workgroup", "workgroups", "affinity", "priority",
        "reduce", "ordered", "chunk", "min", "max", "and", "or",
        "pattern", "target", "requires", "emits"
    )

    $text = Get-Content -Path $keywordsPath -Raw
    foreach ($entry in $required) {
        if ($text -notmatch ('"' + [regex]::Escape($entry) + '"')) {
            throw "Case 'issue33_fixed_identifiers_coverage' missing fixed identifier '$entry' in $keywordsPath."
        }
    }

    Write-Host "[compiler-static] issue33_fixed_identifiers_coverage: required=$($required.Count) present=$($required.Count)"
}

function Invoke-Issue33TypeWhereKeywordPolicyCase {
    $policyPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\keyword_policy.cpp"
    if (-not (Test-Path $policyPath)) {
        throw "Case 'issue33_typewhere_keyword_policy' missing file: $policyPath"
    }

    $text = Get-Content -Path $policyPath -Raw
    $match = [regex]::Match($text, 'bool\s+TypeWhereTok\s*\(const\s+Token&\s+tok\)\s*\{(?<body>.*?)\}', [System.Text.RegularExpressions.RegexOptions]::Singleline)
    if (-not $match.Success) {
        throw "Case 'issue33_typewhere_keyword_policy' could not locate TypeWhereTok body in $policyPath."
    }

    $body = $match.Groups['body'].Value
    if ($body -notmatch 'TokenKind::Keyword') {
        throw "Case 'issue33_typewhere_keyword_policy' expected keyword-only TypeWhereTok classification."
    }
    if ($body -match 'TokenKind::Identifier') {
        throw "Case 'issue33_typewhere_keyword_policy' found identifier acceptance in TypeWhereTok body."
    }

    Write-Host "[compiler-static] issue33_typewhere_keyword_policy: keyword_only=1 identifier_acceptance=0"
}

function Invoke-Issue33FixedIdentTokenPolicyCase {
    $policyPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\keyword_policy.cpp"
    if (-not (Test-Path $policyPath)) {
        throw "Case 'issue33_fixed_ident_token_policy' missing file: $policyPath"
    }

    $text = Get-Content -Path $policyPath -Raw
    $match = [regex]::Match($text, 'bool\s+IsFixedIdentTok\s*\(const\s+Token&\s+tok,\s*std::string_view\s+s\)\s*\{(?<body>.*?)\}', [System.Text.RegularExpressions.RegexOptions]::Singleline)
    if (-not $match.Success) {
        throw "Case 'issue33_fixed_ident_token_policy' could not locate IsFixedIdentTok body in $policyPath."
    }

    $body = $match.Groups['body'].Value
    if (($body -notmatch 'IsIdentTok\s*\(\s*tok\s*\)') -and ($body -notmatch 'TokenKind::Identifier')) {
        throw "Case 'issue33_fixed_ident_token_policy' expected identifier-token gating in IsFixedIdentTok."
    }
    if ($body -notmatch 'tok\.lexeme\s*==\s*s') {
        throw "Case 'issue33_fixed_ident_token_policy' expected lexeme equality check in IsFixedIdentTok."
    }
    if ($body -notmatch 'IsFixedIdentifier\s*\(\s*s\s*\)') {
        throw "Case 'issue33_fixed_ident_token_policy' expected IsFixedIdentTok to require membership in FixedIdent."
    }

    Write-Host "[compiler-static] issue33_fixed_ident_token_policy: identifier_gate=1 lexeme_guard=1 fixed_ident_guard=1"
}

function Invoke-Issue33UsingImportAttrWiringCase {
    $checks = @(
        @{ Path = "cursive\\src\\02_source\\ast\\nodes\\ast_items.h"; Pattern = 'struct UsingDecl[\s\S]*?AttrOpt attrs_opt;' },
        @{ Path = "cursive\\src\\02_source\\ast\\nodes\\ast_items.h"; Pattern = 'struct ImportDecl[\s\S]*?AttrOpt attrs_opt;' },
        @{ Path = "cursive\\src\\02_source\\ast\\nodes\\ast_items.h"; Pattern = 'struct StaticDecl[\s\S]*?AttrOpt attrs_opt;' },
        @{ Path = "cursive\\src\\02_source\\parser\\item\\using_decl.cpp"; Pattern = 'decl\.attrs_opt\s*=\s*std::move\(attrs_opt\)' },
        @{ Path = "cursive\\src\\02_source\\parser\\item\\import_decl.cpp"; Pattern = 'decl\.attrs_opt\s*=\s*std::move\(attrs_opt\)' },
        @{ Path = "cursive\\src\\02_source\\parser\\item\\static_decl.cpp"; Pattern = 'decl\.attrs_opt\s*=\s*std::move\(attrs_opt\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\item\\using_decl.cpp"; Pattern = 'ValidateUnsupportedAttributeTarget\(ast::AttrListOf\(decl\.attrs_opt\),\s*"using declarations"\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\item\\import_decl.cpp"; Pattern = 'ValidateUnsupportedAttributeTarget\(ast::AttrListOf\(decl\.attrs_opt\),\s*"import declarations"\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\item\\static_decl.cpp"; Pattern = 'ValidateUnsupportedAttributeTarget\(ast::AttrListOf\(decl\.attrs_opt\),\s*"static declarations"\)' }
    )

    foreach ($check in $checks) {
        $fullPath = Join-Path $workspaceRoot $check.Path
        if (-not (Test-Path $fullPath)) {
            throw "Case 'issue33_using_import_attr_wiring' missing file: $fullPath"
        }
        $text = Get-Content -Path $fullPath -Raw
        if ($text -notmatch $check.Pattern) {
            throw "Case 'issue33_using_import_attr_wiring' missing expected pattern '$($check.Pattern)' in $fullPath."
        }
    }

    Write-Host "[compiler-static] issue33_using_import_attr_wiring: checks=$($checks.Count)"
}

function Invoke-Issue33ImportParseAttrListConformanceCase {
    $checks = @(
        @{
            Path = "docs\\CursiveSpecification.md"
            Pattern = '\*\*\(Parse-AttrListTail-Cons\)\*\*[\s\S]*?Γ ⊢ ParseAttrListTail\(P, attrs\) ⇓ \(P_2, attrs_1\)'
        },
        @{
            Path = "docs\\CursiveSpecification.md"
            Pattern = '\*\*\(Parse-Import\)\*\*[\s\S]*?Γ ⊢ ParseAttrListOpt\(P\) ⇓ \(P_0, attrs_opt\)[\s\S]*?Γ ⊢ ParseItem\(P\) ⇓ \(P_3, ⟨ImportDecl, attrs_opt, vis, path, alias_opt, SpanBetween\(P, P_3\), \[\]⟩\)'
        },
        @{
            Path = "cursive\\src\\02_source\\parser\\item\\attribute_list.cpp"
            Pattern = 'ParseElemResult<AttributeList>\s+ParseAttrListTail\(Parser parser, AttributeList xs\)\s*\{[\s\S]*?if \(!IsAttrStart\(parser\)\)[\s\S]*?ParseElemResult<std::vector<AttributeItem>> block = ParseAttrBlock\(parser\);[\s\S]*?xs\.insert\(xs\.end\(\), block\.elem\.begin\(\), block\.elem\.end\(\)\);[\s\S]*?return ParseAttrListTail\(block\.parser, std::move\(xs\)\);'
        },
        @{
            Path = "cursive\\src\\02_source\\parser\\item\\parse_item.cpp"
            Pattern = 'ParseElemResult<AttrOpt>\s+attrs = ParseAttributeListOpt\(parser\);[\s\S]*?if \(IsKw\(parser, "import"\)\)[\s\S]*?return ParseImportDecl\(parser, Visibility::Internal, attrs\.elem\);[\s\S]*?if \(IsKw\(cur, "import"\)\)[\s\S]*?return ParseImportDecl\(cur, vis\.elem, attrs\.elem\);'
        }
    )

    foreach ($check in $checks) {
        $fullPath = Join-Path $workspaceRoot $check.Path
        if (-not (Test-Path $fullPath)) {
            throw "Case 'issue33_import_parse_attr_list_conformance' missing file: $fullPath"
        }
        $text = Get-Content -Path $fullPath -Raw
        if ($text -notmatch $check.Pattern) {
            throw "Case 'issue33_import_parse_attr_list_conformance' missing expected pattern '$($check.Pattern)' in $fullPath."
        }
    }

    Write-Host "[compiler-static] issue33_import_parse_attr_list_conformance: checks=$($checks.Count)"
}

function Invoke-Issue619ImportDeclSurfaceConformanceCase {
    $requiredChecks = @(
        @{
            Path = "docs\\CursiveSpecification.md"
            Pattern = 'ImportDecl = ⟨attrs_opt, vis, path, alias_opt, span, doc⟩'
        },
        @{
            Path = "cursive\\src\\02_source\\ast\\nodes\\ast_items.h"
            Pattern = 'struct ImportDecl[\s\S]*?AttrOpt attrs_opt;[\s\S]*?Visibility vis;[\s\S]*?Path path;[\s\S]*?std::optional<Identifier> alias_opt;[\s\S]*?core::Span span;[\s\S]*?DocList doc;'
        },
        @{
            Path = "cursive\\src\\02_source\\ast\\nodes\\ast_items.h"
            Pattern = 'inline const AttributeList& AttrListOf\(const ImportDecl& item\)\s*\{\s*return AttrListOf\(item\.attrs_opt\);\s*\}'
        },
        @{
            Path = "cursive\\src\\02_source\\parser\\item\\import_decl.cpp"
            Pattern = 'ParseItemResult ParseImportDecl\(Parser parser, Visibility vis,\s*AttrOpt attrs_opt\)\s*\{[\s\S]*?decl\.attrs_opt = std::move\(attrs_opt\);[\s\S]*?decl\.alias_opt = alias\.elem;'
        },
        @{
            Path = "cursive\\src\\02_source\\parser\\item\\parse_item.cpp"
            Pattern = 'if \(IsKw\(parser, "import"\)\)\s*\{[\s\S]*?return ParseImportDecl\(parser, Visibility::Internal, attrs\.elem\);[\s\S]*?if \(IsKw\(cur, "import"\)\)\s*\{[\s\S]*?return ParseImportDecl\(cur, vis\.elem, attrs\.elem\);'
        },
        @{
            Path = "cursive\\src\\04_analysis\\typing\\item\\import_decl.cpp"
            Pattern = 'ValidateUnsupportedAttributeTarget\(ast::AttrListOf\(decl\.attrs_opt\),\s*"import declarations"\)'
        },
        @{
            Path = "cursive\\src\\04_analysis\\typing\\item\\import_decl.cpp"
            Pattern = 'decl\.alias_opt\.has_value\(\)'
        },
        @{
            Path = "cursive\\src\\04_analysis\\resolve\\collect_toplevel.cpp"
            Pattern = 'node\.alias_opt\.value_or'
        },
        @{
            Path = "cursive\\src\\04_analysis\\resolve\\resolve_imports.cpp"
            Pattern = 'if \(import\.alias_opt\)'
        }
    )
    $forbiddenChecks = @(
        @{
            Path = "cursive\\src\\02_source\\ast\\nodes\\ast_items.h"
            Pattern = 'std::vector<Identifier>\s+items;'
        },
        @{
            Path = "cursive\\src\\04_analysis\\resolve\\resolve_imports.cpp"
            Pattern = 'import\.items'
        }
    )

    foreach ($check in $requiredChecks) {
        $fullPath = Join-Path $workspaceRoot $check.Path
        if (-not (Test-Path $fullPath)) {
            throw "Case 'issue619_import_decl_surface_conformance' missing file: $fullPath"
        }
        $text = Get-Content -Path $fullPath -Raw
        if ($text -notmatch $check.Pattern) {
            throw "Case 'issue619_import_decl_surface_conformance' missing expected pattern '$($check.Pattern)' in $fullPath."
        }
    }

    foreach ($check in $forbiddenChecks) {
        $fullPath = Join-Path $workspaceRoot $check.Path
        if (-not (Test-Path $fullPath)) {
            throw "Case 'issue619_import_decl_surface_conformance' missing file: $fullPath"
        }
        $text = Get-Content -Path $fullPath -Raw
        if ($text -match $check.Pattern) {
            throw "Case 'issue619_import_decl_surface_conformance' found forbidden pattern '$($check.Pattern)' in $fullPath."
        }
    }

    Write-Host "[compiler-static] issue619_import_decl_surface_conformance: required=$($requiredChecks.Count) forbidden=$($forbiddenChecks.Count)"
}

function Invoke-Issue622UsingDeclAttributeListConformanceCase {
    $requiredChecks = @(
        @{
            Path = "docs\\CursiveSpecification.md"
            Pattern = 'using_decl\s+::=\s+attribute_list\?\s+visibility\?\s+"using"\s+using_clause'
        },
        @{
            Path = "docs\\CursiveSpecification.md"
            Pattern = 'attribute_list ::= attribute\+'
        },
        @{
            Path = "docs\\CursiveSpecification.md"
            Pattern = '\*\*\(Parse-Using-List\)\*\*[\s\S]*?Γ ⊢ ParseAttrListOpt\(P\) ⇓ \(P_0, attrs_opt\)[\s\S]*?Γ ⊢ ParseItem\(P\) ⇓ \(P_3, ⟨UsingDecl, attrs_opt, vis, ⟨UsingList, mp, specs⟩, SpanBetween\(P, P_3\), \[\]⟩\)'
        },
        @{
            Path = "cursive\\src\\02_source\\parser\\item\\attribute_list.cpp"
            Pattern = 'ParseElemResult<AttributeList>\s+ParseAttrListTail\(Parser parser, AttributeList xs\)\s*\{[\s\S]*?if \(!IsAttrStart\(parser\)\)[\s\S]*?ParseElemResult<std::vector<AttributeItem>> block = ParseAttrBlock\(parser\);[\s\S]*?xs\.insert\(xs\.end\(\), block\.elem\.begin\(\), block\.elem\.end\(\)\);[\s\S]*?return ParseAttrListTail\(block\.parser, std::move\(xs\)\);'
        },
        @{
            Path = "cursive\\src\\02_source\\parser\\item\\parse_item.cpp"
            Pattern = 'ParseElemResult<AttrOpt> attrs = ParseAttributeListOpt\(parser\);[\s\S]*?AttributeList attrs_list = attrs\.elem\.value_or\(AttributeList\{\}\);[\s\S]*?if \(IsKw\(cur, "using"\)\)\s*\{[\s\S]*?return ParseUsingDecl\(start,\s*cur,\s*vis\.elem,\s*attrs\.elem\);'
        },
        @{
            Path = "cursive\\src\\02_source\\parser\\item\\using_decl.cpp"
            Pattern = 'ParseItemResult ParseUsingDecl\(Parser item_start,\s*Parser parser,\s*Visibility vis,\s*AttrOpt attrs_opt\)\s*\{[\s\S]*?if \(IsPunc\(after_colons, "\{"\)\) \{[\s\S]*?SPEC_RULE\("Parse-Using-List"\)[\s\S]*?decl\.attrs_opt = std::move\(attrs_opt\);[\s\S]*?decl\.clause = UsingList\{module_path\.elem,\s*std::move\(specs\.elem\)\};[\s\S]*?decl\.span = SpanBetween\(item_start,\s*specs\.parser\);'
        }
    )

    foreach ($check in $requiredChecks) {
        $fullPath = Join-Path $workspaceRoot $check.Path
        if (-not (Test-Path $fullPath)) {
            throw "Case 'issue622_using_decl_attribute_list_conformance' missing file: $fullPath"
        }
        $text = Get-Content -Path $fullPath -Raw
        if ($text -notmatch $check.Pattern) {
            throw "Case 'issue622_using_decl_attribute_list_conformance' missing expected pattern '$($check.Pattern)' in $fullPath."
        }
    }

    $parseItemPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\item\\parse_item.cpp"
    $parseItemText = Get-Content -Path $parseItemPath -Raw
    $parseAttrListOptCount = ([regex]::Matches($parseItemText, 'ParseAttributeListOpt\(parser\)')).Count
    if ($parseAttrListOptCount -ne 1) {
        throw "Case 'issue622_using_decl_attribute_list_conformance' expected a single ParseAttributeListOpt(parser) call in parse_item.cpp, observed $parseAttrListOptCount."
    }

    $usingDeclPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\item\\using_decl.cpp"
    $usingDeclText = Get-Content -Path $usingDeclPath -Raw
    if ($usingDeclText -match 'ParseAttributeListOpt\s*\(' -or $usingDeclText -match 'ParseAttrList\s*\(') {
        throw "Case 'issue622_using_decl_attribute_list_conformance' found local attribute-list reparsing in using_decl.cpp."
    }

    Write-Host "[compiler-static] issue622_using_decl_attribute_list_conformance: checks=$($requiredChecks.Count) parse_attr_list_opt_calls=$parseAttrListOptCount local_reparse=0"
}

function Invoke-Issue623UsingItemSurfaceConformanceCase {
    $requiredChecks = @(
        @{
            Path = "docs\\CursiveSpecification.md"
            Pattern = '\*\*\(Parse-Using-Item\)\*\*[\s\S]*?Γ ⊢ ParseAttrListOpt\(P\) ⇓ \(P_0, attrs_opt\)[\s\S]*?Γ ⊢ ParseVis\(P_0\) ⇓ \(P_1, vis\)[\s\S]*?IsKw\(Tok\(P_1\), `using`\)[\s\S]*?Γ ⊢ ParseModulePath\(Advance\(P_1\)\) ⇓ \(P_2, mp\)[\s\S]*?IsOp\(Tok\(P_2\), `::`\)[\s\S]*?IsIdent\(Tok\(Advance\(P_2\)\)\)[\s\S]*?Γ ⊢ ParseIdent\(Advance\(P_2\)\) ⇓ \(P_3, id\)[\s\S]*?Γ ⊢ ParseAliasOpt\(P_3\) ⇓ \(P_4, alias_opt\)[\s\S]*?Γ ⊢ ParseItem\(P\) ⇓ \(P_4, ⟨UsingDecl, attrs_opt, vis, ⟨UsingItem, mp, id, alias_opt⟩, SpanBetween\(P, P_4\), \[\]⟩\)'
        },
        @{
            Path = "cursive\\src\\02_source\\parser\\item\\parse_item.cpp"
            Pattern = 'ParseElemResult<AttrOpt> attrs = ParseAttributeListOpt\(parser\);[\s\S]*?AttributeList attrs_list = attrs\.elem\.value_or\(AttributeList\{\}\);[\s\S]*?if \(IsKw\(cur, "using"\)\)\s*\{[\s\S]*?return ParseUsingDecl\(start,\s*cur,\s*vis\.elem,\s*attrs\.elem\);'
        },
        @{
            Path = "cursive\\src\\02_source\\parser\\item\\using_decl.cpp"
            Pattern = 'ParseItemResult ParseUsingDecl\(Parser item_start,\s*Parser parser,\s*Visibility vis,\s*AttrOpt attrs_opt\)\s*\{[\s\S]*?SPEC_RULE\("Parse-Using-Item"\)[\s\S]*?ParseElemResult<Identifier> name = ParseIdent\(after_colons\);[\s\S]*?ParseElemResult<std::optional<Identifier>> alias = ParseAliasOpt\(name\.parser\);[\s\S]*?decl\.attrs_opt = std::move\(attrs_opt\);[\s\S]*?decl\.vis = vis;[\s\S]*?decl\.clause = UsingItem\{\s*module_path\.elem,\s*name\.elem,\s*alias\.elem,\s*\};[\s\S]*?decl\.span = SpanBetween\(item_start,\s*alias\.parser\);'
        }
    )

    foreach ($check in $requiredChecks) {
        $fullPath = Join-Path $workspaceRoot $check.Path
        if (-not (Test-Path $fullPath)) {
            throw "Case 'issue623_using_item_surface_conformance' missing file: $fullPath"
        }
        $text = Get-Content -Path $fullPath -Raw
        if ($text -notmatch $check.Pattern) {
            throw "Case 'issue623_using_item_surface_conformance' missing expected pattern '$($check.Pattern)' in $fullPath."
        }
    }

    Write-Host "[compiler-static] issue623_using_item_surface_conformance: checks=$($requiredChecks.Count)"
}

function Invoke-Issue624UsingDeclOptionalAttrSurfaceConformanceCase {
    $checks = @(
        @{
            Path = "docs\\CursiveSpecification.md"
            Pattern = 'UsingDecl = ⟨attrs_opt, vis, clause, span, doc⟩'
        },
        @{
            Path = "cursive\\src\\02_source\\ast\\nodes\\ast_items.h"
            Pattern = 'struct UsingDecl[\s\S]*?AttrOpt attrs_opt;[\s\S]*?Visibility vis;[\s\S]*?UsingClause clause;[\s\S]*?core::Span span;[\s\S]*?DocList doc;'
        },
        @{
            Path = "cursive\\src\\02_source\\ast\\nodes\\ast_items.h"
            Pattern = 'inline const AttributeList& AttrListOf\(const UsingDecl& item\)\s*\{\s*return AttrListOf\(item\.attrs_opt\);\s*\}'
        },
        @{
            Path = "cursive\\src\\02_source\\parser\\item\\parse_item.cpp"
            Pattern = 'if \(IsKw\(cur, "using"\)\)\s*\{\s*return ParseUsingDecl\(start,\s*cur,\s*vis\.elem,\s*attrs\.elem\);\s*\}'
        },
        @{
            Path = "cursive\\src\\02_source\\parser\\item\\using_decl.cpp"
            Pattern = 'ParseItemResult ParseUsingDecl\(Parser item_start,\s*Parser parser,\s*Visibility vis,\s*AttrOpt attrs_opt\)\s*\{[\s\S]*?decl\.attrs_opt = std::move\(attrs_opt\);'
        },
        @{
            Path = "cursive\\src\\04_analysis\\typing\\item\\using_decl.cpp"
            Pattern = 'ValidateUnsupportedAttributeTarget\(ast::AttrListOf\(decl\.attrs_opt\),\s*"using declarations"\)'
        }
    )

    foreach ($check in $checks) {
        $fullPath = Join-Path $workspaceRoot $check.Path
        if (-not (Test-Path $fullPath)) {
            throw "Case 'issue624_using_decl_optional_attr_surface_conformance' missing file: $fullPath"
        }
        $text = Get-Content -Path $fullPath -Raw
        if ($text -notmatch $check.Pattern) {
            throw "Case 'issue624_using_decl_optional_attr_surface_conformance' missing expected pattern '$($check.Pattern)' in $fullPath."
        }
    }

    Write-Host "[compiler-static] issue624_using_decl_optional_attr_surface_conformance: checks=$($checks.Count)"
}

function Invoke-Issue625StaticDeclParseAttrListConformanceCase {
    $requiredChecks = @(
        @{
            Path = "docs\\CursiveSpecification.md"
            Pattern = 'static_decl\s+::=\s+attribute_list\?\s+visibility\?\s+\("let"\s+\|\s+"var"\)\s+binding_decl'
        },
        @{
            Path = "docs\\CursiveSpecification.md"
            Pattern = '\*\*\(Parse-AttrListTail-Cons\)\*\*[\s\S]*?Γ ⊢ ParseAttrListTail\(P, attrs\) ⇓ \(P_2, attrs_1\)'
        },
        @{
            Path = "docs\\CursiveSpecification.md"
            Pattern = '\*\*\(Parse-Static-Decl\)\*\*[\s\S]*?Γ ⊢ ParseAttrListOpt\(P\) ⇓ \(P_0, attrs_opt\)[\s\S]*?Γ ⊢ ParseVis\(P_0\) ⇓ \(P_1, vis\)[\s\S]*?Tok\(P_1\) = Keyword\(kw\)[\s\S]*?kw ∈ \{`let`, `var`\}[\s\S]*?Γ ⊢ ParseBindingAfterLetVar\(P_1\) ⇓ \(P_2, bind\)[\s\S]*?Γ ⊢ ParseItem\(P\) ⇓ \(P_2, ⟨StaticDecl, attrs_opt, vis, mut, bind, SpanBetween\(P, P_2\), \[\]⟩\)'
        },
        @{
            Path = "cursive\\src\\02_source\\parser\\item\\attribute_list.cpp"
            Pattern = 'ParseElemResult<AttributeList>\s+ParseAttrListTail\(Parser parser, AttributeList xs\)\s*\{[\s\S]*?if \(!IsAttrStart\(parser\)\)[\s\S]*?ParseElemResult<std::vector<AttributeItem>> block = ParseAttrBlock\(parser\);[\s\S]*?xs\.insert\(xs\.end\(\), block\.elem\.begin\(\), block\.elem\.end\(\)\);[\s\S]*?return ParseAttrListTail\(block\.parser, std::move\(xs\)\);'
        },
        @{
            Path = "cursive\\src\\02_source\\parser\\item\\parse_item.cpp"
            Pattern = 'ParseElemResult<AttrOpt>\s+attrs = ParseAttributeListOpt\(parser\);[\s\S]*?AttributeList attrs_list = attrs\.elem\.value_or\(AttributeList\{\}\);[\s\S]*?if \(IsKw\(cur, "let"\) \|\| IsKw\(cur, "var"\)\)\s*\{[\s\S]*?SPEC_RULE\("Parse-Static-Decl"\);[\s\S]*?return ParseStaticDecl\(cur, vis\.elem, attrs\.elem\);'
        },
        @{
            Path = "cursive\\src\\02_source\\parser\\item\\static_decl.cpp"
            Pattern = 'ParseItemResult ParseStaticDecl\(Parser parser, Visibility vis,\s*AttrOpt attrs_opt\)\s*\{[\s\S]*?SPEC_RULE\("Parse-Static-Decl"\);[\s\S]*?ParseElemResult<Binding> binding = ParseBindingAfterLetVar\(parser\);[\s\S]*?decl\.attrs_opt = std::move\(attrs_opt\);[\s\S]*?decl\.mut = mut;[\s\S]*?decl\.binding = binding\.elem;'
        }
    )

    foreach ($check in $requiredChecks) {
        $fullPath = Join-Path $workspaceRoot $check.Path
        if (-not (Test-Path $fullPath)) {
            throw "Case 'issue625_static_decl_parse_attr_list_conformance' missing file: $fullPath"
        }
        $text = Get-Content -Path $fullPath -Raw
        if ($text -notmatch $check.Pattern) {
            throw "Case 'issue625_static_decl_parse_attr_list_conformance' missing expected pattern '$($check.Pattern)' in $fullPath."
        }
    }

    $parseItemPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\item\\parse_item.cpp"
    $parseItemText = Get-Content -Path $parseItemPath -Raw
    $parseAttrListOptCount = ([regex]::Matches($parseItemText, 'ParseAttributeListOpt\(parser\)')).Count
    if ($parseAttrListOptCount -ne 1) {
        throw "Case 'issue625_static_decl_parse_attr_list_conformance' expected a single ParseAttributeListOpt(parser) call in parse_item.cpp, observed $parseAttrListOptCount."
    }

    $staticDeclPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\item\\static_decl.cpp"
    $staticDeclText = Get-Content -Path $staticDeclPath -Raw
    if ($staticDeclText -match 'ParseAttributeListOpt\s*\(' -or $staticDeclText -match 'ParseAttrList\s*\(') {
        throw "Case 'issue625_static_decl_parse_attr_list_conformance' found local attribute-list reparsing in static_decl.cpp."
    }

    Write-Host "[compiler-static] issue625_static_decl_parse_attr_list_conformance: checks=$($requiredChecks.Count) parse_attr_list_opt_calls=$parseAttrListOptCount local_reparse=0"
}

function Invoke-Issue627ExternBlockShellConformanceCase {
    $requiredChecks = @(
        @{
            Path = "docs\\CursiveSpecification.md"
            Pattern = 'extern_block\s+::=\s+attribute_list\?\s+visibility\?\s+"extern"\s+extern_abi\?\s+"\{"\s+extern_item\*\s+"\}"'
        },
        @{
            Path = "docs\\CursiveSpecification.md"
            Pattern = '\*\*\(Parse-ExternBlock\)\*\*[\s\S]*?Γ ⊢ ParseAttrListOpt\(P\) ⇓ \(P_0, attrs_opt\)[\s\S]*?Γ ⊢ ParseVis\(P_0\) ⇓ \(P_1, vis\)[\s\S]*?IsKw\(Tok\(P_1\), `extern`\)[\s\S]*?Γ ⊢ ParseExternAbiOpt\(Advance\(P_1\)\) ⇓ \(P_2, abi_opt\)[\s\S]*?Γ ⊢ ParseItem\(P\) ⇓ \(Advance\(P_3\), ⟨ExternBlock, attrs_opt, vis, abi_opt, items, SpanBetween\(P, Advance\(P_3\)\), \[\]⟩\)'
        },
        @{
            Path = "cursive\\src\\02_source\\parser\\item\\parse_item.cpp"
            Pattern = 'ParseElemResult<AttrOpt>\s+attrs = ParseAttributeListOpt\(parser\);[\s\S]*?ParseElemResult<Visibility>\s+vis = ParseVis\(parser\);[\s\S]*?if \(IsKw\(cur, "extern"\)\)\s*\{[\s\S]*?SPEC_RULE\("Parse-Extern-Block"\);[\s\S]*?return ParseExternBlock\(cur, vis\.elem, attrs_list\);'
        },
        @{
            Path = "cursive\\src\\02_source\\parser\\item\\extern_block.cpp"
            Pattern = 'ParseItemResult ParseExternBlock\(Parser parser, Visibility vis,\s*AttributeList attrs\)\s*\{[\s\S]*?SPEC_RULE\("Parse-Extern-Block"\);[\s\S]*?if \(!IsKw\(parser, "extern"\)\)\s*\{[\s\S]*?EmitParseSyntaxErr\(parser, TokSpan\(parser\)\);[\s\S]*?SyncItem\(parser\);[\s\S]*?return \{parser, ErrorItem\{SpanBetween\(start, parser\), \{\}\}\};[\s\S]*?\}[\s\S]*?Advance\(parser\);'
        }
    )

    foreach ($check in $requiredChecks) {
        $fullPath = Join-Path $workspaceRoot $check.Path
        if (-not (Test-Path $fullPath)) {
            throw "Case 'issue627_extern_block_shell_conformance' missing file: $fullPath"
        }
        $text = Get-Content -Path $fullPath -Raw
        if ($text -notmatch $check.Pattern) {
            throw "Case 'issue627_extern_block_shell_conformance' missing expected pattern '$($check.Pattern)' in $fullPath."
        }
    }

    $parseItemPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\item\\parse_item.cpp"
    $parseItemText = Get-Content -Path $parseItemPath -Raw
    if ($parseItemText -match 'tok && IsIdentTok\(\*tok\) && tok->lexeme == "extern"') {
        throw "Case 'issue627_extern_block_shell_conformance' found legacy identifier-based extern dispatch in parse_item.cpp."
    }

    Write-Host "[compiler-static] issue627_extern_block_shell_conformance: checks=$($requiredChecks.Count) identifier_dispatch=0"
}

function Invoke-Issue628PathStringSurfaceConformanceCase {
    $requiredChecks = @(
        @{
            Path = "docs\\CursiveSpecification.md"
            Pattern = 'PathString\(p\)\s*=\s*StringOfPath\(p\)'
        },
        @{
            Path = "cursive\\include\\00_core\\symbols.h"
            Pattern = 'std::string\s+PathString\s*\(\s*const\s+std::vector<std::string>&\s+comps\s*\);'
        },
        @{
            Path = "cursive\\include\\00_core\\symbols.h"
            Pattern = 'std::string\s+PathString\s*\(\s*std::initializer_list<std::string_view>\s+comps\s*\);'
        },
        @{
            Path = "cursive\\src\\00_core\\symbols.cpp"
            Pattern = 'void\s+SpecDefsPathStrings\(\)\s*\{\s*SPEC_DEF\("StringOfPath",\s*"3\.4\.1"\);\s*SPEC_DEF\("StringOfPathRef",\s*"3\.4\.1"\);\s*SPEC_DEF\("PathString",\s*"11\.5\.3"\);\s*\}'
        },
        @{
            Path = "cursive\\src\\00_core\\symbols.cpp"
            Pattern = 'std::string\s+PathString\s*\(\s*const\s+std::vector<std::string>&\s+comps\s*\)\s*\{\s*SpecDefsPathStrings\(\);\s*return\s+StringOfPath\(comps\);\s*\}'
        },
        @{
            Path = "cursive\\src\\00_core\\symbols.cpp"
            Pattern = 'std::string\s+PathString\s*\(\s*std::initializer_list<std::string_view>\s+comps\s*\)\s*\{\s*SpecDefsPathStrings\(\);\s*return\s+StringOfPath\(comps\);\s*\}'
        },
        @{
            Path = "cursive\\src\\00_core\\symbols.cpp"
            Pattern = 'std::string\s+PathSig\s*\(\s*std::initializer_list<std::string_view>\s+comps\s*\)\s*\{\s*return\s+Mangle\(PathString\(comps\)\);\s*\}'
        },
        @{
            Path = "cursive\\src\\00_core\\symbols.cpp"
            Pattern = 'std::string\s+MangleModulePath\s*\(\s*std::string_view\s+module_path\s*\)\s*\{[\s\S]*?return\s+Mangle\(PathString\(parts\)\);\s*\}'
        }
    )

    foreach ($check in $requiredChecks) {
        $fullPath = Join-Path $workspaceRoot $check.Path
        if (-not (Test-Path $fullPath)) {
            throw "Case 'issue628_path_string_surface_conformance' missing file: $fullPath"
        }
        $text = Get-Content -Path $fullPath -Raw
        if ($text -notmatch $check.Pattern) {
            throw "Case 'issue628_path_string_surface_conformance' missing expected pattern '$($check.Pattern)' in $fullPath."
        }
    }

    $symbolsPath = Join-Path $workspaceRoot "cursive\\src\\00_core\\symbols.cpp"
    $symbolsText = Get-Content -Path $symbolsPath -Raw
    foreach ($forbidden in @(
        'return\s+Mangle\(StringOfPath\(comps\)\);',
        'return\s+Mangle\(StringOfPath\(parts\)\);'
    )) {
        if ($symbolsText -match $forbidden) {
            throw "Case 'issue628_path_string_surface_conformance' found stale direct StringOfPath mangling pattern '$forbidden' in $symbolsPath."
        }
    }

    Write-Host "[compiler-static] issue628_path_string_surface_conformance: checks=$($requiredChecks.Count) forbidden=2"
}

function Invoke-Issue629StringOfPathRefSurfaceConformanceCase {
    $requiredChecks = @(
        @{
            Path = "docs\\CursiveSpecification.md"
            Pattern = 'StringOfPathRef\s*=\s*\{"3\.4\.1"\}'
        },
        @{
            Path = "cursive\\src\\00_core\\symbols.cpp"
            Pattern = 'void\s+SpecDefsPathStrings\(\)\s*\{\s*SPEC_DEF\("StringOfPath",\s*"3\.4\.1"\);\s*SPEC_DEF\("StringOfPathRef",\s*"3\.4\.1"\);\s*SPEC_DEF\("PathString",\s*"11\.5\.3"\);\s*\}'
        },
        @{
            Path = "cursive\\src\\00_core\\symbols.cpp"
            Pattern = 'std::string\s+StringOfPath\s*\(\s*const\s+std::vector<std::string>&\s+comps\s*\)\s*\{\s*SpecDefsPathStrings\(\);\s*return\s+JoinWithDoubleColon\(comps\);\s*\}'
        },
        @{
            Path = "cursive\\src\\00_core\\symbols.cpp"
            Pattern = 'std::string\s+StringOfPath\s*\(\s*std::initializer_list<std::string_view>\s+comps\s*\)\s*\{\s*SpecDefsPathStrings\(\);'
        }
    )

    foreach ($check in $requiredChecks) {
        $fullPath = Join-Path $workspaceRoot $check.Path
        if (-not (Test-Path $fullPath)) {
            throw "Case 'issue629_string_of_path_ref_surface_conformance' missing file: $fullPath"
        }
        $text = Get-Content -Path $fullPath -Raw
        if ($text -notmatch $check.Pattern) {
            throw "Case 'issue629_string_of_path_ref_surface_conformance' missing expected pattern '$($check.Pattern)' in $fullPath."
        }
    }

    Write-Host "[compiler-static] issue629_string_of_path_ref_surface_conformance: checks=$($requiredChecks.Count)"
}

function Invoke-Issue626StaticDeclOptionalAttrSurfaceConformanceCase {
    $checks = @(
        @{
            Path = "docs\\CursiveSpecification.md"
            Pattern = 'StaticDecl = ⟨attrs_opt, vis, mut, binding, span, doc⟩'
        },
        @{
            Path = "cursive\\src\\02_source\\ast\\nodes\\ast_items.h"
            Pattern = 'struct StaticDecl[\s\S]*?AttrOpt attrs_opt;[\s\S]*?Visibility vis;[\s\S]*?Mutability mut;[\s\S]*?Binding binding;[\s\S]*?core::Span span;[\s\S]*?DocList doc;'
        },
        @{
            Path = "cursive\\src\\02_source\\ast\\nodes\\ast_items.h"
            Pattern = 'inline const AttributeList& AttrListOf\(const StaticDecl& item\)\s*\{\s*return AttrListOf\(item\.attrs_opt\);\s*\}'
        },
        @{
            Path = "cursive\\src\\02_source\\parser\\item\\parse_item.cpp"
            Pattern = 'if \(IsKw\(cur, "let"\) \|\| IsKw\(cur, "var"\)\)\s*\{[\s\S]*?SPEC_RULE\("Parse-Static-Decl"\);[\s\S]*?return ParseStaticDecl\(cur, vis\.elem, attrs\.elem\);'
        },
        @{
            Path = "cursive\\src\\02_source\\parser\\item\\static_decl.cpp"
            Pattern = 'ParseItemResult ParseStaticDecl\(Parser parser, Visibility vis,\s*AttrOpt attrs_opt\)\s*\{[\s\S]*?decl\.attrs_opt = std::move\(attrs_opt\);'
        },
        @{
            Path = "cursive\\src\\04_analysis\\typing\\item\\static_decl.cpp"
            Pattern = 'ValidateUnsupportedAttributeTarget\(ast::AttrListOf\(decl\.attrs_opt\),\s*"static declarations"\)'
        }
    )

    foreach ($check in $checks) {
        $fullPath = Join-Path $workspaceRoot $check.Path
        if (-not (Test-Path $fullPath)) {
            throw "Case 'issue626_static_decl_optional_attr_surface_conformance' missing file: $fullPath"
        }
        $text = Get-Content -Path $fullPath -Raw
        if ($text -notmatch $check.Pattern) {
            throw "Case 'issue626_static_decl_optional_attr_surface_conformance' missing expected pattern '$($check.Pattern)' in $fullPath."
        }
    }

    Write-Host "[compiler-static] issue626_static_decl_optional_attr_surface_conformance: checks=$($checks.Count)"
}

function Invoke-Issue630ModalRefSurfaceConformanceCase {
    $checks = @(
        @{
            Path = "cursive\\src\\02_source\\ast\\ast_common.h"
            Pattern = 'using ModalRef = std::variant<TypePath, GenericTypeRef>;'
        },
        @{
            Path = "cursive\\src\\02_source\\ast\\ast_common.h"
            Pattern = 'struct ModalStateRef\s*\{[\s\S]*?ModalRef modal_ref;[\s\S]*?TypePath path;[\s\S]*?std::vector<TypePtr> generic_args;[\s\S]*?Identifier state;'
        },
        @{
            Path = "cursive\\src\\02_source\\ast\\ast_common.h"
            Pattern = 'inline void SyncModalStateRefFromFields\(ModalStateRef& ref\)\s*\{\s*ref\.modal_ref = MakeModalRef\(ref\.path, ref\.generic_args\);\s*\}'
        },
        @{
            Path = "cursive\\src\\02_source\\ast\\nodes\\ast_types.h"
            Pattern = 'using TypeModalRef = std::variant<TypePathType, TypeApply>;'
        },
        @{
            Path = "cursive\\src\\02_source\\ast\\nodes\\ast_types.h"
            Pattern = 'struct TypeModalState\s*\{[\s\S]*?TypeModalRef modal_ref;[\s\S]*?TypePath path;[\s\S]*?std::vector<std::shared_ptr<Type>> generic_args;[\s\S]*?Identifier state;'
        },
        @{
            Path = "cursive\\src\\02_source\\ast\\nodes\\ast_types.h"
            Pattern = 'inline void SyncTypeModalStateFromFields\(TypeModalState& state\)\s*\{\s*state\.modal_ref = MakeTypeModalRef\(state\.path, state\.generic_args\);\s*\}'
        },
        @{
            Path = "cursive\\src\\02_source\\parser\\type\\state_specific_type.cpp"
            Pattern = 'modal\.generic_args = std::move\(generic_args\);\s*SyncTypeModalStateFromFields\(modal\);\s*modal\.state = state\.elem;'
        },
        @{
            Path = "cursive\\src\\02_source\\parser\\expr\\record_literal.cpp"
            Pattern = 'modal\.generic_args = std::move\(generic_args\);\s*SyncModalStateRefFromFields\(modal\);\s*modal\.state = state\.elem;'
        },
        @{
            Path = "cursive\\src\\02_source\\ast\\ast_dump.cpp"
            Pattern = 'dump_path\(out, TypeModalRefPath\(node\.modal_ref\)\);[\s\S]*?const auto& modal_args = TypeModalRefArgs\(node\.modal_ref\);'
        },
        @{
            Path = "cursive\\src\\04_analysis\\resolve\\resolve_types.cpp"
            Pattern = 'SyncTypeModalStateFromFields\(out_node\);'
        },
        @{
            Path = "cursive\\src\\04_analysis\\resolve\\resolve_expr.cpp"
            Pattern = 'SyncModalStateRefFromFields\(out\);'
        }
    )

    foreach ($check in $checks) {
        $fullPath = Join-Path $workspaceRoot $check.Path
        if (-not (Test-Path $fullPath)) {
            throw "Case 'issue630_modal_ref_surface_conformance' missing file: $fullPath"
        }
        $text = Get-Content -Path $fullPath -Raw
        if ($text -notmatch $check.Pattern) {
            throw "Case 'issue630_modal_ref_surface_conformance' missing expected pattern '$($check.Pattern)' in $fullPath."
        }
    }

    Write-Host "[compiler-static] issue630_modal_ref_surface_conformance: checks=$($checks.Count)"
}

function Invoke-Issue33AstTypeWiringCase {
    $checks = @(
        @{ Path = "cursive\\src\\02_source\\ast\\nodes\\ast_module.h"; Pattern = 'struct ASTModule\s*\{\s*Path path;\s*std::vector<ASTItem> items;\s*DocList module_doc;\s*\};' },
        @{ Path = "cursive\\src\\02_source\\ast\\nodes\\ast_module.h"; Pattern = 'struct ASTFile\s*\{\s*Path path;\s*std::vector<ASTItem> items;\s*DocList module_doc;\s*\};' },
        @{ Path = "cursive\\include\\02_source\\parser\\parser.h"; Pattern = 'struct ParseFileResult\s*\{\s*std::optional<ASTFile> file;\s*std::vector<core::Span> unsafe_spans;\s*core::DiagnosticStream diags;\s*\};' },
        @{ Path = "cursive\\src\\02_source\\parser\\parser.cpp"; Pattern = 'file\.path\.push_back\(source\.path\);' },
        @{ Path = "cursive\\src\\02_source\\parser\\parser.cpp"; Pattern = 'result\.unsafe_spans\s*=\s*std::move\(unsafe_spans\);' },
        @{ Path = "cursive\\src\\02_source\\parser\\parse_modules.cpp"; Pattern = 'result\.unsafe_spans_by_file\[load\.source->path\]\s*=\s*std::move\(parsed\.unsafe_spans\);' },
        @{ Path = "cursive\\include\\04_analysis\\typing\\context.h"; Pattern = 'std::unordered_map<std::string,\s*std::vector<core::Span>> unsafe_spans_by_file;' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\type_expr.cpp"; Pattern = 'const auto file_it = ctx\.sigma\.unsafe_spans_by_file\.find\(span\.file\);' },
        @{ Path = "cursive\\src\\02_source\\ast\\nodes\\ast_items.h"; Pattern = 'struct StateFieldDecl[\s\S]*?AttributeList attrs;' },
        @{ Path = "cursive\\src\\02_source\\ast\\nodes\\ast_items.h"; Pattern = 'struct StateMethodDecl[\s\S]*?AttributeList attrs;' },
        @{ Path = "cursive\\src\\02_source\\ast\\nodes\\ast_items.h"; Pattern = 'struct StateMethodDecl[\s\S]*?std::optional<GenericParams>\s+generic_params;' },
        @{ Path = "cursive\\src\\02_source\\ast\\nodes\\ast_items.h"; Pattern = 'struct StateMethodDecl[\s\S]*?std::optional<ContractClause>\s+contract;' },
        @{ Path = "cursive\\src\\02_source\\ast\\nodes\\ast_items.h"; Pattern = 'struct TransitionDecl[\s\S]*?AttributeList attrs;' },
        @{ Path = "cursive\\src\\02_source\\ast\\nodes\\ast_items.h"; Pattern = 'struct ModalDecl[\s\S]*?AttributeList attrs;' },
        @{ Path = "cursive\\include\\02_source\\ast\\ast_utils.h"; Pattern = 'std::vector<std::string>\s+state_recv_perms\(const std::vector<StateBlock>& states\);' },
        @{ Path = "cursive\\src\\02_source\\ast\\nodes\\ast_nodes.cpp"; Pattern = 'std::vector<std::string>\s+state_recv_perms\(const std::vector<StateBlock>& states\)\s*\{' },
        @{ Path = "cursive\\src\\02_source\\ast\\nodes\\ast_nodes.cpp"; Pattern = 'for \(const auto& state : states\)\s*\{[\s\S]*?for \(const auto& member : state\.members\)\s*\{[\s\S]*?const auto\* method = std::get_if<StateMethodDecl>\(&member\);[\s\S]*?InsertReceiverShorthandPerm\(method->receiver, out\);' },
        @{ Path = "cursive\\src\\02_source\\parser\\item\\modal_decl.cpp"; Pattern = 'method\.attrs\s*=\s*std::move\(attrs\.elem\);' },
        @{ Path = "cursive\\src\\02_source\\parser\\item\\modal_decl.cpp"; Pattern = 'method\.generic_params\s*=\s*gen_params\.elem;' },
        @{ Path = "cursive\\src\\02_source\\parser\\item\\modal_decl.cpp"; Pattern = 'method\.contract\s*=\s*contract\.elem;' },
        @{ Path = "cursive\\src\\02_source\\parser\\item\\modal_decl.cpp"; Pattern = 'field\.attrs\s*=\s*std::move\(attrs\.elem\);' },
        @{ Path = "cursive\\src\\02_source\\parser\\item\\modal_decl.cpp"; Pattern = 'trans\.attrs\s*=\s*std::move\(attrs\.elem\);' },
        @{ Path = "cursive\\src\\02_source\\parser\\item\\modal_decl.cpp"; Pattern = 'decl\.attrs\s*=\s*std::move\(attrs\);' },
        @{ Path = "cursive\\src\\02_source\\ast\\nodes\\ast_types.h"; Pattern = 'struct TypeApply' },
        @{ Path = "cursive\\src\\02_source\\ast\\nodes\\ast_types.h"; Pattern = 'struct TypeRange\b' },
        @{ Path = "cursive\\src\\02_source\\ast\\nodes\\ast_types.h"; Pattern = 'struct TypeRangeInclusive\b' },
        @{ Path = "cursive\\src\\02_source\\ast\\nodes\\ast_types.h"; Pattern = 'struct TypeRangeFrom\b' },
        @{ Path = "cursive\\src\\02_source\\ast\\nodes\\ast_types.h"; Pattern = 'struct TypeRangeTo\b' },
        @{ Path = "cursive\\src\\02_source\\ast\\nodes\\ast_types.h"; Pattern = 'struct TypeRangeToInclusive\b' },
        @{ Path = "cursive\\src\\02_source\\ast\\nodes\\ast_types.h"; Pattern = 'struct TypeRangeFull\s*\{\};' },
        @{ Path = "cursive\\src\\02_source\\ast\\nodes\\ast_types.h"; Pattern = 'using TypeNode = std::variant<[\s\S]*TypeApply' },
        @{ Path = "cursive\\src\\02_source\\ast\\nodes\\ast_types.h"; Pattern = 'using TypeNode = std::variant<[\s\S]*TypeRangeFull' },
        @{ Path = "cursive\\src\\02_source\\parser\\type\\type_path.cpp"; Pattern = 'SPEC_RULE\("Parse-Type-Apply"\)' },
        @{ Path = "cursive\\src\\02_source\\parser\\type\\type_path.cpp"; Pattern = 'TypeApply apply;' },
        @{ Path = "cursive\\src\\02_source\\parser\\type\\type_common.cpp"; Pattern = 'return TypeRange\{base\};' },
        @{ Path = "cursive\\src\\02_source\\parser\\type\\type_common.cpp"; Pattern = 'return TypeRangeInclusive\{base\};' },
        @{ Path = "cursive\\src\\02_source\\parser\\type\\type_common.cpp"; Pattern = 'return TypeRangeFrom\{base\};' },
        @{ Path = "cursive\\src\\02_source\\parser\\type\\type_common.cpp"; Pattern = 'return TypeRangeTo\{base\};' },
        @{ Path = "cursive\\src\\02_source\\parser\\type\\type_common.cpp"; Pattern = 'return TypeRangeToInclusive\{base\};' },
        @{ Path = "cursive\\src\\02_source\\parser\\type\\type_common.cpp"; Pattern = 'TypeRangeFull\{\}' },
        @{ Path = "cursive\\src\\02_source\\ast\\ast_dump.cpp"; Pattern = 'std::is_same_v<T,\s*TypeApply>' },
        @{ Path = "cursive\\src\\02_source\\ast\\ast_dump.cpp"; Pattern = 'std::is_same_v<T,\s*TypeRangeFull>' }
    )

    foreach ($check in $checks) {
        $fullPath = Join-Path $workspaceRoot $check.Path
        if (-not (Test-Path $fullPath)) {
            throw "Case 'issue33_ast_type_wiring' missing file: $fullPath"
        }
        $text = Get-Content -Path $fullPath -Raw
        if ($text -notmatch $check.Pattern) {
            throw "Case 'issue33_ast_type_wiring' missing expected pattern '$($check.Pattern)' in $fullPath."
        }
    }

    Write-Host "[compiler-static] issue33_ast_type_wiring: checks=$($checks.Count)"
}

function Invoke-Issue33ExprWiringCase {
    $checks = @(
        @{ Path = "cursive\\src\\02_source\\ast\\nodes\\ast_exprs.h"; Pattern = 'struct CallTypeArgsExpr[\s\S]*?std::vector<TypePtr>\s+type_args;' },
        @{ Path = "cursive\\src\\02_source\\ast\\nodes\\ast_exprs.h"; Pattern = 'using ExprNode = std::variant<[\s\S]*CallTypeArgsExpr' },
        @{ Path = "cursive\\src\\02_source\\parser\\expr\\call_type_args.cpp"; Pattern = 'bool CallTypeArgsStart\(Parser parser\)' },
        @{ Path = "cursive\\src\\02_source\\parser\\expr\\call_type_args.cpp"; Pattern = 'ParseElemResult<ExprPtr> ParseCallTypeArgsStep\(Parser parser, ExprPtr expr\)' },
        @{ Path = "cursive\\src\\02_source\\parser\\expr\\call_type_args.cpp"; Pattern = 'SPEC_RULE\("Postfix-Call-TypeArgs"\)' },
        @{ Path = "cursive\\src\\02_source\\parser\\expr\\call_type_args.cpp"; Pattern = 'CallTypeArgsExpr call;' },
        @{ Path = "cursive\\src\\02_source\\parser\\expr\\call_type_args.cpp"; Pattern = 'call\.type_args\s*=\s*parsed_targs->elem;' },
        @{ Path = "cursive\\src\\02_source\\parser\\expr\\postfix.cpp"; Pattern = 'if\s*\(CallTypeArgsStart\(parser\)\)\s*\{\s*return ParseCallTypeArgsStep\(parser, expr\);' },
        @{ Path = "cursive\\src\\02_source\\parser\\expr\\postfix.cpp"; Pattern = 'const bool postfix_start = CallTypeArgsStart\(parser\)' },
        @{ Path = "cursive\\src\\02_source\\parser\\stmt\\parse_stmt.cpp"; Pattern = 'ParseElemResult<ExprPtr> place = ParsePlace\(place_probe, true\);' },
        @{ Path = "cursive\\src\\02_source\\parser\\stmt\\parse_stmt.cpp"; Pattern = 'SPEC_RULE\("Parse-Assign-Stmt"\)' },
        @{ Path = "cursive\\src\\02_source\\parser\\expr\\primary.cpp"; Pattern = 'if\s*\(auto transmute = TryParseTransmuteExpr\(parser\)\)\s*\{' },
        @{ Path = "cursive\\src\\02_source\\parser\\expr\\primary.cpp"; Pattern = 'if\s*\(auto alloc = TryParseAllocExpr\(parser\)\)\s*\{' },
        @{ Path = "cursive\\src\\02_source\\parser\\expr\\transmute_expr.cpp"; Pattern = 'if\s*\(!IsOp\(next,\s*"<"\)\)' },
        @{ Path = "cursive\\src\\04_analysis\\resolve\\resolve_expr.cpp"; Pattern = 'std::is_same_v<T,\s*ast::CallTypeArgsExpr>' },
        @{ Path = "cursive\\src\\04_analysis\\resolve\\resolve_expr.cpp"; Pattern = 'SPEC_RULE\("ResolveExpr-CallTypeArgs"\)' },
        @{ Path = "cursive\\src\\04_analysis\\resolve\\visibility.cpp"; Pattern = 'std::is_same_v<T,\s*ast::CallTypeArgsExpr>' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\type_expr.cpp"; Pattern = 'std::is_same_v<T,\s*ast::CallTypeArgsExpr>' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\type_expr.cpp"; Pattern = 'TypeCallTypeArgsExprImpl\(ctx,\s*type_ctx,\s*node,\s*env\)' },
        @{ Path = "cursive\\include\\04_analysis\\typing\\expr\\call_type_args.h"; Pattern = 'const ast::CallTypeArgsExpr& expr' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\expr\\call_type_args.cpp"; Pattern = 'const ast::CallTypeArgsExpr& expr' },
        @{ Path = "cursive\\src\\CMakeLists.txt"; Pattern = '04_analysis/typing/expr/call_type_args\.cpp' }
    )

    foreach ($check in $checks) {
        $fullPath = Join-Path $workspaceRoot $check.Path
        if (-not (Test-Path $fullPath)) {
            throw "Case 'issue33_expr_wiring' missing file: $fullPath"
        }
        $text = Get-Content -Path $fullPath -Raw
        if ($text -notmatch $check.Pattern) {
            throw "Case 'issue33_expr_wiring' missing expected pattern '$($check.Pattern)' in $fullPath."
        }
    }

    $spawnPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\expr\\spawn_expr.cpp"
    $spawnText = Get-Content -Path $spawnPath -Raw
    $spawnRules = @(
        "Parse-SpawnOpt-Name",
        "Parse-SpawnOpt-Affinity",
        "Parse-SpawnOpt-Priority",
        "Parse-SpawnOptList-Empty",
        "Parse-SpawnOptList-Cons",
        "Parse-SpawnOptListTail-End",
        "Parse-SpawnOptListTail-TrailingComma",
        "Parse-SpawnOptListTail-Comma",
        "Parse-SpawnOptsOpt-None",
        "Parse-SpawnOptsOpt-Yes"
    )
    foreach ($rule in $spawnRules) {
        if ($spawnText -notmatch [regex]::Escape($rule)) {
            throw "Case 'issue33_expr_wiring' missing spawn helper rule '$rule' in $spawnPath."
        }
    }

    $dispatchPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\expr\\dispatch_expr.cpp"
    $dispatchText = Get-Content -Path $dispatchPath -Raw
    $dispatchRules = @(
        "Parse-KeyMode-Read",
        "Parse-KeyMode-Write",
        "Parse-KeyClauseOpt-None",
        "Parse-KeyClauseOpt-Yes",
        "Parse-DispatchOpt-Reduce",
        "Parse-DispatchOpt-Ordered",
        "Parse-DispatchOpt-Chunk",
        "Parse-DispatchOptList-Empty",
        "Parse-DispatchOptList-Cons",
        "Parse-DispatchOptListTail-End",
        "Parse-DispatchOptListTail-TrailingComma",
        "Parse-DispatchOptListTail-Comma",
        "Parse-DispatchOptsOpt-None",
        "Parse-DispatchOptsOpt-Yes"
    )
    foreach ($rule in $dispatchRules) {
        if ($dispatchText -notmatch [regex]::Escape($rule)) {
            throw "Case 'issue33_expr_wiring' missing dispatch helper rule '$rule' in $dispatchPath."
        }
    }

    $transmutePath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\expr\\transmute_expr.cpp"
    $transmuteText = Get-Content -Path $transmutePath -Raw
    if ($transmuteText -match 'transmute::') {
        throw "Case 'issue33_expr_wiring' found deprecated transmute::<...> parser shape in $transmutePath."
    }

    $totalChecks = $checks.Count + $spawnRules.Count + $dispatchRules.Count + 1
    Write-Host "[compiler-static] issue33_expr_wiring: checks=$totalChecks"
}

function Invoke-Issue33QualifiedNamePhaseBoundaryWiringCase {
    $checks = @(
        @{ Path = "cursive\\src\\02_source\\ast\\nodes\\ast_exprs.h"; Pattern = 'struct QualifiedNameExpr[\s\S]*?ModulePath path;' },
        @{ Path = "cursive\\src\\02_source\\ast\\nodes\\ast_exprs.h"; Pattern = 'struct PathExpr[\s\S]*?ModulePath path;' },
        @{ Path = "cursive\\src\\02_source\\parser\\expr\\qualified_apply.cpp"; Pattern = 'SPEC_RULE\("Parse-Qualified-Name"\)' },
        @{ Path = "cursive\\src\\02_source\\parser\\expr\\qualified_apply.cpp"; Pattern = 'QualifiedNameExpr qname;' },
        @{ Path = "cursive\\src\\04_analysis\\resolve\\resolve_qual.cpp"; Pattern = 'std::is_same_v<T,\s*ast::QualifiedNameExpr>' },
        @{ Path = "cursive\\src\\04_analysis\\resolve\\resolve_qual.cpp"; Pattern = 'ast::PathExpr path;' },
        @{ Path = "cursive\\src\\04_analysis\\resolve\\resolve_qual.cpp"; Pattern = 'SPEC_RULE\("ResolveQual-Name-Value"\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\type_expr.cpp"; Pattern = 'std::is_same_v<T,\s*ast::QualifiedNameExpr>' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\type_expr.cpp"; Pattern = 'SPEC_RULE\("Expr-Unresolved-Err"\)' }
    )

    foreach ($check in $checks) {
        $fullPath = Join-Path $workspaceRoot $check.Path
        if (-not (Test-Path $fullPath)) {
            throw "Case 'issue33_qualified_name_phase_boundary_wiring' missing file: $fullPath"
        }
        $text = Get-Content -Path $fullPath -Raw
        if ($text -notmatch $check.Pattern) {
            throw "Case 'issue33_qualified_name_phase_boundary_wiring' missing expected pattern '$($check.Pattern)' in $fullPath."
        }
    }

    Write-Host "[compiler-static] issue33_qualified_name_phase_boundary_wiring: checks=$($checks.Count)"
}

function Invoke-Issue33QualifiedNameResolutionPipelineTraceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue33_qualified_name_resolution_pipeline_trace" `
        -Source (New-Issue33QualifiedNameResolutionPipelineSource) `
        -ConformanceFileName "issue33_qualified_name_resolution_pipeline.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue33_qualified_name_resolution_pipeline_trace' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue33_qualified_name_resolution_pipeline_trace' expected zero compile-time errors but observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $parseQualifiedNameCount = @($logLines | Where-Object { $_ -like "*`tParse-Qualified-Name`t*" }).Count
    $parseQualifiedApplyParenCount = @($logLines | Where-Object { $_ -like "*`tParse-Qualified-Apply-Paren`t*" }).Count
    $resolveQualNameValueCount = @($logLines | Where-Object { $_ -like "*`tResolveQual-Name-Value`t*" }).Count
    $resolveQualApplyValueCount = @($logLines | Where-Object { $_ -like "*`tResolveQual-Apply-Value`t*" }).Count
    $resolveExprQualifiedCount = @($logLines | Where-Object { $_ -like "*`tResolveExpr-Qualified`t*" }).Count
    $typedPathValueCount = @($logLines | Where-Object { $_ -like "*`tT-Path-Value`t*" }).Count
    $exprUnresolvedCount = @($logLines | Where-Object { $_ -like "*`tExpr-Unresolved-Err`t*" }).Count

    if ($parseQualifiedNameCount -lt 1 -or
        $parseQualifiedApplyParenCount -lt 1 -or
        $resolveQualNameValueCount -lt 1 -or
        $resolveQualApplyValueCount -lt 1 -or
        $resolveExprQualifiedCount -lt 2 -or
        $typedPathValueCount -lt 2) {
        throw "Case 'issue33_qualified_name_resolution_pipeline_trace' expected Parse-Qualified-Name, Parse-Qualified-Apply-Paren, ResolveQual-Name-Value, ResolveQual-Apply-Value, ResolveExpr-Qualified, and T-Path-Value in conformance trace."
    }
    if ($exprUnresolvedCount -ne 0) {
        throw "Case 'issue33_qualified_name_resolution_pipeline_trace' expected no Expr-Unresolved-Err events after resolution."
    }

    Write-Host "[compiler-static] issue33_qualified_name_resolution_pipeline_trace: exit=$($result.ExitCode) errors=$errorCount parse_name=$parseQualifiedNameCount parse_apply=$parseQualifiedApplyParenCount resolve_name=$resolveQualNameValueCount resolve_apply=$resolveQualApplyValueCount resolve_qualified=$resolveExprQualifiedCount typed_path=$typedPathValueCount unresolved=$exprUnresolvedCount"
}

function Invoke-Issue51UsingItemParseTraceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue51_using_item_parse_trace" `
        -Source (New-Issue51UsingItemParseTraceSource) `
        -ConformanceFileName "issue51_using_item_parse_trace.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue51_using_item_parse_trace' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue51_using_item_parse_trace' expected zero compile-time errors but observed $errorCount."
    }

    $traceText = Get-Content -Path $result.ConformancePath -Raw
    $parseUsingItemCount = ([regex]::Matches($traceText, "`tParse-Using-Item`t")).Count
    $parseUsingListCount = ([regex]::Matches($traceText, "`tParse-Using-List`t")).Count
    $parseUsingWildcardCount = ([regex]::Matches($traceText, "`tParse-Using-Wildcard`t")).Count
    if ($parseUsingItemCount -lt 1) {
        throw "Case 'issue51_using_item_parse_trace' expected Parse-Using-Item in the conformance trace."
    }
    if ($parseUsingListCount -ne 0 -or $parseUsingWildcardCount -ne 0) {
        throw "Case 'issue51_using_item_parse_trace' expected only the single-item using parse branch; observed list=$parseUsingListCount wildcard=$parseUsingWildcardCount."
    }

    $parserFile = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\item\\using_decl.cpp"
    if (-not (Test-Path $parserFile)) {
        throw "Case 'issue51_using_item_parse_trace' missing parser implementation file: $parserFile"
    }
    $parserText = Get-Content -Path $parserFile -Raw
    if ($parserText -notmatch 'SPEC_RULE\("Parse-Using-Item"\)') {
        throw "Case 'issue51_using_item_parse_trace' expected parser implementation to contain the Parse-Using-Item SPEC_RULE anchor."
    }

    Write-Host "[compiler-static] issue51_using_item_parse_trace: exit=$($result.ExitCode) errors=$errorCount parse_using_item=$parseUsingItemCount"
}

function Invoke-Issue51UsingListParseTraceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue51_using_list_parse_trace" `
        -Source (New-Issue51UsingListParseTraceSource) `
        -ConformanceFileName "issue51_using_list_parse_trace.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue51_using_list_parse_trace' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue51_using_list_parse_trace' expected zero compile-time errors but observed $errorCount."
    }

    $traceText = Get-Content -Path $result.ConformancePath -Raw
    $parseUsingItemCount = ([regex]::Matches($traceText, "`tParse-Using-Item`t")).Count
    $parseUsingListCount = ([regex]::Matches($traceText, "`tParse-Using-List`t")).Count
    $parseUsingWildcardCount = ([regex]::Matches($traceText, "`tParse-Using-Wildcard`t")).Count
    if ($parseUsingListCount -lt 1) {
        throw "Case 'issue51_using_list_parse_trace' expected Parse-Using-List in the conformance trace."
    }
    if ($parseUsingItemCount -ne 0 -or $parseUsingWildcardCount -ne 0) {
        throw "Case 'issue51_using_list_parse_trace' expected only the list using parse branch; observed item=$parseUsingItemCount wildcard=$parseUsingWildcardCount."
    }

    $parseItemPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\item\\parse_item.cpp"
    if (-not (Test-Path $parseItemPath)) {
        throw "Case 'issue51_using_list_parse_trace' missing parser dispatcher file: $parseItemPath"
    }
    $parseItemText = Get-Content -Path $parseItemPath -Raw
    if ($parseItemText -notmatch 'return ParseUsingDecl\(start,\s*cur,\s*vis\.elem,\s*attrs\.elem\);') {
        throw "Case 'issue51_using_list_parse_trace' expected ParseItem to pass the item-start parser into ParseUsingDecl."
    }

    $usingDeclPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\item\\using_decl.cpp"
    if (-not (Test-Path $usingDeclPath)) {
        throw "Case 'issue51_using_list_parse_trace' missing parser implementation file: $usingDeclPath"
    }
    $usingDeclText = Get-Content -Path $usingDeclPath -Raw
    if ($usingDeclText -notmatch 'SPEC_RULE\("Parse-Using-List"\)') {
        throw "Case 'issue51_using_list_parse_trace' expected parser implementation to contain the Parse-Using-List SPEC_RULE anchor."
    }
    if ($usingDeclText -notmatch 'decl\.span = SpanBetween\(item_start,\s*specs\.parser\);') {
        throw "Case 'issue51_using_list_parse_trace' expected list using spans to start at the original item parser position."
    }

    Write-Host "[compiler-static] issue51_using_list_parse_trace: exit=$($result.ExitCode) errors=$errorCount parse_using_list=$parseUsingListCount"
}

function Invoke-Issue51UsingWildcardParseTraceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue51_using_wildcard_parse_trace" `
        -Source (New-Issue51UsingWildcardParseTraceSource) `
        -ConformanceFileName "issue51_using_wildcard_parse_trace.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue51_using_wildcard_parse_trace' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue51_using_wildcard_parse_trace' expected zero compile-time errors but observed $errorCount."
    }

    $traceText = Get-Content -Path $result.ConformancePath -Raw
    $parseUsingItemCount = ([regex]::Matches($traceText, "`tParse-Using-Item`t")).Count
    $parseUsingListCount = ([regex]::Matches($traceText, "`tParse-Using-List`t")).Count
    $parseUsingWildcardCount = ([regex]::Matches($traceText, "`tParse-Using-Wildcard`t")).Count
    if ($parseUsingWildcardCount -lt 1) {
        throw "Case 'issue51_using_wildcard_parse_trace' expected Parse-Using-Wildcard in the conformance trace."
    }
    if ($parseUsingItemCount -ne 0 -or $parseUsingListCount -ne 0) {
        throw "Case 'issue51_using_wildcard_parse_trace' expected only the wildcard using parse branch; observed item=$parseUsingItemCount list=$parseUsingListCount."
    }

    $parseItemPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\item\\parse_item.cpp"
    if (-not (Test-Path $parseItemPath)) {
        throw "Case 'issue51_using_wildcard_parse_trace' missing parser dispatcher file: $parseItemPath"
    }
    $parseItemText = Get-Content -Path $parseItemPath -Raw
    if ($parseItemText -notmatch 'return ParseUsingDecl\(start,\s*cur,\s*vis\.elem,\s*attrs\.elem\);') {
        throw "Case 'issue51_using_wildcard_parse_trace' expected ParseItem to pass the item-start parser into ParseUsingDecl."
    }

    $usingDeclPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\item\\using_decl.cpp"
    if (-not (Test-Path $usingDeclPath)) {
        throw "Case 'issue51_using_wildcard_parse_trace' missing parser implementation file: $usingDeclPath"
    }
    $usingDeclText = Get-Content -Path $usingDeclPath -Raw
    if ($usingDeclText -notmatch 'ParseItemResult ParseUsingDecl\(Parser item_start,\s*Parser parser,\s*Visibility vis,\s*AttrOpt attrs_opt\)') {
        throw "Case 'issue51_using_wildcard_parse_trace' expected ParseUsingDecl to accept the item-start parser."
    }
    if ($usingDeclText -notmatch 'decl\.span = SpanBetween\(item_start,\s*after_star\);') {
        throw "Case 'issue51_using_wildcard_parse_trace' expected wildcard using spans to start at the original item parser position."
    }

    Write-Host "[compiler-static] issue51_using_wildcard_parse_trace: exit=$($result.ExitCode) errors=$errorCount parse_using_wildcard=$parseUsingWildcardCount"
}

function Invoke-Issue51PublicUsingItemVisibilityCase {
    $crossModuleFiles = @{
        "mod/Other.cursive" = (New-Issue51PublicUsingItemModuleSource)
    }

    $rejected = Invoke-CheckWithConformance `
        -CaseId "issue51_public_using_item_visibility_rejected" `
        -Source (New-Issue51PublicUsingItemRejectMainSource) `
        -ConformanceFileName "issue51_public_using_item_visibility_rejected.log" `
        -ExtraFiles $crossModuleFiles

    if ($rejected.ExitCode -ne 1) {
        throw "Case 'issue51_public_using_item_visibility_rejected' expected exit 1 but got $($rejected.ExitCode)."
    }

    $rejectedErrorCount = @($rejected.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($rejectedErrorCount -lt 1) {
        throw "Case 'issue51_public_using_item_visibility_rejected' expected at least one compile-time error."
    }

    $rejectedDiagCount = @($rejected.DiagJson.diagnostics | Where-Object {
        $_.code -eq "E-MOD-1205"
    }).Count
    if ($rejectedDiagCount -lt 1) {
        throw "Case 'issue51_public_using_item_visibility_rejected' expected diagnostic code E-MOD-1205."
    }

    $rejectedLog = Get-Content -Path $rejected.ConformancePath
    $canonicalRejectCount = @($rejectedLog | Where-Object { $_ -like "*`tUsing-Item-Public-Err`t*" }).Count
    $legacyRejectCount = @($rejectedLog | Where-Object { $_ -like "*`tUsing-Path-Item-Public-Err`t*" }).Count
    $listRejectCount = @($rejectedLog | Where-Object { $_ -like "*`tUsing-List-Public-Err`t*" }).Count
    if ($canonicalRejectCount -lt 1) {
        throw "Case 'issue51_public_using_item_visibility_rejected' expected Using-Item-Public-Err in the conformance trace."
    }
    if ($legacyRejectCount -ne 0 -or $listRejectCount -ne 0) {
        throw "Case 'issue51_public_using_item_visibility_rejected' expected only the canonical single-item public-using rule; observed legacy=$legacyRejectCount list=$listRejectCount."
    }

    $accepted = Invoke-CheckWithConformance `
        -CaseId "issue51_public_using_item_visibility_accepted" `
        -Source (New-Issue51PublicUsingItemAcceptMainSource) `
        -ConformanceFileName "issue51_public_using_item_visibility_accepted.log" `
        -ExtraFiles $crossModuleFiles

    if ($accepted.ExitCode -ne 0) {
        throw "Case 'issue51_public_using_item_visibility_accepted' expected exit 0 but got $($accepted.ExitCode)."
    }

    $acceptedErrorCount = @($accepted.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($acceptedErrorCount -ne 0) {
        throw "Case 'issue51_public_using_item_visibility_accepted' expected zero compile-time errors but observed $acceptedErrorCount."
    }

    $acceptedLog = Get-Content -Path $accepted.ConformancePath
    $acceptedUsingPathCount = @($acceptedLog | Where-Object { $_ -like "*`tUsing-Path-Item`t*" }).Count
    $acceptedRejectCount = @($acceptedLog | Where-Object { $_ -like "*`tUsing-Item-Public-Err`t*" }).Count
    $acceptedLegacyRejectCount = @($acceptedLog | Where-Object { $_ -like "*`tUsing-Path-Item-Public-Err`t*" }).Count
    if ($acceptedUsingPathCount -lt 1) {
        throw "Case 'issue51_public_using_item_visibility_accepted' expected Using-Path-Item in the conformance trace."
    }
    if ($acceptedRejectCount -ne 0 -or $acceptedLegacyRejectCount -ne 0) {
        throw "Case 'issue51_public_using_item_visibility_accepted' unexpectedly emitted public-using rejection traces."
    }

    Write-Host "[compiler-static] issue51_public_using_item_visibility: rejected_diag=$rejectedDiagCount canonical_reject=$canonicalRejectCount accepted_using_item=$acceptedUsingPathCount"
}

function Invoke-Issue33ExprHelperTraceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue33_expr_helper_trace" `
        -Source (New-Issue33ExprHelperTraceSource) `
        -ConformanceFileName "issue33_expr_helper_trace.log"

    if ($result.ExitCode -ne 1) {
        throw "Case 'issue33_expr_helper_trace' expected exit 1 (region allocation diagnostic) but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -lt 1) {
        throw "Case 'issue33_expr_helper_trace' expected at least one compile-time error."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $allocCount = @($logLines | Where-Object { $_ -like "*`tParse-Alloc-Implicit`t*" }).Count
    $transmuteCount = @($logLines | Where-Object { $_ -like "*`tParse-Transmute-Expr`t*" }).Count
    $callTypeArgsCount = @($logLines | Where-Object { $_ -like "*`tPostfix-Call-TypeArgs`t*" }).Count

    if ($allocCount -lt 1 -or $transmuteCount -lt 1 -or $callTypeArgsCount -lt 1) {
        throw "Case 'issue33_expr_helper_trace' expected Parse-Alloc-Implicit, Parse-Transmute-Expr, and Postfix-Call-TypeArgs in conformance trace."
    }

    Write-Host "[compiler-static] issue33_expr_helper_trace: exit=$($result.ExitCode) errors=$errorCount alloc=$allocCount transmute=$transmuteCount call_type_args=$callTypeArgsCount"
}

function Invoke-Issue33AllocTypingTraceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue33_alloc_typing_trace" `
        -Source (New-Issue33AllocTypingTraceSource) `
        -ConformanceFileName "issue33_alloc_typing_trace.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue33_alloc_typing_trace' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue33_alloc_typing_trace' expected zero compile-time errors but observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $parseAllocImplicitCount = @($logLines | Where-Object { $_ -like "*`tParse-Alloc-Implicit`t*" }).Count
    $resolveAllocImplicitCount = @($logLines | Where-Object { $_ -like "*`tResolveExpr-Alloc-Implicit`t*" }).Count
    $resolveAllocExplicitByAliasCount = @($logLines | Where-Object { $_ -like "*`tResolveExpr-Alloc-Explicit-ByAlias`t*" }).Count
    $typedAllocImplicitCount = @($logLines | Where-Object { $_ -like "*`tT-Alloc-Implicit`t*" }).Count
    $typedAllocExplicitCount = @($logLines | Where-Object { $_ -like "*`tT-Alloc-Explicit`t*" }).Count
    $allocRegionNotFoundErrCount = @($logLines | Where-Object { $_ -like "*`tAlloc-Region-NotFound-Err`t*" }).Count
    $allocImplicitNoRegionErrCount = @($logLines | Where-Object { $_ -like "*`tAlloc-Implicit-NoRegion-Err`t*" }).Count

    if ($parseAllocImplicitCount -lt 1 -or
        $resolveAllocImplicitCount -lt 1 -or
        $resolveAllocExplicitByAliasCount -lt 1 -or
        $typedAllocImplicitCount -lt 1 -or
        $typedAllocExplicitCount -lt 1) {
        throw "Case 'issue33_alloc_typing_trace' expected Parse-Alloc-Implicit, ResolveExpr-Alloc-Implicit, ResolveExpr-Alloc-Explicit-ByAlias, T-Alloc-Implicit, and T-Alloc-Explicit in conformance trace."
    }
    if ($allocRegionNotFoundErrCount -ne 0 -or $allocImplicitNoRegionErrCount -ne 0) {
        throw "Case 'issue33_alloc_typing_trace' expected no allocation error rules in successful typing trace."
    }

    Write-Host "[compiler-static] issue33_alloc_typing_trace: exit=$($result.ExitCode) errors=$errorCount parse_alloc=$parseAllocImplicitCount resolve_implicit=$resolveAllocImplicitCount resolve_explicit_alias=$resolveAllocExplicitByAliasCount t_alloc_implicit=$typedAllocImplicitCount t_alloc_explicit=$typedAllocExplicitCount"
}

function Invoke-Issue33AllocImplicitNoRegionCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue33_alloc_implicit_no_region" `
        -Source (New-Issue33AllocImplicitNoRegionSource) `
        -ConformanceFileName "issue33_alloc_implicit_no_region.log"

    if ($result.ExitCode -ne 1) {
        throw "Case 'issue33_alloc_implicit_no_region' expected exit 1 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -lt 1) {
        throw "Case 'issue33_alloc_implicit_no_region' expected at least one compile-time error."
    }

    $diagCount = @($result.DiagJson.diagnostics | Where-Object { $_.code -eq "E-MEM-3021" }).Count
    if ($diagCount -lt 1) {
        throw "Case 'issue33_alloc_implicit_no_region' expected diagnostic code E-MEM-3021."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $parseAllocImplicitCount = @($logLines | Where-Object { $_ -like "*`tParse-Alloc-Implicit`t*" }).Count
    $allocImplicitNoRegionErrCount = @($logLines | Where-Object { $_ -like "*`tAlloc-Implicit-NoRegion-Err`t*" }).Count
    if ($parseAllocImplicitCount -lt 1 -or $allocImplicitNoRegionErrCount -lt 1) {
        throw "Case 'issue33_alloc_implicit_no_region' expected Parse-Alloc-Implicit and Alloc-Implicit-NoRegion-Err in conformance trace."
    }

    Write-Host "[compiler-static] issue33_alloc_implicit_no_region: exit=$($result.ExitCode) errors=$errorCount e_mem_3021=$diagCount parse_alloc=$parseAllocImplicitCount alloc_no_region=$allocImplicitNoRegionErrCount"
}

function Invoke-Issue33AllocExplicitFrozenRegionCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue33_alloc_explicit_frozen_region" `
        -Source (New-Issue33AllocExplicitFrozenRegionSource) `
        -ConformanceFileName "issue33_alloc_explicit_frozen_region.log"

    if ($result.ExitCode -ne 1) {
        throw "Case 'issue33_alloc_explicit_frozen_region' expected exit 1 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -lt 1) {
        throw "Case 'issue33_alloc_explicit_frozen_region' expected at least one compile-time error."
    }

    $diagCount = @($result.DiagJson.diagnostics | Where-Object { $_.code -eq "E-MEM-1206" }).Count
    if ($diagCount -lt 1) {
        throw "Case 'issue33_alloc_explicit_frozen_region' expected diagnostic code E-MEM-1206."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $resolveAllocExplicitByAliasCount = @($logLines | Where-Object { $_ -like "*`tResolveExpr-Alloc-Explicit-ByAlias`t*" }).Count
    $allocRegionNotFoundErrCount = @($logLines | Where-Object { $_ -like "*`tAlloc-Region-NotFound-Err`t*" }).Count
    if ($resolveAllocExplicitByAliasCount -lt 1 -or $allocRegionNotFoundErrCount -lt 1) {
        throw "Case 'issue33_alloc_explicit_frozen_region' expected ResolveExpr-Alloc-Explicit-ByAlias and Alloc-Region-NotFound-Err in conformance trace."
    }

    Write-Host "[compiler-static] issue33_alloc_explicit_frozen_region: exit=$($result.ExitCode) errors=$errorCount e_mem_1206=$diagCount resolve_explicit_alias=$resolveAllocExplicitByAliasCount alloc_region_not_found=$allocRegionNotFoundErrCount"
}

function Invoke-Issue33AllocLoweringTraceCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue33_alloc_lowering_trace" `
        -Source (New-Issue33AllocLoweringBuildSource) `
        -ConformanceFileName "issue33_alloc_lowering_trace.log" `
        -Manifest $manifest

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue33_alloc_lowering_trace' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue33_alloc_lowering_trace' expected zero compile-time errors but observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $lowerAllocCount = @($logLines | Where-Object { $_ -like "*`tLower-Expr-Alloc`t*" }).Count
    if ($lowerAllocCount -lt 2) {
        throw "Case 'issue33_alloc_lowering_trace' expected Lower-Expr-Alloc at least twice (implicit + explicit allocation)."
    }

    Write-Host "[compiler-static] issue33_alloc_lowering_trace: exit=$($result.ExitCode) errors=$errorCount lower_alloc=$lowerAllocCount"
}

function Invoke-Issue547TupleLoweringTraceCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue547_tuple_lowering_trace" `
        -Source (New-Issue547TupleLoweringTraceSource) `
        -ConformanceFileName "issue547_tuple_lowering_trace.log" `
        -Manifest $manifest

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue547_tuple_lowering_trace' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue547_tuple_lowering_trace' expected zero compile-time errors but observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $lowerTupleCount = @($logLines | Where-Object { $_ -like "*`tLower-Expr-Tuple`t*" }).Count
    $lowerTupleAccessCount = @($logLines | Where-Object { $_ -like "*`tLower-Expr-TupleAccess`t*" }).Count
    if ($lowerTupleCount -lt 4) {
        throw "Case 'issue547_tuple_lowering_trace' expected repeated Lower-Expr-Tuple traces for nested and temporary tuple literals."
    }
    if ($lowerTupleAccessCount -lt 4) {
        throw "Case 'issue547_tuple_lowering_trace' expected repeated Lower-Expr-TupleAccess traces for chained tuple projections."
    }

    $irPath = Join-Path $result.CaseRoot "build\\probe\\ir\\probe.ll"
    if (-not (Test-Path $irPath)) {
        throw "Case 'issue547_tuple_lowering_trace' missing LLVM IR file: $irPath"
    }

    $irText = Get-Content -Path $irPath -Raw
    $extractValueCount = ([regex]::Matches($irText, "\bextractvalue\b")).Count
    if ($extractValueCount -lt 4) {
        throw "Case 'issue547_tuple_lowering_trace' expected tuple element extraction in emitted LLVM IR."
    }

    $tupleLiteralPath = Join-Path $workspaceRoot "cursive\\src\\05_codegen\\lower\\expr\\tuple_literal.cpp"
    $lowerListPath = Join-Path $workspaceRoot "cursive\\src\\05_codegen\\lower\\expr\\expr_common.cpp"
    $tupleAccessPath = Join-Path $workspaceRoot "cursive\\src\\05_codegen\\lower\\expr\\tuple_access.cpp"
    $tupleLiteralText = Get-Content -Path $tupleLiteralPath -Raw
    $lowerListText = Get-Content -Path $lowerListPath -Raw
    $tupleAccessText = Get-Content -Path $tupleAccessPath -Raw
    if ($tupleLiteralText -notmatch 'LowerList\(expr\.elements,\s*ctx\)') {
        throw "Case 'issue547_tuple_lowering_trace' expected tuple lowering to delegate element evaluation to LowerList as required by the spec."
    }
    if ($tupleLiteralText -notmatch 'RegisterValueType\(tuple_value,\s*analysis::MakeTypeTuple') {
        throw "Case 'issue547_tuple_lowering_trace' expected tuple literal lowering to register the concrete tuple value type."
    }
    if ($lowerListText -notmatch 'for \(const auto& expr : exprs\)' -or
        $lowerListText -notmatch 'ir_parts\.push_back\(result\.ir\)' -or
        $lowerListText -notmatch 'values\.push_back\(result\.value\)' -or
        $lowerListText -notmatch 'return \{SeqIR\(std::move\(ir_parts\)\), std::move\(values\)\};') {
        throw "Case 'issue547_tuple_lowering_trace' expected LowerList to preserve left-to-right child evaluation and state threading."
    }
    if ($tupleAccessText -notmatch 'TupleElementTypeForIndex' -or
        $tupleAccessText -notmatch 'RegisterValueType\(elem_value,\s*elem_type\)') {
        throw "Case 'issue547_tuple_lowering_trace' expected tuple access lowering to register the projected element type explicitly."
    }

    Write-Host "[compiler-static] issue547_tuple_lowering_trace: exit=$($result.ExitCode) errors=$errorCount lower_tuple=$lowerTupleCount lower_tuple_access=$lowerTupleAccessCount extractvalue=$extractValueCount"
}

function Invoke-Issue548TupleAccessEvalSigmaCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue548_tuple_access_eval_sigma" `
        -Source (New-Issue548TupleAccessEvalSigmaSource) `
        -ConformanceFileName "issue548_tuple_access_eval_sigma.log" `
        -Manifest $manifest

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue548_tuple_access_eval_sigma' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue548_tuple_access_eval_sigma' expected zero compile-time errors but observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $lowerTupleAccessCount = @($logLines | Where-Object { $_ -like "*`tLower-Expr-TupleAccess`t*" }).Count
    if ($lowerTupleAccessCount -lt 2) {
        throw "Case 'issue548_tuple_access_eval_sigma' expected tuple access lowering for both EvalSigma and control-propagation probes."
    }

    $irPath = Join-Path $result.CaseRoot "build\\probe\\ir\\probe.ll"
    if (-not (Test-Path $irPath)) {
        throw "Case 'issue548_tuple_access_eval_sigma' missing LLVM IR file: $irPath"
    }

    $irText = Get-Content -Path $irPath -Raw
    $buildTupleCallCount = ([regex]::Matches($irText, 'call \{ i32, i32 \} @probe_x3a_x3aBuildTuple')).Count
    if ($buildTupleCallCount -ne 1) {
        throw "Case 'issue548_tuple_access_eval_sigma' expected exactly one BuildTuple call in emitted IR so the tuple-access base is evaluated once."
    }
    if ($irText -notmatch 'define i32 @probe_x3a_x3aTupleAccessEvalMetric[\s\S]*call \{ i32, i32 \} @probe_x3a_x3aBuildTuple[\s\S]*extractvalue \{ i32, i32 \} %\d+, 1') {
        throw "Case 'issue548_tuple_access_eval_sigma' expected TupleAccessEvalMetric to evaluate the tuple base and then extract element 1 from the resulting aggregate."
    }
    if ($irText -notmatch 'define i32 @probe_x3a_x3aTupleAccessCtrlMetric[\s\S]*ret i32 17[\s\S]*extractvalue \{ i32, i32 \} %\d+, 0') {
        throw "Case 'issue548_tuple_access_eval_sigma' expected TupleAccessCtrlMetric to retain an early-return control path ahead of tuple extraction in the non-return path."
    }

    $tupleAccessPath = Join-Path $workspaceRoot "cursive\\src\\05_codegen\\lower\\expr\\tuple_access.cpp"
    $llvmEmitPath = Join-Path $workspaceRoot "cursive\\src\\05_codegen\\llvm\\llvm_emit.cpp"
    $tupleAccessText = Get-Content -Path $tupleAccessPath -Raw
    $llvmEmitText = Get-Content -Path $llvmEmitPath -Raw
    if ($tupleAccessText -notmatch 'auto base_result = LowerExpr\(\*expr\.base,\s*ctx\);' -or
        $tupleAccessText -notmatch 'return LowerResult\{base_result\.ir,\s*elem_value\};') {
        throw "Case 'issue548_tuple_access_eval_sigma' expected LowerTupleAccess to lower the base exactly once and thread that IR forward unchanged."
    }
    if ($llvmEmitText -notmatch 'case DerivedValueInfo::Kind::Tuple[\s\S]*EvaluateIRValue\(derived->base\)[\s\S]*CreateExtractValue') {
        throw "Case 'issue548_tuple_access_eval_sigma' expected LLVM tuple materialization to extract from the already-evaluated base value."
    }

    Write-Host "[compiler-static] issue548_tuple_access_eval_sigma: exit=$($result.ExitCode) errors=$errorCount lower_tuple_access=$lowerTupleAccessCount build_tuple_calls=$buildTupleCallCount"
}

function Invoke-Issue549ArrayIndexConformanceCase {
    $specText = Get-Content -Path $canonicalSpecPath -Raw
    $requiredSpecRules = @(
        '\*\*\(Index-Array-NonConst-Err\)\*\*',
        '\*\*\(Index-Array-OOB-Err\)\*\*',
        '\*\*\(T-Index-Array-Dynamic\)\*\*',
        '\*\*\(P-Index-Array-Dynamic\)\*\*',
        'ComputeDynamicContext\(s, ancestors\)\s*=\s*[\r\n]+\s*let enclosing_dynamic = FindInnermostDynamic\(s, ancestors\)',
        'InDynamicContext ⇔ DynamicScope\(s\) where `s` is the span of the syntactic form currently being verified or type-checked\.'
    )
    foreach ($pattern in $requiredSpecRules) {
        if ($specText -notmatch $pattern) {
            throw "Case 'issue549_array_index_conformance' missing expected canonical rule pattern '$pattern' in the language spec."
        }
    }

    $exprAstPath = Join-Path $workspaceRoot "cursive\src\02_source\ast\nodes\ast_exprs.h"
    if (-not (Test-Path $exprAstPath)) {
        throw "Case 'issue549_array_index_conformance' missing expression AST helper source: $exprAstPath"
    }
    $exprAstText = Get-Content -Path $exprAstPath -Raw
    $requiredExprAstPatterns = @(
        'inline bool DynamicExpr\(const Expr& expr\)',
        'return !ExprAttrByName\(expr, "dynamic"\)\.empty\(\);'
    )
    foreach ($pattern in $requiredExprAstPatterns) {
        if ($exprAstText -notmatch $pattern) {
            throw "Case 'issue549_array_index_conformance' missing expected expression-attribute helper pattern '$pattern'."
        }
    }

    $dynamicContextHeaderPath = Join-Path $workspaceRoot "cursive\include\04_analysis\typing\dynamic_context.h"
    if (-not (Test-Path $dynamicContextHeaderPath)) {
        throw "Case 'issue549_array_index_conformance' missing dynamic-context header source: $dynamicContextHeaderPath"
    }
    $dynamicContextHeaderText = Get-Content -Path $dynamicContextHeaderPath -Raw
    $requiredDynamicContextHeaderPatterns = @(
        'bool ComputeDynamicContext\(\s*const core::Span& current_span,\s*std::span<const DynamicScopeAncestor> ancestors\);'
    )
    foreach ($pattern in $requiredDynamicContextHeaderPatterns) {
        if ($dynamicContextHeaderText -notmatch $pattern) {
            throw "Case 'issue549_array_index_conformance' missing expected dynamic-context header pattern '$pattern'."
        }
    }

    $dynamicContextPath = Join-Path $workspaceRoot "cursive\src\04_analysis\typing\dynamic_context.cpp"
    if (-not (Test-Path $dynamicContextPath)) {
        throw "Case 'issue549_array_index_conformance' missing dynamic-context implementation source: $dynamicContextPath"
    }
    $dynamicContextText = Get-Content -Path $dynamicContextPath -Raw
    $requiredDynamicContextPatterns = @(
        'bool ComputeDynamicContext\(\s*const core::Span& current_span,\s*std::span<const DynamicScopeAncestor> ancestors\) \{',
        'if \(FindInnermostDynamic\(current_span, ancestors\) != nullptr\) \{[\s\S]*return true;[\s\S]*\}[\s\S]*return false;'
    )
    foreach ($pattern in $requiredDynamicContextPatterns) {
        if ($dynamicContextText -notmatch $pattern) {
            throw "Case 'issue549_array_index_conformance' missing expected dynamic-context implementation pattern '$pattern'."
        }
    }

    $typeExprPath = Join-Path $workspaceRoot "cursive\src\04_analysis\typing\type_expr.cpp"
    if (-not (Test-Path $typeExprPath)) {
        throw "Case 'issue549_array_index_conformance' missing active type-expression source: $typeExprPath"
    }
    $typeExprText = Get-Content -Path $typeExprPath -Raw
    $requiredTypeExprPatterns = @(
        'bool ComputeExprDynamicContext\(const ast::Expr& expr, bool inherited\)',
        'if \(!ast::DynamicExpr\(expr\)\)',
        'const ast::AttributeList& attrs = ast::ExprAttrList\(expr\);',
        'return inherited \|\| ComputeDynamicContext\(expr\.span, ancestors\);',
        'dynamic_context = ComputeExprDynamicContext\(\*e, type_ctx\.contract_dynamic\);',
        'inner_ctx\.contract_dynamic =\s+e \? ComputeExprDynamicContext\(\*e, type_ctx\.contract_dynamic\)',
        'auto typed = TypeExpr\(ctx, inner_ctx, node\.body, comptime_env\);'
    )
    foreach ($pattern in $requiredTypeExprPatterns) {
        if ($typeExprText -notmatch $pattern) {
            throw "Case 'issue549_array_index_conformance' missing expected dynamic-expression typing pattern '$pattern'."
        }
    }

    $indexAccessPath = Join-Path $workspaceRoot "cursive\src\04_analysis\typing\expr\index_access.cpp"
    if (-not (Test-Path $indexAccessPath)) {
        throw "Case 'issue549_array_index_conformance' missing active index-access typer source: $indexAccessPath"
    }
    $indexAccessText = Get-Content -Path $indexAccessPath -Raw
    $requiredIndexPatterns = @(
        'SPEC_RULE\("Index-Array-NonConst-Err"\)',
        'SPEC_RULE\("Index-Array-OOB-Err"\)',
        'SPEC_RULE\("T-Index-Array-Dynamic"\)',
        'SPEC_RULE\("P-Index-Array-Dynamic"\)',
        'if \(!has_const_index && !type_ctx\.contract_dynamic\)',
        'if \(has_const_index && \*index_const\.value >= arr->length\)'
    )
    foreach ($pattern in $requiredIndexPatterns) {
        if ($indexAccessText -notmatch $pattern) {
            throw "Case 'issue549_array_index_conformance' missing expected active index-access pattern '$pattern'."
        }
    }

    $caseRoot = Join-Path $workRoot "issue549_array_index_registry"
    New-Item -ItemType Directory -Path $caseRoot -Force | Out-Null
    $generatorPath = Join-Path $workspaceRoot "cursive\tools\generate_static_rule_registry.ps1"
    $mappingPath = Join-Path $workspaceRoot "cursive\tools\static_rule_mapping.json"
    $registryOutPath = Join-Path $caseRoot "static_rule_registry.inc"
    $reportOutPath = Join-Path $caseRoot "static_rule_registry_report.json"
    & powershell -NoProfile -ExecutionPolicy Bypass -File $generatorPath -RepoRoot $workspaceRoot -MappingPath $mappingPath -OutputPath $registryOutPath -ReportPath $reportOutPath -Strict
    $generatorExit = $LASTEXITCODE
    if ($generatorExit -ne 0) {
        throw "Case 'issue549_array_index_conformance' expected registry generator exit 0 but got $generatorExit."
    }

    $registryText = Get-Content -Path $registryOutPath -Raw
    $requiredRegistryPatterns = @(
        '\{"Index-Array-NonConst-Err",\s*"ExprJudg",\s*std::nullopt,\s*"04_analysis/typing/expr/index_access\.cpp"\}',
        '\{"Index-Array-OOB-Err",\s*"ExprJudg",\s*std::nullopt,\s*"04_analysis/typing/expr/index_access\.cpp"\}',
        '\{"T-Index-Array-Dynamic",\s*"ExprJudg",\s*std::nullopt,\s*"04_analysis/typing/expr/index_access\.cpp"\}',
        '\{"P-Index-Array-Dynamic",\s*"ExprJudg",\s*std::nullopt,\s*"04_analysis/typing/expr/index_access\.cpp"\}'
    )
    foreach ($pattern in $requiredRegistryPatterns) {
        if ($registryText -notmatch $pattern) {
            throw "Case 'issue549_array_index_conformance' missing canonical registry entry '$pattern'."
        }
    }
    if ($registryText -match '\{"Index-Array-NonConst-Err",\s*"DeclJudg",\s*std::nullopt,\s*"04_analysis/composite/arrays_slices\.cpp"\}' -or
        $registryText -match '\{"Index-Array-OOB-Err",\s*"DeclJudg",\s*std::nullopt,\s*"04_analysis/composite/arrays_slices\.cpp"\}') {
        throw "Case 'issue549_array_index_conformance' found stale composite arrays_slices.cpp registry entries for array-index diagnostics."
    }

    $report = (Get-Content -Path $reportOutPath -Raw) | ConvertFrom-Json
    $duplicateIndexRules = @($report.duplicate_rule_ids | Where-Object {
        $_.rule_id -in @(
            "Index-Array-NonConst-Err",
            "Index-Array-OOB-Err",
            "T-Index-Array-Dynamic",
            "P-Index-Array-Dynamic"
        )
    })
    if ($duplicateIndexRules.Count -ne 0) {
        throw "Case 'issue549_array_index_conformance' expected zero duplicate array-index rule entries in generated registry report."
    }

    $appliedNonConstSource = $report.applied_source_overrides."Index-Array-NonConst-Err"
    $appliedOobSource = $report.applied_source_overrides."Index-Array-OOB-Err"
    $appliedDynamicValueSource = $report.applied_source_overrides."T-Index-Array-Dynamic"
    $appliedDynamicPlaceSource = $report.applied_source_overrides."P-Index-Array-Dynamic"
    if ($appliedNonConstSource -ne "04_analysis/typing/expr/index_access.cpp" -or
        $appliedOobSource -ne "04_analysis/typing/expr/index_access.cpp" -or
        $appliedDynamicValueSource -ne "04_analysis/typing/expr/index_access.cpp" -or
        $appliedDynamicPlaceSource -ne "04_analysis/typing/expr/index_access.cpp") {
        throw "Case 'issue549_array_index_conformance' expected applied source overrides for array-index rules to target typing/expr/index_access.cpp."
    }

    $nonConst = Invoke-CheckWithConformance `
        -CaseId "issue549_array_index_nonconst_rejected" `
        -Source (New-Issue549ArrayIndexNonConstSource) `
        -ConformanceFileName "issue549_array_index_nonconst_rejected.log"
    if ($nonConst.ExitCode -ne 1) {
        throw "Case 'issue549_array_index_nonconst_rejected' expected exit 1 but got $($nonConst.ExitCode)."
    }
    $nonConstCodeCount = @($nonConst.DiagJson.diagnostics | Where-Object { $_.code -eq "E-UNS-0102" }).Count
    if ($nonConstCodeCount -lt 1) {
        throw "Case 'issue549_array_index_nonconst_rejected' expected E-UNS-0102."
    }
    $nonConstLines = Get-Content -Path $nonConst.ConformancePath
    $nonConstTraceCount = @($nonConstLines | Where-Object { $_ -like "*`tIndex-Array-NonConst-Err`t*" }).Count
    if ($nonConstTraceCount -lt 1) {
        throw "Case 'issue549_array_index_nonconst_rejected' expected Index-Array-NonConst-Err in conformance trace."
    }

    $oob = Invoke-CheckWithConformance `
        -CaseId "issue549_array_index_oob_rejected" `
        -Source (New-Issue549ArrayIndexOobSource) `
        -ConformanceFileName "issue549_array_index_oob_rejected.log"
    if ($oob.ExitCode -ne 1) {
        throw "Case 'issue549_array_index_oob_rejected' expected exit 1 but got $($oob.ExitCode)."
    }
    $oobCodeCount = @($oob.DiagJson.diagnostics | Where-Object { $_.code -eq "E-UNS-0103" }).Count
    if ($oobCodeCount -lt 1) {
        throw "Case 'issue549_array_index_oob_rejected' expected E-UNS-0103."
    }
    $oobLines = Get-Content -Path $oob.ConformancePath
    $oobTraceCount = @($oobLines | Where-Object { $_ -like "*`tIndex-Array-OOB-Err`t*" }).Count
    if ($oobTraceCount -lt 1) {
        throw "Case 'issue549_array_index_oob_rejected' expected Index-Array-OOB-Err in conformance trace."
    }

    $dynamic = Invoke-CheckWithConformance `
        -CaseId "issue549_array_index_dynamic_allowed" `
        -Source (New-Issue549ArrayIndexDynamicSource) `
        -ConformanceFileName "issue549_array_index_dynamic_allowed.log"
    if ($dynamic.ExitCode -ne 0) {
        throw "Case 'issue549_array_index_dynamic_allowed' expected exit 0 but got $($dynamic.ExitCode)."
    }
    $dynamicErrorCount = @($dynamic.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($dynamicErrorCount -ne 0) {
        throw "Case 'issue549_array_index_dynamic_allowed' expected zero compile-time errors, observed $dynamicErrorCount."
    }
    $dynamicLines = Get-Content -Path $dynamic.ConformancePath
    $dynamicValueCount = @($dynamicLines | Where-Object { $_ -like "*`tT-Index-Array-Dynamic`t*" }).Count
    $dynamicPlaceCount = @($dynamicLines | Where-Object { $_ -like "*`tP-Index-Array-Dynamic`t*" }).Count
    if ($dynamicValueCount -lt 1 -or $dynamicPlaceCount -lt 1) {
        throw "Case 'issue549_array_index_dynamic_allowed' expected both value and place dynamic array-index traces."
    }

    $dynamicExpr = Invoke-CheckWithConformance `
        -CaseId "issue549_array_index_dynamic_expr_allowed" `
        -Source (New-Issue549ArrayIndexDynamicExprSource) `
        -ConformanceFileName "issue549_array_index_dynamic_expr_allowed.log"
    if ($dynamicExpr.ExitCode -ne 0) {
        throw "Case 'issue549_array_index_dynamic_expr_allowed' expected exit 0 but got $($dynamicExpr.ExitCode)."
    }
    $dynamicExprErrorCount = @($dynamicExpr.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($dynamicExprErrorCount -ne 0) {
        throw "Case 'issue549_array_index_dynamic_expr_allowed' expected zero compile-time errors, observed $dynamicExprErrorCount."
    }
    $dynamicExprNonConstCount = @($dynamicExpr.DiagJson.diagnostics | Where-Object { $_.code -eq "E-UNS-0102" }).Count
    if ($dynamicExprNonConstCount -ne 0) {
        throw "Case 'issue549_array_index_dynamic_expr_allowed' must not emit E-UNS-0102 inside expression-level [[dynamic]] scope."
    }
    $dynamicExprStderr = Get-Content -Path $dynamicExpr.StderrPath -Raw
    if ($dynamicExprStderr -match "Internal error:" -or $dynamicExprStderr -match "unknown diagnostic id") {
        throw "Case 'issue549_array_index_dynamic_expr_allowed' must not surface internal compiler diagnostics on expression-level [[dynamic]] array indexing."
    }
    $dynamicExprLines = Get-Content -Path $dynamicExpr.ConformancePath
    $dynamicExprValueCount = @($dynamicExprLines | Where-Object { $_ -like "*`tT-Index-Array-Dynamic`t*" }).Count
    if ($dynamicExprValueCount -lt 1) {
        throw "Case 'issue549_array_index_dynamic_expr_allowed' expected a dynamic array-index value trace."
    }

    $dynamicStmt = Invoke-CheckWithConformance `
        -CaseId "issue549_array_index_dynamic_stmt_allowed" `
        -Source (New-Issue549ArrayIndexDynamicStmtSource) `
        -ConformanceFileName "issue549_array_index_dynamic_stmt_allowed.log"
    if ($dynamicStmt.ExitCode -ne 0) {
        throw "Case 'issue549_array_index_dynamic_stmt_allowed' expected exit 0 but got $($dynamicStmt.ExitCode)."
    }
    $dynamicStmtErrorCount = @($dynamicStmt.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($dynamicStmtErrorCount -ne 0) {
        throw "Case 'issue549_array_index_dynamic_stmt_allowed' expected zero compile-time errors, observed $dynamicStmtErrorCount."
    }
    $dynamicStmtNonConstCount = @($dynamicStmt.DiagJson.diagnostics | Where-Object { $_.code -eq "E-UNS-0102" }).Count
    if ($dynamicStmtNonConstCount -ne 0) {
        throw "Case 'issue549_array_index_dynamic_stmt_allowed' must not emit E-UNS-0102 inside statement-level [[dynamic]] scope."
    }
    $dynamicStmtStderr = Get-Content -Path $dynamicStmt.StderrPath -Raw
    if ($dynamicStmtStderr -match "Internal error:" -or $dynamicStmtStderr -match "unknown diagnostic id") {
        throw "Case 'issue549_array_index_dynamic_stmt_allowed' must not surface internal compiler diagnostics on statement-level [[dynamic]] array indexing."
    }
    $dynamicStmtLines = Get-Content -Path $dynamicStmt.ConformancePath
    $dynamicStmtValueCount = @($dynamicStmtLines | Where-Object { $_ -like "*`tT-Index-Array-Dynamic`t*" }).Count
    $dynamicStmtPlaceCount = @($dynamicStmtLines | Where-Object { $_ -like "*`tP-Index-Array-Dynamic`t*" }).Count
    if ($dynamicStmtValueCount -lt 1 -or $dynamicStmtPlaceCount -lt 1) {
        throw "Case 'issue549_array_index_dynamic_stmt_allowed' expected both dynamic array-index value and place traces."
    }

    Write-Host "[compiler-static] issue549_array_index_conformance: expr_attr_helper=1 dynamic_context_helper=1 dynamic_expr_typing=1 registry=1 nonconst_code=$nonConstCodeCount nonconst_trace=$nonConstTraceCount oob_code=$oobCodeCount oob_trace=$oobTraceCount dynamic_value=$dynamicValueCount dynamic_place=$dynamicPlaceCount dynamic_expr_value=$dynamicExprValueCount dynamic_stmt_value=$dynamicStmtValueCount dynamic_stmt_place=$dynamicStmtPlaceCount"
}

function Invoke-Issue560IfStmtNonUnitBranchDiagnosticCase {
    $cases = @(
        @{
            Id = "issue560_if_stmt_nonunit_branch_diag"
            Source = (New-Issue560IfStmtNonUnitBranchSource)
        },
        @{
            Id = "issue560_loop_if_stmt_nonunit_branch_diag"
            Source = (New-Issue560LoopIfStmtNonUnitBranchSource)
        }
    )

    foreach ($case in $cases) {
        $caseRoot = Join-Path $workRoot $case.Id
        New-Item -ItemType Directory -Path $caseRoot -Force | Out-Null

        [System.IO.File]::WriteAllLines((Join-Path $caseRoot "Cursive.toml"), $manifestLines)
        [System.IO.File]::WriteAllText((Join-Path $caseRoot "Main.cursive"), $case.Source)

        $diagJsonPath = Join-Path $caseRoot "diag.json"
        $stderrPath = Join-Path $caseRoot "stderr.txt"
        Push-Location $caseRoot
        try {
            & $CompilerPath --incremental off --check --diag-json --quiet Main.cursive 1> $diagJsonPath 2> $stderrPath
            $exitCode = $LASTEXITCODE
        } finally {
            Pop-Location
        }

        if ($exitCode -ne 1) {
            throw "Case '$($case.Id)' expected exit 1 but got $exitCode."
        }

        $diagJson = (Get-Content -Path $diagJsonPath -Raw) | ConvertFrom-Json
        $mismatchCount = @($diagJson.diagnostics | Where-Object {
            $_.code -eq "E-MOD-2402"
        }).Count
        if ($mismatchCount -lt 1) {
            throw "Case '$($case.Id)' expected diagnostic code 'E-MOD-2402'."
        }

        $stderrText = Get-Content -Path $stderrPath -Raw
        if ($stderrText -match "Internal error:" -or $stderrText -match "unknown diagnostic id") {
            throw "Case '$($case.Id)' must not surface internal compiler diagnostics."
        }
    }

    Write-Host "[compiler-static] issue560_if_stmt_nonunit_branch_diag: cases=$($cases.Count) matched_code=E-MOD-2402"
}

function Invoke-Issue550ArrayEvalSigmaCase {
    $specText = Get-Content -Path $canonicalSpecPath -Raw
    $requiredSpecRules = @(
        '\*\*\(EvalSigma-Array\)\*\*',
        '\*\*\(EvalSigma-Array-Ctrl\)\*\*'
    )
    foreach ($pattern in $requiredSpecRules) {
        if ($specText -notmatch $pattern) {
            throw "Case 'issue550_array_eval_sigma' missing expected canonical rule pattern '$pattern' in the language spec."
        }
    }

    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue550_array_eval_sigma" `
        -Source (New-Issue550ArrayEvalSigmaSource) `
        -ConformanceFileName "issue550_array_eval_sigma.log" `
        -Manifest $manifest

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue550_array_eval_sigma' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue550_array_eval_sigma' expected zero compile-time errors but observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $lowerArrayCount = @($logLines | Where-Object { $_ -like "*`tLower-Expr-Array`t*" }).Count
    if ($lowerArrayCount -lt 2) {
        throw "Case 'issue550_array_eval_sigma' expected array lowering traces for both the evaluation-order and control-propagation probes."
    }

    $irText = Get-EmittedLlvmIrText -CaseId "issue550_array_eval_sigma" -CaseRoot $result.CaseRoot
    if ($irText -notmatch 'define \[2 x i32\] @probe_x3a_x3aBuildArray[\s\S]*call i32 @probe_x3a_x3aAdvance\(ptr %counter_mut, i32 7[\s\S]*call i32 @probe_x3a_x3aAdvance\(ptr %counter_mut, i32 8[\s\S]*insertvalue \[2 x i32\] zeroinitializer, i32 %\d+, 0[\s\S]*insertvalue \[2 x i32\] %\d+, i32 %\d+, 1') {
        throw "Case 'issue550_array_eval_sigma' expected BuildArray to evaluate array elements left-to-right and materialize the resulting aggregate in emitted LLVM IR."
    }
    if ($irText -notmatch 'define i32 @probe_x3a_x3aArrayCtrlMetric[\s\S]*call i32 @probe_x3a_x3aAdvance\(ptr %counter, i32 4[\s\S]*ret i32 17') {
        throw "Case 'issue550_array_eval_sigma' expected ArrayCtrlMetric to retain the observable early-return path from the middle array element."
    }

    $arrayLiteralPath = Join-Path $workspaceRoot "cursive\\src\\05_codegen\\lower\\expr\\array_literal.cpp"
    $llvmEmitPath = Join-Path $workspaceRoot "cursive\\src\\05_codegen\\llvm\\llvm_emit.cpp"
    $arrayLiteralText = Get-Content -Path $arrayLiteralPath -Raw
    $llvmEmitText = Get-Content -Path $llvmEmitPath -Raw
    if ($arrayLiteralText -notmatch 'LowerList\(expr\.elements,\s*ctx\)' -or
        $arrayLiteralText -notmatch 'RegisterValueType\(\s*array_value,\s*analysis::MakeTypeArray') {
        throw "Case 'issue550_array_eval_sigma' expected array literal lowering to route element evaluation through LowerList and preserve the concrete array type."
    }
    if ($llvmEmitText -notmatch 'case DerivedValueInfo::Kind::ArrayLit[\s\S]*CreateInsertValue') {
        throw "Case 'issue550_array_eval_sigma' expected LLVM array materialization to build the aggregate from the already-evaluated element values."
    }

    Write-Host "[compiler-static] issue550_array_eval_sigma: exit=$($result.ExitCode) errors=$errorCount lower_array=$lowerArrayCount"
}

function Invoke-Issue554CallTempNoProvenanceCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue554_call_temp_no_provenance" `
        -Source (New-Issue554CallTempNoProvenanceSource) `
        -ConformanceFileName "issue554_call_temp_no_provenance.log" `
        -Manifest $manifest

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue554_call_temp_no_provenance' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue554_call_temp_no_provenance' expected zero compile-time errors but observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $lowerMoveCount = @($logLines | Where-Object { $_ -like "*`tLower-Args-Cons-Move`t*" }).Count
    $lowerRefCount = @($logLines | Where-Object { $_ -like "*`tLower-Args-Cons-Ref`t*" }).Count
    if ($lowerMoveCount -lt 1 -or $lowerRefCount -lt 1) {
        throw "Case 'issue554_call_temp_no_provenance' expected both move and reference call-argument lowering traces."
    }

    $irPath = Join-Path $result.CaseRoot "build\\probe\\ir\\probe.ll"
    if (-not (Test-Path $irPath)) {
        throw "Case 'issue554_call_temp_no_provenance' missing LLVM IR file: $irPath"
    }

    $irText = Get-Content -Path $irPath -Raw
    if ($irText -notmatch '%call_ref_tmp_[0-9]+ = alloca i32, align 4' -or
        $irText -notmatch 'store i32 3, ptr %call_ref_tmp_[0-9]+, align 4' -or
        $irText -notmatch 'call i32 @probe_x3a_x3aborrow\(ptr %call_ref_tmp_[0-9]+, ptr %\d+\)') {
        throw "Case 'issue554_call_temp_no_provenance' expected by-reference provenance-less arguments to lower through a caller-side temporary place."
    }
    if ($irText -notmatch '%call_move_tmp_[0-9]+ = alloca i32, align 4' -or
        $irText -notmatch 'store i32 7, ptr %call_move_tmp_[0-9]+, align 4' -or
        $irText -notmatch 'load i32, ptr %call_move_tmp_[0-9]+, align 4' -or
        $irText -notmatch 'call i32 @probe_x3a_x3aconsume\(i32 %\d+, ptr %\d+\)' -or
        $irText -match 'call i32 @probe_x3a_x3aconsume\(i32 7, ptr %\d+\)') {
        throw "Case 'issue554_call_temp_no_provenance' expected consuming provenance-less arguments to lower through a caller-side temporary place before the move."
    }

    $callLoweringPath = Join-Path $workspaceRoot "cursive\src\05_codegen\lower\expr\call.cpp"
    $callsAnalysisPath = Join-Path $workspaceRoot "cursive\src\04_analysis\memory\calls.cpp"
    $callLoweringText = Get-Content -Path $callLoweringPath -Raw
    $callsAnalysisText = Get-Content -Path $callsAnalysisPath -Raw
    if ($callLoweringText -notmatch 'LowerMoveArgExprWithTemp' -or
        $callLoweringText -notmatch 'UsesCallTempForConsuming' -or
        $callLoweringText -notmatch 'LowerMovePlace\(temp_ident,\s*ctx\)') {
        throw "Case 'issue554_call_temp_no_provenance' expected ordinary call lowering to materialize and move from a synthetic call temporary."
    }
    if ($callsAnalysisText -notmatch 'UsesCallTempForConsumingLocal' -or
        $callsAnalysisText -notmatch 'return mode == ParamMode::Move && !arg\.moved &&\s*!HasSourceProvenanceLocal\(arg\.value\);') {
        throw "Case 'issue554_call_temp_no_provenance' expected call analysis to model the spec-defined ConsumeArgExpr call-temp branch."
    }

    Write-Host "[compiler-static] issue554_call_temp_no_provenance: exit=$($result.ExitCode) errors=$errorCount lower_move=$lowerMoveCount lower_ref=$lowerRefCount"
}

function Invoke-Issue551IndexEvalSigmaCase {
    $specText = Get-Content -Path $canonicalSpecPath -Raw
    $requiredSpecRules = @(
        '\*\*\(EvalSigma-Index\)\*\*',
        '\*\*\(EvalSigma-Index-OOB\)\*\*'
    )
    foreach ($pattern in $requiredSpecRules) {
        if ($specText -notmatch $pattern) {
            throw "Case 'issue551_index_eval_sigma' missing expected canonical rule pattern '$pattern' in the language spec."
        }
    }

    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue551_index_eval_sigma" `
        -Source (New-Issue551IndexEvalSigmaSource) `
        -ConformanceFileName "issue551_index_eval_sigma.log" `
        -Manifest $manifest

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue551_index_eval_sigma' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue551_index_eval_sigma' expected zero compile-time errors but observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $lowerIndexCount = @($logLines | Where-Object { $_ -like "*`tLower-Expr-Index-Scalar`t*" }).Count
    if ($lowerIndexCount -lt 3) {
        throw "Case 'issue551_index_eval_sigma' expected Lower-Expr-Index-Scalar in the conformance trace for success, control, and bounds probes."
    }

    $indexAccessPath = Join-Path $workspaceRoot "cursive\\src\\05_codegen\\lower\\expr\\index_access.cpp"
    $llvmEmitPath = Join-Path $workspaceRoot "cursive\\src\\05_codegen\\llvm\\llvm_emit.cpp"
    $indexAccessText = Get-Content -Path $indexAccessPath -Raw
    $llvmEmitText = Get-Content -Path $llvmEmitPath -Raw

    $requiredLowerPatterns = @(
        'auto base_result = LowerExpr\(\*expr\.base,\s*ctx\);',
        'auto index_result = LowerExpr\(\*expr\.index,\s*ctx\);',
        'const bool needs_check = NeedsIndexCheck\(\*expr\.base,\s*ctx\);',
        'IRCheckIndex check;',
        'info\.kind = DerivedValueInfo::Kind::Index;',
        'ctx\.RegisterDerivedValue\(elem_value,\s*info\);'
    )
    foreach ($pattern in $requiredLowerPatterns) {
        if ($indexAccessText -notmatch $pattern) {
            throw "Case 'issue551_index_eval_sigma' missing expected index lowering pattern '$pattern'."
        }
    }

    if ($llvmEmitText -notmatch 'void operator\(\)\(const IRCheckIndex &check\) const[\s\S]*CreateICmpULT[\s\S]*PanicCode\(PanicReason::Bounds\)') {
        throw "Case 'issue551_index_eval_sigma' expected LLVM check_index lowering to compare the runtime index against the base length and route failures to the bounds panic path."
    }
    if ($llvmEmitText -notmatch 'case DerivedValueInfo::Kind::Index[\s\S]*EvaluateIRValue\(derived->base\)[\s\S]*EvaluateIRValue\(derived->index\)[\s\S]*CreateLoad') {
        throw "Case 'issue551_index_eval_sigma' expected LLVM index materialization to consume the evaluated base and evaluated index before loading the selected element."
    }

    $irText = Get-EmittedLlvmIrText -CaseId "issue551_index_eval_sigma" -CaseRoot $result.CaseRoot
    if ($irText -notmatch 'define i32 @probe_x3a_x3aIndexEvalMetric[\s\S]*call \[2 x i32\] @probe_x3a_x3aBuildValues[\s\S]*call i64 @probe_x3a_x3aBuildIndex[\s\S]*icmp ult i64 %\d+, 2[\s\S]*getelementptr inbounds \[2 x i32\], ptr %\d+, i64 0, i64 %\d+[\s\S]*load i32, ptr %\d+') {
        throw "Case 'issue551_index_eval_sigma' expected IndexEvalMetric to evaluate the array base, then the index, then perform the bounds check and element load in emitted LLVM IR."
    }
    if ($irText -notmatch 'define i32 @probe_x3a_x3aIndexCtrlMetric[\s\S]*call \[2 x i32\] @probe_x3a_x3aBuildValues[\s\S]*ret i32 17') {
        throw "Case 'issue551_index_eval_sigma' expected IndexCtrlMetric to preserve the control-return path from index evaluation after the base has already been evaluated."
    }
    if ($irText -notmatch 'define i32 @probe_x3a_x3aIndexOobMetric[\s\S]*icmp ult i64 %\d+, 2[\s\S]*store i32 6, ptr %\d+') {
        throw "Case 'issue551_index_eval_sigma' expected IndexOobMetric to retain the runtime bounds-failure path that records panic code 6."
    }

    Write-Host "[compiler-static] issue551_index_eval_sigma: exit=$($result.ExitCode) errors=$errorCount lower_index=$lowerIndexCount"
}

function Invoke-Issue554IfCaseTempOwnershipCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue554_if_case_temp_ownership" `
        -Source (New-Issue554IfCaseTempOwnershipSource) `
        -ConformanceFileName "issue554_if_case_temp_ownership.log" `
        -Manifest $manifest

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue554_if_case_temp_ownership' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue554_if_case_temp_ownership' expected zero compile-time errors but observed $errorCount."
    }

    $ifCaseLoweringPath = Join-Path $workspaceRoot "cursive\\src\\05_codegen\\lower\\expr\\if_case_expr.cpp"
    $ifCaseLoweringText = Get-Content -Path $ifCaseLoweringPath -Raw
    $requiredPatterns = @(
        'std::vector<TempValue>\s+scrutinee_temps;',
        'ctx\.temp_sink = &scrutinee_temps;',
        'IRPtr scrutinee_cleanup = CleanupTemps\(scrutinee_temps,\s*ctx\);',
        'struct OwnedIfCaseScrutinee',
        'IRBindVar scrutinee_bind;',
        'capture\.name = ctx\.FreshTempValue\("if_case_clause_result"\)\.name;'
    )
    foreach ($pattern in $requiredPatterns) {
        if ($ifCaseLoweringText -notmatch $pattern) {
            throw "Case 'issue554_if_case_temp_ownership' missing expected if-case lowering pattern '$pattern'."
        }
    }

    $irText = Get-EmittedLlvmIrText -CaseId "issue554_if_case_temp_ownership" -CaseRoot $result.CaseRoot
    $funcIfCase = [regex]::Match(
        $irText,
        'define i32 @probe_x3a_x3aIfCaseOwnedMetric[\s\S]*?\n\}',
        [System.Text.RegularExpressions.RegexOptions]::Singleline)
    if (-not $funcIfCase.Success) {
        throw "Case 'issue554_if_case_temp_ownership' could not locate the emitted IfCaseOwnedMetric LLVM function."
    }
    $payloadDropCount = [regex]::Matches(
        $funcIfCase.Value,
        'load \{ ptr, i64, i64 \}, ptr %text[\s\S]{0,120}@cursive_x3a_x3aruntime_x3a_x3astring_x3a_x3adrop_x5fmanaged',
        [System.Text.RegularExpressions.RegexOptions]::Singleline).Count
    if ($payloadDropCount -lt 1) {
        throw "Case 'issue554_if_case_temp_ownership' expected IfCaseOwnedMetric to drop the bound payload local in emitted LLVM IR."
    }
    $scrutineeDropCount = [regex]::Matches(
        $funcIfCase.Value,
        'load \{ ptr, i64, i64 \}, ptr %if_case_scrutinee_[\s\S]{0,120}@cursive_x3a_x3aruntime_x3a_x3astring_x3a_x3adrop_x5fmanaged',
        [System.Text.RegularExpressions.RegexOptions]::Singleline).Count
    if ($scrutineeDropCount -ne 0) {
        throw "Case 'issue554_if_case_temp_ownership' expected IfCaseOwnedMetric to avoid dropping the hidden if-case scrutinee storage, but observed $scrutineeDropCount such drop paths."
    }

    Write-Host "[compiler-static] issue554_if_case_temp_ownership: exit=$($result.ExitCode) errors=$errorCount payload_drop_paths=$payloadDropCount scrutinee_drop_paths=$scrutineeDropCount"
}

function Invoke-Issue555SliceIndexEvalSigmaCase {
    $specText = Get-Content -Path $canonicalSpecPath -Raw
    $requiredSpecRules = @(
        '\*\*\(EvalSigma-Index\)\*\*',
        '\*\*\(EvalSigma-Index-OOB\)\*\*'
    )
    foreach ($pattern in $requiredSpecRules) {
        if ($specText -notmatch $pattern) {
            throw "Case 'issue555_slice_index_eval_sigma' missing expected canonical rule pattern '$pattern' in the language spec."
        }
    }

    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue555_slice_index_eval_sigma" `
        -Source (New-Issue555SliceIndexEvalSigmaSource) `
        -ConformanceFileName "issue555_slice_index_eval_sigma.log" `
        -Manifest $manifest

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue555_slice_index_eval_sigma' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue555_slice_index_eval_sigma' expected zero compile-time errors but observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $lowerIndexCount = @($logLines | Where-Object { $_ -like "*`tLower-Expr-Index-Scalar`t*" }).Count
    if ($lowerIndexCount -lt 2) {
        throw "Case 'issue555_slice_index_eval_sigma' expected Lower-Expr-Index-Scalar in the conformance trace for the dynamic success and bounds probes."
    }

    $llvmEmitPath = Join-Path $workspaceRoot "cursive\\src\\05_codegen\\llvm\\llvm_emit.cpp"
    $llvmEmitText = Get-Content -Path $llvmEmitPath -Raw
    $requiredEmitPatterns = @(
        'llvm::Value \*DynamicLengthOf\(const IRValue &value\) const',
        'void operator\(\)\(const IRCheckIndex &check\) const[\s\S]*dynamic_len = DynamicLengthOf\(check\.base\);',
        'void operator\(\)\(const IRCheckRange &check\) const[\s\S]*dynamic_len = DynamicLengthOf\(check\.base\);',
        'void operator\(\)\(const IRCheckSliceLen &check\) const[\s\S]*llvm::Value \*base_len = length_value\(check\.base\);'
    )
    foreach ($pattern in $requiredEmitPatterns) {
        if ($llvmEmitText -notmatch $pattern) {
            throw "Case 'issue555_slice_index_eval_sigma' missing expected LLVM bounds-check pattern '$pattern'."
        }
    }

    $irText = Get-EmittedLlvmIrText -CaseId "issue555_slice_index_eval_sigma" -CaseRoot $result.CaseRoot
    if ($irText -notmatch 'define i32 @probe_x3a_x3aSliceIndexEvalMetric[\s\S]*call \{ ptr, i64 \} @probe_x3a_x3aBuildSlice[\s\S]*call i64 @probe_x3a_x3aBuildSliceIndex[\s\S]*extractvalue \{ ptr, i64 \} %\d+, 1[\s\S]*icmp ult i64 %\d+, %\d+[\s\S]*extractvalue \{ ptr, i64 \} %\d+, 0[\s\S]*getelementptr inbounds i8, ptr %\d+, i64 %\d+[\s\S]*load i8, ptr %\d+') {
        throw "Case 'issue555_slice_index_eval_sigma' expected SliceIndexEvalMetric to extract the dynamic slice length, perform the bounds check, and then load the selected byte."
    }
    if ($irText -notmatch 'define i32 @probe_x3a_x3aSliceIndexOobMetric[\s\S]*extractvalue \{ ptr, i64 \} %\d+, 1[\s\S]*icmp ult i64 %\d+, %\d+[\s\S]*store i32 6, ptr %\d+') {
        throw "Case 'issue555_slice_index_eval_sigma' expected SliceIndexOobMetric to retain the dynamic slice bounds-failure path that records panic code 6."
    }

    Write-Host "[compiler-static] issue555_slice_index_eval_sigma: exit=$($result.ExitCode) errors=$errorCount lower_index=$lowerIndexCount"
}

function Invoke-Issue556RangeIndexEvalSigmaCase {
    $specText = Get-Content -Path $canonicalSpecPath -Raw
    $requiredSpecRules = @(
        '\*\*\(EvalSigma-Index-Range\)\*\*',
        '\*\*\(EvalSigma-Index-Range-OOB\)\*\*'
    )
    foreach ($pattern in $requiredSpecRules) {
        if ($specText -notmatch $pattern) {
            throw "Case 'issue556_range_index_eval_sigma' missing expected canonical rule pattern '$pattern' in the language spec."
        }
    }

    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue556_range_index_eval_sigma" `
        -Source (New-Issue556RangeIndexEvalSigmaSource) `
        -ConformanceFileName "issue556_range_index_eval_sigma.log" `
        -Manifest $manifest

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue556_range_index_eval_sigma' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue556_range_index_eval_sigma' expected zero compile-time errors but observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $lowerRangeCount = @($logLines | Where-Object { $_ -like "*`tLower-Expr-Index-Range`t*" }).Count
    if ($lowerRangeCount -lt 3) {
        throw "Case 'issue556_range_index_eval_sigma' expected Lower-Expr-Index-Range in the conformance trace for the array-backed, slice-backed, and bounds-failure probes."
    }

    $indexAccessPath = Join-Path $workspaceRoot "cursive\\src\\05_codegen\\lower\\expr\\index_access.cpp"
    $llvmEmitPath = Join-Path $workspaceRoot "cursive\\src\\05_codegen\\llvm\\llvm_emit.cpp"
    $indexAccessText = Get-Content -Path $indexAccessPath -Raw
    $llvmEmitText = Get-Content -Path $llvmEmitPath -Raw

    $requiredLowerPatterns = @(
        'if \(std::holds_alternative<ast::RangeExpr>\(expr\.index->node\)\)',
        'IRValue slice_value = ctx\.FreshTempValue\("slice"\);',
        'info\.kind = DerivedValueInfo::Kind::Slice;',
        'ctx\.RegisterDerivedValue\(slice_value,\s*info\);'
    )
    foreach ($pattern in $requiredLowerPatterns) {
        if ($indexAccessText -notmatch $pattern) {
            throw "Case 'issue556_range_index_eval_sigma' missing expected range-index lowering pattern '$pattern'."
        }
    }

    $requiredEmitPatterns = @(
        'case DerivedValueInfo::Kind::Slice',
        'std::get_if<analysis::TypeArray>\(&base_type->node\)',
        'base_len = builder->CreateExtractValue\(base,\s*\{1u\}\);',
        'llvm::Value \*slice_len = builder->CreateSub\(end,\s*start\);',
        'slice_value = builder->CreateInsertValue\(slice_value,\s*slice_ptr,\s*\{0u\}\);',
        'slice_value = builder->CreateInsertValue\(slice_value,\s*slice_len,\s*\{1u\}\);'
    )
    foreach ($pattern in $requiredEmitPatterns) {
        if ($llvmEmitText -notmatch $pattern) {
            throw "Case 'issue556_range_index_eval_sigma' missing expected LLVM slice materialization pattern '$pattern'."
        }
    }

    $irText = Get-EmittedLlvmIrText -CaseId "issue556_range_index_eval_sigma" -CaseRoot $result.CaseRoot
    if ($irText -notmatch 'define i32 @probe_x3a_x3aRangeIndexArrayMetric[\s\S]*call \[4 x i32\] @probe_x3a_x3aBuildArray[\s\S]*call i64 @probe_x3a_x3aBuildRangeStart[\s\S]*call i64 @probe_x3a_x3aBuildRangeEnd[\s\S]*icmp ule i64 %\d+, %\d+[\s\S]*icmp ule i64 %\d+, 4[\s\S]*getelementptr inbounds i32, ptr %\d+, i64 %\d+[\s\S]*sub i64 %\d+, %\d+[\s\S]*insertvalue \{ ptr, i64 \} .*?, ptr %\d+, 0[\s\S]*insertvalue \{ ptr, i64 \} .*?, i64 %\d+, 1') {
        throw "Case 'issue556_range_index_eval_sigma' expected RangeIndexArrayMetric to evaluate the base before both range bounds, check the bounds, and materialize the resulting array-backed slice in emitted LLVM IR."
    }
    if ($irText -notmatch 'define i32 @probe_x3a_x3aRangeIndexSliceMetric[\s\S]*call \{ ptr, i64 \} @probe_x3a_x3aBuildSlice[\s\S]*call i64 @probe_x3a_x3aBuildRangeStart[\s\S]*call i64 @probe_x3a_x3aBuildRangeEnd') {
        throw "Case 'issue556_range_index_eval_sigma' expected RangeIndexSliceMetric to evaluate the base before both dynamic range bounds in emitted LLVM IR."
    }
    if ($irText -notmatch 'define i32 @probe_x3a_x3aRangeIndexSliceMetric[\s\S]*icmp ule i64 %\d+, %\d+[\s\S]*icmp ule i64 %\d+, 4') {
        throw "Case 'issue556_range_index_eval_sigma' expected RangeIndexSliceMetric to retain the dynamic slice range-bounds checks in emitted LLVM IR."
    }
    if ($irText -notmatch 'define i32 @probe_x3a_x3aRangeIndexSliceMetric[\s\S]*load \{ ptr, i64 \}, ptr %values[\s\S]*extractvalue \{ ptr, i64 \} %\d+, 1[\s\S]*extractvalue \{ ptr, i64 \} %\d+, 0[\s\S]*getelementptr inbounds i8, ptr %\d+, i64 %\d+[\s\S]*sub i64 %\d+, %\d+[\s\S]*insertvalue \{ ptr, i64 \} undef, ptr %\d+, 0[\s\S]*insertvalue \{ ptr, i64 \} %\d+, i64 %\d+, 1') {
        throw "Case 'issue556_range_index_eval_sigma' expected RangeIndexSliceMetric to extract the dynamic slice backing pointer and length, compute the subslice length, and materialize the resulting slice in emitted LLVM IR."
    }
    if ($irText -notmatch 'define i32 @probe_x3a_x3aRangeIndexOobMetric[\s\S]*icmp ule i64 1, %\d+[\s\S]*icmp ule i64 %\d+, 4[\s\S]*store i32 6, ptr %\d+') {
        throw "Case 'issue556_range_index_eval_sigma' expected RangeIndexOobMetric to retain the runtime range-bounds failure path that records panic code 6."
    }

    Write-Host "[compiler-static] issue556_range_index_eval_sigma: exit=$($result.ExitCode) errors=$errorCount lower_range=$lowerRangeCount"
}

function Invoke-Issue557RangeEvalSigmaCase {
    $specText = Get-Content -Path $canonicalSpecPath -Raw
    $requiredSpecRules = @(
        '\*\*\(EvalSigma-Range\)\*\*',
        '\*\*\(EvalSigma-Range-Ctrl\)\*\*',
        '\*\*\(EvalSigma-Range-Ctrl-Hi\)\*\*'
    )
    foreach ($pattern in $requiredSpecRules) {
        if ($specText -notmatch $pattern) {
            throw "Case 'issue557_range_eval_sigma' missing expected canonical rule pattern '$pattern' in the language spec."
        }
    }

    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue557_range_eval_sigma" `
        -Source (New-Issue557RangeEvalSigmaSource) `
        -ConformanceFileName "issue557_range_eval_sigma.log" `
        -Manifest $manifest

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue557_range_eval_sigma' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue557_range_eval_sigma' expected zero compile-time errors but observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $lowerExprRangeCount = @($logLines | Where-Object { $_ -like "*`tLower-Expr-Range`t*" }).Count
    $lowerIndexRangeCount = @($logLines | Where-Object { $_ -like "*`tLower-Expr-Index-Range`t*" }).Count
    if ($lowerExprRangeCount -lt 1) {
        throw "Case 'issue557_range_eval_sigma' expected Lower-Expr-Range in the conformance trace for standalone range-value construction."
    }
    if ($lowerIndexRangeCount -lt 1) {
        throw "Case 'issue557_range_eval_sigma' expected Lower-Expr-Index-Range in the conformance trace for consuming the standalone range value."
    }

    $rangeLowerPath = Join-Path $workspaceRoot "cursive\\src\\05_codegen\\lower\\expr\\range.cpp"
    $llvmEmitPath = Join-Path $workspaceRoot "cursive\\src\\05_codegen\\llvm\\llvm_emit.cpp"
    $rangeLowerText = Get-Content -Path $rangeLowerPath -Raw
    $llvmEmitText = Get-Content -Path $llvmEmitPath -Raw

    $requiredLowerPatterns = @(
        'SPEC_RULE\("Lower-Expr-Range"\)',
        'lower_opt\(expr\.lhs,\s*value\.lo\);',
        'lower_opt\(expr\.rhs,\s*value\.hi\);',
        'info\.kind = DerivedValueInfo::Kind::RangeLit;',
        'ctx\.RegisterDerivedValue\(range_value,\s*info\);'
    )
    foreach ($pattern in $requiredLowerPatterns) {
        if ($rangeLowerText -notmatch $pattern) {
            throw "Case 'issue557_range_eval_sigma' missing expected range-evaluation lowering pattern '$pattern'."
        }
    }

    $requiredEmitPatterns = @(
        'case DerivedValueInfo::Kind::RangeLit',
        'RangeStructShape::TwoBounds',
        'builder->CreateInsertValue\(out,\s*lo,\s*\{0u\}\);',
        'builder->CreateInsertValue\(out,\s*hi,\s*\{1u\}\);',
        'std::optional<MaterializedRangeValue> ResolveRangeValue\('
    )
    foreach ($pattern in $requiredEmitPatterns) {
        if ($llvmEmitText -notmatch $pattern) {
            throw "Case 'issue557_range_eval_sigma' missing expected runtime range-materialization pattern '$pattern'."
        }
    }

    $irText = Get-EmittedLlvmIrText -CaseId "issue557_range_eval_sigma" -CaseRoot $result.CaseRoot
    if ($irText -notmatch 'define \{ i64, i64 \} @probe_x3a_x3aBuildRangeValue[\s\S]*call i64 @probe_x3a_x3aBuildRangeStart[\s\S]*call i64 @probe_x3a_x3aBuildRangeEnd[\s\S]*insertvalue \{ i64, i64 \} .*?, i64 %\d+, 0[\s\S]*insertvalue \{ i64, i64 \} .*?, i64 %\d+, 1') {
        throw "Case 'issue557_range_eval_sigma' expected BuildRangeValue to evaluate lower and upper bounds in order and materialize the standalone range value in emitted LLVM IR."
    }
    if ($irText -notmatch 'define i32 @probe_x3a_x3aRangeValueMetric[\s\S]*call \{ i64, i64 \} @probe_x3a_x3aBuildRangeValue[\s\S]*extractvalue \{ i64, i64 \} %\d+, 0[\s\S]*extractvalue \{ i64, i64 \} %\d+, 1[\s\S]*icmp ule i64 %\d+, %\d+[\s\S]*icmp ule i64 %\d+, 5') {
        throw "Case 'issue557_range_eval_sigma' expected RangeValueMetric to consume the helper-returned standalone range value through slice bounds checks in emitted LLVM IR."
    }

    Write-Host "[compiler-static] issue557_range_eval_sigma: exit=$($result.ExitCode) errors=$errorCount lower_expr_range=$lowerExprRangeCount lower_index_range=$lowerIndexRangeCount"
}

function Invoke-Issue558RecordEvalSigmaCase {
    $specText = Get-Content -Path $canonicalSpecPath -Raw
    $requiredSpecRules = @(
        '\*\*\(EvalSigma-Record\)\*\*',
        '\*\*\(EvalSigma-Record-Ctrl\)\*\*',
        '\*\*\(ApplyRecordCtorSigma\)\*\*',
        '\*\*\(ApplyRecordCtorSigma-Ctrl\)\*\*'
    )
    foreach ($pattern in $requiredSpecRules) {
        if ($specText -notmatch $pattern) {
            throw "Case 'issue558_record_eval_sigma' missing expected canonical rule pattern '$pattern' in the language spec."
        }
    }

    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue558_record_eval_sigma" `
        -Source (New-Issue558RecordEvalSigmaSource) `
        -ConformanceFileName "issue558_record_eval_sigma.log" `
        -Manifest $manifest

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue558_record_eval_sigma' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue558_record_eval_sigma' expected zero compile-time errors but observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $lowerRecordCount = @($logLines | Where-Object { $_ -like "*`tLower-Expr-Record`t*" }).Count
    $recordCtorCount = @($logLines | Where-Object { $_ -like "*`tLower-CallIR-RecordCtor`t*" }).Count
    $fieldInitsCount = @($logLines | Where-Object { $_ -like "*`tLowerFieldInits-Cons`t*" }).Count
    if ($lowerRecordCount -lt 2) {
        throw "Case 'issue558_record_eval_sigma' expected record-literal lowering traces for the evaluation-order and control-propagation probes."
    }
    if ($recordCtorCount -lt 1) {
        throw "Case 'issue558_record_eval_sigma' expected a record-constructor lowering trace for the zero-argument default-constructor probe."
    }
    if ($fieldInitsCount -lt 3) {
        throw "Case 'issue558_record_eval_sigma' expected LowerFieldInits-Cons traces for both record literals and the default-record constructor."
    }

    $recordLiteralPath = Join-Path $workspaceRoot "cursive\\src\\05_codegen\\lower\\expr\\record_literal.cpp"
    $callPath = Join-Path $workspaceRoot "cursive\\src\\05_codegen\\lower\\expr\\call.cpp"
    $exprCommonPath = Join-Path $workspaceRoot "cursive\\src\\05_codegen\\lower\\expr\\expr_common.cpp"
    $qualifiedApplyPath = Join-Path $workspaceRoot "cursive\\src\\05_codegen\\lower\\expr\\qualified_apply.cpp"
    $recordLiteralText = Get-Content -Path $recordLiteralPath -Raw
    $callText = Get-Content -Path $callPath -Raw
    $exprCommonText = Get-Content -Path $exprCommonPath -Raw
    $qualifiedApplyText = Get-Content -Path $qualifiedApplyPath -Raw

    $requiredLowerPatterns = @(
        @{
            Path = "record_literal.cpp"
            Pattern = 'LowerFieldInits\(expr\.fields,\s*ctx,\s*/\*suppress_temps=\*/true\)'
        },
        @{
            Path = "record_literal.cpp"
            Pattern = 'RegisterLoweredRecordValue\(\s*std::move\(field_values\),\s*LowerRecordTargetType\(expr,\s*ctx\),\s*"record",\s*ctx\s*\)'
        },
        @{
            Path = "call.cpp"
            Pattern = 'RegisterLoweredRecordValue\(\s*std::move\(field_values\),[\s\S]*"record_ctor",\s*ctx\s*\)'
        },
        @{
            Path = "expr_common.cpp"
            Pattern = 'IRValue RegisterLoweredRecordValue\('
        },
        @{
            Path = "qualified_apply.cpp"
            Pattern = 'RegisterLoweredRecordValue\(\s*std::move\(field_values\),\s*std::nullopt,\s*"qualified_apply",\s*ctx\s*\)'
        }
    )
    foreach ($entry in $requiredLowerPatterns) {
        $text = switch ($entry.Path) {
            "record_literal.cpp" { $recordLiteralText }
            "call.cpp" { $callText }
            "expr_common.cpp" { $exprCommonText }
            "qualified_apply.cpp" { $qualifiedApplyText }
            default { "" }
        }
        if ($text -notmatch $entry.Pattern) {
            throw "Case 'issue558_record_eval_sigma' missing expected record-lowering pattern '$($entry.Pattern)' in $($entry.Path)."
        }
    }

    $irText = Get-EmittedLlvmIrText -CaseId "issue558_record_eval_sigma" -CaseRoot $result.CaseRoot
    if ($irText -notmatch 'define i32 @probe_x3a_x3aRecordEvalMetric[\s\S]*call i32 @probe_x3a_x3aAdvance\(ptr %counter, i32 1, ptr %\d+\)[\s\S]*call i32 @probe_x3a_x3aAdvance\(ptr %counter, i32 2, ptr %\d+\)[\s\S]*call i32 @probe_x3a_x3aAdvance\(ptr %counter, i32 3, ptr %\d+\)[\s\S]*store \{ i32, i32, i32 \} zeroinitializer, ptr %\d+, align 4[\s\S]*store i32 %\d+, ptr %\d+, align 1[\s\S]*store i32 %\d+, ptr %\d+, align 1[\s\S]*store i32 %\d+, ptr %\d+, align 1') {
        throw "Case 'issue558_record_eval_sigma' expected RecordEvalMetric to evaluate record fields left-to-right and materialize the resulting record aggregate in emitted LLVM IR."
    }
    if ($irText -notmatch 'define i32 @probe_x3a_x3aRecordCtrlMetric[\s\S]*call i32 @probe_x3a_x3aAdvance\(ptr %counter, i32 4[\s\S]*ret i32 17') {
        throw "Case 'issue558_record_eval_sigma' expected RecordCtrlMetric to retain the observable early-return path from the middle record field."
    }
    if ($irText -notmatch 'define i32 @probe_x3a_x3aRecordCtorMetric[\s\S]*call i32 @probe_x3a_x3aDefaultFieldOne\(ptr %\d+\)[\s\S]*call i32 @probe_x3a_x3aDefaultFieldTwo\(ptr %\d+\)[\s\S]*store \{ i32, i32 \} zeroinitializer, ptr %\d+, align 4[\s\S]*store i32 %\d+, ptr %\d+, align 1[\s\S]*store i32 %\d+, ptr %\d+, align 1') {
        throw "Case 'issue558_record_eval_sigma' expected RecordCtorMetric to evaluate default field initializers in declaration order and materialize the resulting record aggregate in emitted LLVM IR."
    }

    Write-Host "[compiler-static] issue558_record_eval_sigma: exit=$($result.ExitCode) errors=$errorCount lower_record=$lowerRecordCount record_ctor=$recordCtorCount field_inits=$fieldInitsCount"
}

function Invoke-Issue52PackedFieldRefArgGapCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue52_packed_field_ref_arg_enforced" `
        -Source (New-Issue52PackedFieldRefArgSource) `
        -ConformanceFileName "issue52_packed_field_ref_arg_enforced.log"

    if ($result.ExitCode -ne 1) {
        throw "Case 'issue52_packed_field_ref_arg_enforced' expected exit 1 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -lt 1) {
        throw "Case 'issue52_packed_field_ref_arg_enforced' expected at least one compile-time error."
    }

    $packedDiagCount = @($result.DiagJson.diagnostics | Where-Object { $_.code -eq "E-TYP-2105" }).Count
    if ($packedDiagCount -lt 1) {
        throw "Case 'issue52_packed_field_ref_arg_enforced' expected diagnostic code E-TYP-2105."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $packedCallErrCount = @($logLines | Where-Object { $_ -like "*`tCall-Arg-Packed-Unsafe-Err`t*" }).Count
    if ($packedCallErrCount -lt 1) {
        throw "Case 'issue52_packed_field_ref_arg_enforced' expected Call-Arg-Packed-Unsafe-Err in conformance trace."
    }

    Write-Host "[compiler-static] issue52_packed_field_ref_arg_enforced: exit=$($result.ExitCode) errors=$errorCount e_typ_2105=$packedDiagCount packed_call_err=$packedCallErrCount"
}

function Invoke-Issue52PackedFieldRefArgUnsafeCase {
    Invoke-ExpectedSuccessCase `
        -Id "issue52_packed_field_ref_arg_unsafe_allowed" `
        -Source (New-Issue52PackedFieldRefArgUnsafeSource)
}

function Invoke-Issue52GenericDefaultTypeArgsCase {
    Invoke-ExpectedSuccessCase `
        -Id "issue52_generic_default_type_args" `
        -Source (New-Issue52GenericDefaultTypeArgsSource)
}

function Invoke-Issue52IfCaseTraceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue52_if_case_trace" `
        -Source (New-Issue52IfCaseSectionDriftSource) `
        -ConformanceFileName "issue52_if_case_trace.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue52_if_case_trace' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue52_if_case_trace' expected zero compile-time errors but observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $ifElseCount = @($logLines | Where-Object { $_ -like "*`tT-If-Else`t*" }).Count
    $ifCanonicalCount = @($logLines | Where-Object { $_ -like "*`tT-If`t*" }).Count
    $ifIsCount = @($logLines | Where-Object { $_ -like "*`tT-IfIs`t*" }).Count
    $ifCaseOtherCount = @($logLines | Where-Object { $_ -like "*`tT-IfCase-Other`t*" }).Count

    if ($ifElseCount -lt 1 -or $ifCanonicalCount -lt 1 -or $ifIsCount -lt 1 -or $ifCaseOtherCount -lt 1) {
        throw "Case 'issue52_if_case_trace' expected T-If, T-If-Else, T-IfIs, and T-IfCase-Other in conformance trace."
    }

    Write-Host "[compiler-static] issue52_if_case_trace: exit=$($result.ExitCode) errors=$errorCount t_if_else=$ifElseCount t_if=$ifCanonicalCount t_if_is=$ifIsCount t_if_case_other=$ifCaseOtherCount"
}

function Invoke-Issue52IfCaseClauseUnreachableTraceCase {
    param(
        [string]$CaseId,
        [string]$Source,
        [string]$ConformanceFileName
    )

    $result = Invoke-CheckWithConformance `
        -CaseId $CaseId `
        -Source $Source `
        -ConformanceFileName $ConformanceFileName

    if ($result.ExitCode -ne 1) {
        throw "Case '$CaseId' expected exit 1 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -lt 1) {
        throw "Case '$CaseId' expected at least one compile-time error."
    }

    $diagCount = @($result.DiagJson.diagnostics | Where-Object { $_.code -eq "E-SEM-2751" }).Count
    if ($diagCount -lt 1) {
        throw "Case '$CaseId' expected diagnostic code E-SEM-2751."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $ruleCount = @($logLines | Where-Object { $_ -like "*`tIfCase-Unreachable`t*" }).Count
    if ($ruleCount -lt 1) {
        throw "Case '$CaseId' expected IfCase-Unreachable in conformance trace."
    }

    Write-Host "[compiler-static] ${CaseId}: exit=$($result.ExitCode) errors=$errorCount e_sem_2751=$diagCount if_case_clause_unreachable=$ruleCount"
}

function Invoke-Issue52IfCaseClauseUnreachableIrrefutableCase {
    Invoke-Issue52IfCaseClauseUnreachableTraceCase `
        -CaseId "issue52_if_case_clause_unreachable_irrefutable" `
        -Source (New-Issue52IfCaseClauseUnreachableIrrefutableSource) `
        -ConformanceFileName "issue52_if_case_clause_unreachable_irrefutable.log"
}

function Invoke-Issue52IfCaseClauseUnreachableEnumDuplicateCase {
    Invoke-Issue52IfCaseClauseUnreachableTraceCase `
        -CaseId "issue52_if_case_clause_unreachable_enum_duplicate" `
        -Source (New-Issue52IfCaseClauseUnreachableEnumDuplicateSource) `
        -ConformanceFileName "issue52_if_case_clause_unreachable_enum_duplicate.log"
}

function Invoke-Issue52IfCaseClauseUnreachableUnionDuplicateCase {
    Invoke-Issue52IfCaseClauseUnreachableTraceCase `
        -CaseId "issue52_if_case_clause_unreachable_union_duplicate" `
        -Source (New-Issue52IfCaseClauseUnreachableUnionDuplicateSource) `
        -ConformanceFileName "issue52_if_case_clause_unreachable_union_duplicate.log"
}

function Invoke-Issue52SolveStmtInferenceTraceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue52_solve_stmt_inference_trace" `
        -Source (New-Issue52SolveStmtInferenceSource) `
        -ConformanceFileName "issue52_solve_stmt_inference_trace.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue52_solve_stmt_inference_trace' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue52_solve_stmt_inference_trace' expected zero compile-time errors but observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $solveCount = @($logLines | Where-Object { $_ -like "*`tSolve`t*" }).Count
    $letInferCount = @($logLines | Where-Object { $_ -like "*`tT-LetStmt-Infer`t*" }).Count
    $varInferCount = @($logLines | Where-Object { $_ -like "*`tT-VarStmt-Infer`t*" }).Count
    $usingAliasCount = @($logLines | Where-Object { $_ -like "*`tT-UsingLocalStmt`t*" }).Count
    $warnResultUnreachableCount = @($logLines | Where-Object { $_ -like "*`tWarnResultUnreachable`t*" }).Count

    if ($solveCount -lt 4 -or
        $letInferCount -lt 1 -or
        $varInferCount -lt 1 -or
        $usingAliasCount -lt 1 -or
        $warnResultUnreachableCount -lt 1) {
        throw "Case 'issue52_solve_stmt_inference_trace' expected Solve and inferred statement typing traces (let/var/using-alias) plus WarnResultUnreachable."
    }

    Write-Host "[compiler-static] issue52_solve_stmt_inference_trace: exit=$($result.ExitCode) errors=$errorCount solve=$solveCount let_infer=$letInferCount var_infer=$varInferCount using_alias=$usingAliasCount warn_result_unreachable=$warnResultUnreachableCount"
}

function Invoke-Issue52FieldAccessRecordTraceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue52_field_access_record_trace" `
        -Source (New-Issue52FieldAccessRecordTraceSource) `
        -ConformanceFileName "issue52_field_access_record_trace.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue52_field_access_record_trace' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue52_field_access_record_trace' expected zero compile-time errors but observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $fieldRecordCount = @($logLines | Where-Object { $_ -like "*`tT-Field-Record`t*" }).Count
    $fieldRecordPermCount = @($logLines | Where-Object { $_ -like "*`tT-Field-Record-Perm`t*" }).Count

    if ($fieldRecordCount -lt 1 -or $fieldRecordPermCount -lt 1) {
        throw "Case 'issue52_field_access_record_trace' expected T-Field-Record and T-Field-Record-Perm in conformance trace."
    }

    Write-Host "[compiler-static] issue52_field_access_record_trace: exit=$($result.ExitCode) errors=$errorCount t_field_record=$fieldRecordCount t_field_record_perm=$fieldRecordPermCount"
}

function Invoke-Issue52FieldAccessEnumDiagCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue52_field_access_enum_diag" `
        -Source (New-Issue52FieldAccessEnumErrorSource) `
        -ConformanceFileName "issue52_field_access_enum_diag.log"

    if ($result.ExitCode -ne 1) {
        throw "Case 'issue52_field_access_enum_diag' expected exit 1 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -lt 1) {
        throw "Case 'issue52_field_access_enum_diag' expected at least one compile-time error."
    }

    $diagCount = @($result.DiagJson.diagnostics | Where-Object { $_.code -eq "E-TYP-1904" }).Count
    if ($diagCount -lt 1) {
        throw "Case 'issue52_field_access_enum_diag' expected diagnostic code E-TYP-1904."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $fieldEnumCount = @($logLines | Where-Object { $_ -like "*`tFieldAccess-Enum`t*" }).Count
    if ($fieldEnumCount -lt 1) {
        throw "Case 'issue52_field_access_enum_diag' expected FieldAccess-Enum in conformance trace."
    }

    Write-Host "[compiler-static] issue52_field_access_enum_diag: exit=$($result.ExitCode) errors=$errorCount e_typ_1904=$diagCount field_access_enum=$fieldEnumCount"
}

function Invoke-Issue52RangeLiftTraceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue52_range_lift_trace" `
        -Source (New-Issue52RangeLiftTraceSource) `
        -ConformanceFileName "issue52_range_lift_trace.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue52_range_lift_trace' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue52_range_lift_trace' expected zero compile-time errors but observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $rangeLiftCount = @($logLines | Where-Object { $_ -like "*`tT-Range-Lift`t*" }).Count
    if ($rangeLiftCount -lt 1) {
        throw "Case 'issue52_range_lift_trace' expected T-Range-Lift in conformance trace."
    }

    Write-Host "[compiler-static] issue52_range_lift_trace: exit=$($result.ExitCode) errors=$errorCount t_range_lift=$rangeLiftCount"
}

function Invoke-Issue52TransmuteInvalidTargetWarningCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue52_transmute_invalid_target_warning" `
        -Source (New-Issue52TransmuteInvalidTargetWarningSource) `
        -ConformanceFileName "issue52_transmute_invalid_target_warning.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue52_transmute_invalid_target_warning' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue52_transmute_invalid_target_warning' expected zero compile-time errors but observed $errorCount."
    }

    $warningCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "warning" -and $_.code -eq "W-SAFE-0100"
    }).Count
    if ($warningCount -lt 1) {
        throw "Case 'issue52_transmute_invalid_target_warning' expected warning code W-SAFE-0100."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $warnRuleCount = @($logLines | Where-Object { $_ -like "*`tW-Transmute-Invalid-Target`t*" }).Count
    if ($warnRuleCount -lt 1) {
        throw "Case 'issue52_transmute_invalid_target_warning' expected W-Transmute-Invalid-Target in conformance trace."
    }

    Write-Host "[compiler-static] issue52_transmute_invalid_target_warning: exit=$($result.ExitCode) errors=$errorCount w_safe_0100=$warningCount warn_rule=$warnRuleCount"
}

function Invoke-Issue52AsyncTryTraceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue52_async_try_trace" `
        -Source (New-Issue52AsyncTrySuccessSource) `
        -ConformanceFileName "issue52_async_try_trace.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue52_async_try_trace' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue52_async_try_trace' expected zero compile-time errors but observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $asyncTryCount = @($logLines | Where-Object { $_ -like "*`tT-Async-Try`t*" }).Count
    if ($asyncTryCount -lt 1) {
        throw "Case 'issue52_async_try_trace' expected T-Async-Try in conformance trace."
    }

    Write-Host "[compiler-static] issue52_async_try_trace: exit=$($result.ExitCode) errors=$errorCount t_async_try=$asyncTryCount"
}

function Invoke-Issue52AsyncTryInfallibleErrCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue52_async_try_infallible_err" `
        -Source (New-Issue52AsyncTryInfallibleErrSource) `
        -ConformanceFileName "issue52_async_try_infallible_err.log"

    if ($result.ExitCode -ne 1) {
        throw "Case 'issue52_async_try_infallible_err' expected exit 1 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -lt 1) {
        throw "Case 'issue52_async_try_infallible_err' expected at least one compile-time error."
    }

    $diagCount = @($result.DiagJson.diagnostics | Where-Object { $_.code -eq "E-CON-0230" }).Count
    if ($diagCount -lt 1) {
        throw "Case 'issue52_async_try_infallible_err' expected diagnostic code E-CON-0230."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $infallibleErrCount = @($logLines | Where-Object { $_ -like "*`tAsync-Try-Infallible-Err`t*" }).Count
    if ($infallibleErrCount -lt 1) {
        throw "Case 'issue52_async_try_infallible_err' expected Async-Try-Infallible-Err in conformance trace."
    }

    Write-Host "[compiler-static] issue52_async_try_infallible_err: exit=$($result.ExitCode) errors=$errorCount e_con_0230=$diagCount async_try_infallible_err=$infallibleErrCount"
}

function Invoke-Issue52TransitionBorrowTraceCase {
    $baseline = Invoke-CheckWithConformance `
        -CaseId "issue52_transition_borrow_trace_no_call" `
        -Source (New-Issue52TransitionBorrowNoCallSource) `
        -ConformanceFileName "issue52_transition_borrow_trace_no_call.log"
    $withCall = Invoke-CheckWithConformance `
        -CaseId "issue52_transition_borrow_trace_with_call" `
        -Source (New-Issue52TransitionBorrowCallSource) `
        -ConformanceFileName "issue52_transition_borrow_trace_with_call.log"

    if ($baseline.ExitCode -ne 0 -or $withCall.ExitCode -ne 0) {
        throw "Case 'issue52_transition_borrow_trace' expected both runs to exit 0; observed no_call=$($baseline.ExitCode), with_call=$($withCall.ExitCode)."
    }

    $baselineErrors = @($baseline.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    $withCallErrors = @($withCall.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($baselineErrors -ne 0 -or $withCallErrors -ne 0) {
        throw "Case 'issue52_transition_borrow_trace' expected zero compile-time errors in both runs."
    }

    $baselineLines = Get-Content -Path $baseline.ConformancePath
    $withCallLines = Get-Content -Path $withCall.ConformancePath
    $baselineTransitionCount = @($baselineLines | Where-Object { $_ -like "*`tB-Transition`t*" }).Count
    $withCallTransitionCount = @($withCallLines | Where-Object { $_ -like "*`tB-Transition`t*" }).Count

    if ($baselineTransitionCount -lt 1 -or $withCallTransitionCount -le $baselineTransitionCount) {
        throw "Case 'issue52_transition_borrow_trace' expected transition-call trace to increase B-Transition emission (no_call=$baselineTransitionCount with_call=$withCallTransitionCount)."
    }

    Write-Host "[compiler-static] issue52_transition_borrow_trace: no_call_b_transition=$baselineTransitionCount with_call_b_transition=$withCallTransitionCount"
}

function Invoke-Issue52ExportVisErrTraceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue52_export_vis_err_trace" `
        -Source (New-Issue52ExportVisErrSource) `
        -ConformanceFileName "issue52_export_vis_err_trace.log"

    if ($result.ExitCode -ne 1) {
        throw "Case 'issue52_export_vis_err_trace' expected exit 1 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -lt 1) {
        throw "Case 'issue52_export_vis_err_trace' expected at least one compile-time error."
    }

    $diagCount = @($result.DiagJson.diagnostics | Where-Object { $_.code -eq "E-SYS-3353" }).Count
    if ($diagCount -lt 1) {
        throw "Case 'issue52_export_vis_err_trace' expected diagnostic code E-SYS-3353."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $ruleCount = @($logLines | Where-Object { $_ -like "*`tExport-Vis-Err`t*" }).Count
    if ($ruleCount -lt 1) {
        throw "Case 'issue52_export_vis_err_trace' expected Export-Vis-Err in conformance trace."
    }

    Write-Host "[compiler-static] issue52_export_vis_err_trace: exit=$($result.ExitCode) errors=$errorCount e_sys_3353=$diagCount export_vis_err=$ruleCount"
}

function Invoke-Issue52ExportUnknownAbiTraceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue52_export_unknown_abi_trace" `
        -Source (New-Issue52ExportUnknownAbiTraceSource) `
        -ConformanceFileName "issue52_export_unknown_abi_trace.log"

    if ($result.ExitCode -ne 1) {
        throw "Case 'issue52_export_unknown_abi_trace' expected exit 1 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -lt 1) {
        throw "Case 'issue52_export_unknown_abi_trace' expected at least one compile-time error."
    }

    $diagCount = @($result.DiagJson.diagnostics | Where-Object { $_.code -eq "E-SYS-3352" }).Count
    if ($diagCount -lt 1) {
        throw "Case 'issue52_export_unknown_abi_trace' expected diagnostic code E-SYS-3352."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $ruleCount = @($logLines | Where-Object { $_ -like "*`tExportAbi-Unknown-Err`t*" }).Count
    if ($ruleCount -lt 1) {
        throw "Case 'issue52_export_unknown_abi_trace' expected ExportAbi-Unknown-Err in conformance trace."
    }

    Write-Host "[compiler-static] issue52_export_unknown_abi_trace: exit=$($result.ExitCode) errors=$errorCount e_sys_3352=$diagCount export_abi_unknown_err=$ruleCount"
}

function Invoke-Issue52ExternUnknownAbiTraceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue52_extern_unknown_abi_trace" `
        -Source (New-Issue52ExternUnknownAbiSource) `
        -ConformanceFileName "issue52_extern_unknown_abi_trace.log"

    if ($result.ExitCode -ne 1) {
        throw "Case 'issue52_extern_unknown_abi_trace' expected exit 1 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -lt 1) {
        throw "Case 'issue52_extern_unknown_abi_trace' expected at least one compile-time error."
    }

    $diagCount = @($result.DiagJson.diagnostics | Where-Object { $_.code -eq "E-SYS-3352" }).Count
    if ($diagCount -lt 1) {
        throw "Case 'issue52_extern_unknown_abi_trace' expected diagnostic code E-SYS-3352."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $ruleCount = @($logLines | Where-Object { $_ -like "*`tExternAbi-Unknown-Err`t*" }).Count
    if ($ruleCount -lt 1) {
        throw "Case 'issue52_extern_unknown_abi_trace' expected ExternAbi-Unknown-Err in conformance trace."
    }

    Write-Host "[compiler-static] issue52_extern_unknown_abi_trace: exit=$($result.ExitCode) errors=$errorCount e_sys_3352=$diagCount extern_abi_unknown_err=$ruleCount"
}

function Invoke-Issue52ExternGenericErrTraceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue52_extern_generic_err_trace" `
        -Source (New-Issue52ExternGenericErrSource) `
        -ConformanceFileName "issue52_extern_generic_err_trace.log"

    if ($result.ExitCode -ne 1) {
        throw "Case 'issue52_extern_generic_err_trace' expected exit 1 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -lt 1) {
        throw "Case 'issue52_extern_generic_err_trace' expected at least one compile-time error."
    }

    $diagCount = @($result.DiagJson.diagnostics | Where-Object { $_.code -eq "E-TYP-2306" }).Count
    if ($diagCount -lt 1) {
        throw "Case 'issue52_extern_generic_err_trace' expected diagnostic code E-TYP-2306."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $ruleCount = @($logLines | Where-Object { $_ -like "*`tExternProc-Generic-Err`t*" }).Count
    if ($ruleCount -lt 1) {
        throw "Case 'issue52_extern_generic_err_trace' expected ExternProc-Generic-Err in conformance trace."
    }

    Write-Host "[compiler-static] issue52_extern_generic_err_trace: exit=$($result.ExitCode) errors=$errorCount e_typ_2306=$diagCount extern_proc_generic_err=$ruleCount"
}

function Invoke-Issue52ExternByValueErrTraceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue52_extern_by_value_err_trace" `
        -Source (New-Issue52ExternByValueErrSource) `
        -ConformanceFileName "issue52_extern_by_value_err_trace.log"

    if ($result.ExitCode -ne 1) {
        throw "Case 'issue52_extern_by_value_err_trace' expected exit 1 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -lt 1) {
        throw "Case 'issue52_extern_by_value_err_trace' expected at least one compile-time error."
    }

    $diagCount = @($result.DiagJson.diagnostics | Where-Object { $_.code -eq "E-TYP-2630" }).Count
    if ($diagCount -lt 1) {
        throw "Case 'issue52_extern_by_value_err_trace' expected diagnostic code E-TYP-2630."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $ruleCount = @($logLines | Where-Object { $_ -like "*`tExternProc-ByValue-Err`t*" }).Count
    if ($ruleCount -lt 1) {
        throw "Case 'issue52_extern_by_value_err_trace' expected ExternProc-ByValue-Err in conformance trace."
    }

    Write-Host "[compiler-static] issue52_extern_by_value_err_trace: exit=$($result.ExitCode) errors=$errorCount e_typ_2630=$diagCount extern_proc_byvalue_err=$ruleCount"
}

function Invoke-Issue52WfExternBlockTraceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue52_wf_extern_block_trace" `
        -Source (New-Issue52ExternWfTraceSource) `
        -ConformanceFileName "issue52_wf_extern_block_trace.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue52_wf_extern_block_trace' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue52_wf_extern_block_trace' expected zero compile-time errors but observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $externProcCount = @($logLines | Where-Object { $_ -like "*`tWF-ExternProcDecl`t*" }).Count
    $externBlockCount = @($logLines | Where-Object { $_ -like "*`tWF-ExternBlock`t*" }).Count
    if ($externProcCount -lt 1 -or $externBlockCount -lt 1) {
        throw "Case 'issue52_wf_extern_block_trace' expected WF-ExternProcDecl and WF-ExternBlock in conformance trace."
    }

    Write-Host "[compiler-static] issue52_wf_extern_block_trace: exit=$($result.ExitCode) errors=$errorCount wf_extern_proc_decl=$externProcCount wf_extern_block=$externBlockCount"
}

function Invoke-Issue544ExternItemListParseTraceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue544_extern_item_list_parse_trace" `
        -Source (New-Issue544ExternItemListParseTraceSource) `
        -ConformanceFileName "issue544_extern_item_list_parse_trace.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue544_extern_item_list_parse_trace' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue544_extern_item_list_parse_trace' expected zero compile-time errors but observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $consCount = @($logLines | Where-Object { $_ -like "*`tParse-ExternItemList-Cons`t*" }).Count
    $endCount = @($logLines | Where-Object { $_ -like "*`tParse-ExternItemList-End`t*" }).Count
    if ($consCount -ne 2 -or $endCount -ne 1) {
        throw "Case 'issue544_extern_item_list_parse_trace' expected exactly two Parse-ExternItemList-Cons traces and one Parse-ExternItemList-End trace (cons=$consCount end=$endCount)."
    }

    $parserPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\item\\extern_block.cpp"
    if (-not (Test-Path $parserPath)) {
        throw "Case 'issue544_extern_item_list_parse_trace' missing parser implementation file: $parserPath"
    }

    $parserText = Get-Content -Path $parserPath -Raw
    $requiredPatterns = @(
        'struct\s+ExternItemListResult',
        'ExternItemListResult\s+ParseExternItemList\s*\(Parser\s+parser\)',
        'SPEC_RULE\("Parse-ExternItemList-Cons"\)',
        'SPEC_RULE\("Parse-ExternItemList-End"\)',
        'ExternItemListResult\s+items\s*=\s*ParseExternItemList\(parser\)'
    )
    foreach ($pattern in $requiredPatterns) {
        if ($parserText -notmatch $pattern) {
            throw "Case 'issue544_extern_item_list_parse_trace' missing expected pattern '$pattern' in $parserPath."
        }
    }

    Write-Host "[compiler-static] issue544_extern_item_list_parse_trace: exit=$($result.ExitCode) errors=$errorCount cons=$consCount end=$endCount wiring_checks=$($requiredPatterns.Count)"
}

function Invoke-Issue52SynCallErrTraceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue52_syn_call_err_trace" `
        -Source (New-Issue52SynCallErrSource) `
        -ConformanceFileName "issue52_syn_call_err_trace.log"

    if ($result.ExitCode -ne 1) {
        throw "Case 'issue52_syn_call_err_trace' expected exit 1 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -lt 1) {
        throw "Case 'issue52_syn_call_err_trace' expected at least one compile-time error."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $ruleCount = @($logLines | Where-Object { $_ -like "*`tSyn-Call-Err`t*" }).Count
    if ($ruleCount -lt 1) {
        throw "Case 'issue52_syn_call_err_trace' expected Syn-Call-Err in conformance trace."
    }

    Write-Host "[compiler-static] issue52_syn_call_err_trace: exit=$($result.ExitCode) errors=$errorCount syn_call_err=$ruleCount"
}

function Invoke-Issue52WiringGapCase {
    $presentChecks = @(
        @{ Path = "cursive\\src\\04_analysis\\memory\\calls.cpp"; Pattern = 'SPEC_DEF\("Call-Arg-Packed-Unsafe-Err", "5\.2\.4"\)' },
        @{ Path = "cursive\\src\\04_analysis\\memory\\calls.cpp"; Pattern = 'SPEC_RULE\("Call-Arg-Packed-Unsafe-Err"\)' },
        @{ Path = "cursive\\include\\04_analysis\\typing\\types.h"; Pattern = 'struct\s+TypeVar' },
        @{ Path = "cursive\\include\\04_analysis\\typing\\types.h"; Pattern = 'MakeTypeVar\s*\(' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\expr\\call_type_args.cpp"; Pattern = 'type_args\.size\(\)\s*>\s*params\.size\(\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\expr\\index_access.cpp"; Pattern = 'SPEC_RULE\("T-Index-Array-Dynamic"\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\expr\\index_access.cpp"; Pattern = 'SPEC_RULE\("T-Index-Array-Perm-Dynamic"\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\expr\\index_access.cpp"; Pattern = 'SPEC_RULE\("P-Index-Array-Dynamic"\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\expr\\index_access.cpp"; Pattern = 'SPEC_RULE\("P-Index-Slice"\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\expr\\index_access.cpp"; Pattern = 'SPEC_RULE\("P-Slice-From-Array"\)' },
        @{ Path = "cursive\\include\\04_analysis\\typing\\type_infer.h"; Pattern = 'using\s+TypeSubstitution' },
        @{ Path = "cursive\\include\\04_analysis\\typing\\type_infer.h"; Pattern = 'struct\s+SolveResult' },
        @{ Path = "cursive\\include\\04_analysis\\typing\\type_infer.h"; Pattern = 'Solve\s*\(const\s+ScopeContext&\s+ctx,\s*const\s+ConstraintSet&\s+constraints\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\type_infer.cpp"; Pattern = 'SPEC_RULE\("Solve"\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\expr\\if_expr.cpp"; Pattern = 'SPEC_DEF\("T-If", "5\.2\.12"\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\expr\\if_expr.cpp"; Pattern = 'SPEC_RULE\("T-If"\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\expr\\if_case_expr.cpp"; Pattern = 'SPEC_DEF\("T-IfIs", "5\.2\.13"\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\stmt\\stmt_common.cpp"; Pattern = 'SPEC_DEF\("WarnResultUnreachable", "5\.2\.11"\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\stmt\\let_stmt.cpp"; Pattern = 'Solve\(ctx,\s*constraints\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\stmt\\var_stmt.cpp"; Pattern = 'Solve\(ctx,\s*constraints\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\stmt\\using_local_stmt.cpp"; Pattern = 'SPEC_RULE\("T-UsingLocalStmt"\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\expr\\field_access.cpp"; Pattern = 'SPEC_DEF\("T-Field-Record", "5\.2\.12"\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\expr\\field_access.cpp"; Pattern = 'SPEC_RULE\("FieldAccess-Enum"\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\expr\\range.cpp"; Pattern = 'SPEC_RULE\("T-Range-Lift"\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\expr\\transmute_expr.cpp"; Pattern = 'SPEC_RULE\("W-Transmute-Invalid-Target"\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\expr\\propagate_expr.cpp"; Pattern = 'SPEC_RULE\("T-Async-Try"\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\expr\\propagate_expr.cpp"; Pattern = 'result\.diag_id\s*=\s*"E-CON-0230"' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\type_wf.cpp"; Pattern = 'SPEC_DEF\("WF-Union", "5\.2\.7"\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\item\\procedure_decl.cpp"; Pattern = 'SPEC_RULE\("WF-ProcedureDecl"\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\item\\procedure_decl.cpp"; Pattern = 'SPEC_RULE\("WF-ProcBody-ExplicitReturn-Err"\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\item\\procedure_decl.cpp"; Pattern = 'SPEC_RULE\("Export-Vis-Err"\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\item\\procedure_decl.cpp"; Pattern = 'SPEC_RULE\("ExportAbi-Unknown-Err"\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\item\\procedure_decl.cpp"; Pattern = 'SPEC_RULE\("Export-ByValue-Err"\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\item\\procedure_decl.cpp"; Pattern = '\"04_analysis/typing/ffi_by_value.h\"' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\item\\extern_block.cpp"; Pattern = 'SPEC_RULE\("WF-ExternProcDecl"\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\item\\extern_block.cpp"; Pattern = 'SPEC_RULE\("WF-ExternBlock"\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\item\\extern_block.cpp"; Pattern = 'SPEC_RULE\("ExternAbi-Unknown-Err"\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\item\\extern_block.cpp"; Pattern = 'SPEC_RULE\("ExternProc-Generic-Err"\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\item\\extern_block.cpp"; Pattern = 'SPEC_RULE\("ExternProc-ByValue-Err"\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\item\\extern_block.cpp"; Pattern = 'FfiByValueOk\(ctx,\s*lowered\.type\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\item\\extern_block.cpp"; Pattern = '\"04_analysis/typing/ffi_by_value.h\"' },
        @{ Path = "cursive\\include\\04_analysis\\typing\\ffi_by_value.h"; Pattern = 'bool\s+FfiByValueOk\s*\(' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\ffi_by_value.cpp"; Pattern = 'bool\s+FfiByValueOk\s*\(' },
        @{ Path = "cursive\\src\\CMakeLists.txt"; Pattern = '04_analysis/typing/ffi_by_value\.cpp' },
        @{ Path = "cursive\\src\\04_analysis\\memory\\borrow_bind.cpp"; Pattern = 'SPEC_RULE\("B-Pipeline"\)' },
        @{ Path = "cursive\\src\\04_analysis\\memory\\borrow_bind.cpp"; Pattern = 'IsModalTransitionCall' },
        @{ Path = "cursive\\src\\04_analysis\\memory\\regions.cpp"; Pattern = 'SPEC_RULE\("P-Pipeline"\)' },
        @{ Path = "cursive\\src\\04_analysis\\memory\\regions.cpp"; Pattern = 'WarnAsyncCapture' },
        @{ Path = "cursive\\src\\04_analysis\\memory\\regions.cpp"; Pattern = 'SPEC_RULE\("Warn-Async-LargeCapture"\)' },
        @{ Path = "cursive\\include\\04_analysis\\conformance\\conformance.h"; Pattern = 'CheckTypeSystemMetatheoryHooks' },
        @{ Path = "cursive\\include\\04_analysis\\conformance\\conformance.h"; Pattern = 'bool\s+progress\s*=\s*false;' },
        @{ Path = "cursive\\include\\04_analysis\\conformance\\conformance.h"; Pattern = 'bool\s+preservation\s*=\s*false;' },
        @{ Path = "cursive\\src\\04_analysis\\conformance\\conformance.cpp"; Pattern = 'SPEC_DEF\("Progress", "5\.2\.18"\)' },
        @{ Path = "cursive\\src\\04_analysis\\conformance\\conformance.cpp"; Pattern = 'SPEC_RULE\("Progress"\)\s*;\s*result\.progress\s*=\s*true;' },
        @{ Path = "cursive\\src\\04_analysis\\conformance\\conformance.cpp"; Pattern = 'SPEC_RULE\("Preservation"\)\s*;\s*result\.preservation\s*=\s*true;' },
        @{ Path = "cursive\\src\\04_analysis\\conformance\\conformance.cpp"; Pattern = 'SPEC_RULE\("No-Use-After-Free"\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\typecheck.cpp"; Pattern = 'CheckTypeSystemMetatheoryHooks\(ctx\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\type_infer.cpp"; Pattern = 'SPEC_RULE\("Syn-Call-Err"\)' }
    )

    foreach ($check in $presentChecks) {
        $fullPath = Join-Path $workspaceRoot $check.Path
        if (-not (Test-Path $fullPath)) {
            throw "Case 'issue52_wiring_gap' missing file: $fullPath"
        }
        $text = Get-Content -Path $fullPath -Raw
        if ($text -notmatch $check.Pattern) {
            throw "Case 'issue52_wiring_gap' missing expected pattern '$($check.Pattern)' in $fullPath."
        }
    }

    $analysisRoot = Join-Path $workspaceRoot "cursive\\src\\04_analysis"
    $unionIntroRefs = @(
        Get-ChildItem -Path $analysisRoot -Recurse -File -Include *.cpp |
            Select-String -Pattern 'TypeUnionIntro\('
    )
    $activeUnionIntroRefs = @($unionIntroRefs | Where-Object {
        $_.Path -notlike "*composite\\unions.cpp"
    })
    if ($activeUnionIntroRefs.Count -lt 1) {
        throw "Case 'issue52_wiring_gap' expected active TypeUnionIntro call sites outside composite\\unions.cpp after remediation."
    }

    Write-Host "[compiler-static] issue52_wiring_gap: present_checks=$($presentChecks.Count) active_union_intro_refs=$($activeUnionIntroRefs.Count)"
}

function Invoke-Issue52MetatheoryHooksCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue52_metatheory_hooks" `
        -Source (New-MinimalMainSource) `
        -ConformanceFileName "issue52_metatheory_hooks.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue52_metatheory_hooks' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue52_metatheory_hooks' expected zero compile-time errors but observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $metatheoryRules = @(
        "Progress",
        "Preservation",
        "No-Use-After-Free",
        "No-Double-Free",
        "No-Dangling-Pointers",
        "Permission-Preservation",
        "State-Determinism",
        "No-Resurrection",
        "Data-Race-Freedom",
        "Fork-Join-Guarantee",
        "Key-Serialization",
        "Async-Key-Safety"
    )

    foreach ($rule in $metatheoryRules) {
        $count = @($logLines | Where-Object { $_ -like "*`t$rule`t*" }).Count
        if ($count -lt 1) {
            throw "Case 'issue52_metatheory_hooks' expected '$rule' in conformance trace."
        }
    }

    $progressCount = @($logLines | Where-Object { $_ -like "*`tProgress`t*" }).Count
    if ($progressCount -ne 1) {
        throw "Case 'issue52_metatheory_hooks' expected exactly one Progress trace but observed $progressCount."
    }

    $preservationCount = @($logLines | Where-Object { $_ -like "*`tPreservation`t*" }).Count
    if ($preservationCount -ne 1) {
        throw "Case 'issue52_metatheory_hooks' expected exactly one Preservation trace but observed $preservationCount."
    }

    Write-Host "[compiler-static] issue52_metatheory_hooks: exit=$($result.ExitCode) errors=$errorCount rules=$($metatheoryRules.Count)"
}

function Invoke-Issue561RulePremisesRegistryConformanceCase {
    $caseRoot = Join-Path $workRoot "issue561_rule_premises_registry_conformance"
    New-Item -ItemType Directory -Path $caseRoot -Force | Out-Null

    $generatorPath = Join-Path $workspaceRoot "cursive\\tools\\generate_static_rule_registry.ps1"
    $mappingPath = Join-Path $workspaceRoot "cursive\\tools\\static_rule_mapping.json"
    $registryOutPath = Join-Path $caseRoot "static_rule_registry.inc"
    $reportOutPath = Join-Path $caseRoot "static_rule_registry_report.json"

    if (-not (Test-Path $generatorPath)) {
        throw "Case 'issue561_rule_premises_registry_conformance' missing generator script: $generatorPath"
    }
    if (-not (Test-Path $mappingPath)) {
        throw "Case 'issue561_rule_premises_registry_conformance' missing mapping file: $mappingPath"
    }

    & powershell -NoProfile -ExecutionPolicy Bypass -File $generatorPath -RepoRoot $workspaceRoot -MappingPath $mappingPath -OutputPath $registryOutPath -ReportPath $reportOutPath -Strict
    $generatorExit = $LASTEXITCODE
    if ($generatorExit -ne 0) {
        throw "Case 'issue561_rule_premises_registry_conformance' expected registry generator exit 0 but got $generatorExit."
    }
    if (-not (Test-Path $registryOutPath)) {
        throw "Case 'issue561_rule_premises_registry_conformance' missing generated registry: $registryOutPath"
    }

    $registryText = Get-Content -Path $registryOutPath -Raw
    $expectedRegistryPatterns = @(
        '\{"Reject-IllFormed",\s*"DeclJudg",\s*std::nullopt,\s*"04_analysis/conformance/conformance\.cpp",\s*std::string_view\("¬ Conforming\(P\)"\)\}',
        '\{"Static-Undefined",\s*"DeclJudg",\s*std::nullopt,\s*"00_core/behavior_model\.cpp",\s*std::string_view\("StaticUndefined\(J\)\\nCode\(DiagIdOf\(J\)\) = c"\)\}',
        '\{"WF-Span",\s*"WFModulePathJudg",\s*std::nullopt,\s*"00_core/span\.cpp",\s*std::string_view\("0 ≤ s ≤ e ≤ S\.byte_len\\nΓ ⊢ Locate\(S, s\) ⇓ ℓ_s\\nΓ ⊢ Locate\(S, e\) ⇓ ℓ_e"\)\}'
    )
    foreach ($pattern in $expectedRegistryPatterns) {
        if ($registryText -notmatch $pattern) {
            throw "Case 'issue561_rule_premises_registry_conformance' missing expected premise registry pattern '$pattern'."
        }
    }

    $headerPath = Join-Path $workspaceRoot "cursive\\include\\00_core\\behavior_model.h"
    $implPath = Join-Path $workspaceRoot "cursive\\src\\00_core\\behavior_model.cpp"
    if (-not (Test-Path $headerPath)) {
        throw "Case 'issue561_rule_premises_registry_conformance' missing behavior model header: $headerPath"
    }
    if (-not (Test-Path $implPath)) {
        throw "Case 'issue561_rule_premises_registry_conformance' missing behavior model implementation: $implPath"
    }

    $headerText = Get-Content -Path $headerPath -Raw
    if ($headerText -notmatch 'PremisesOfRule\s*\(\s*std::string_view\s+rule_id\s*\)') {
        throw "Case 'issue561_rule_premises_registry_conformance' expected PremisesOfRule declaration in behavior_model.h."
    }

    $implText = Get-Content -Path $implPath -Raw
    foreach ($pattern in @(
        'SPEC_DEF\("Premises", "1\.2"\)',
        'std::optional<std::vector<std::string_view>>\s+PremisesOfRule',
        'meta->premises_text',
        'SplitPremises'
    )) {
        if ($implText -notmatch $pattern) {
            throw "Case 'issue561_rule_premises_registry_conformance' missing expected behavior model pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue561_rule_premises_registry_conformance: generator=1 registry_patterns=$($expectedRegistryPatterns.Count) api_patterns=4"
}

function Invoke-Issue562UnicodeScalarDomainConformanceCase {
    $headerPath = Join-Path $workspaceRoot "cursive\\include\\00_core\\source_text.h"
    $unicodeImplPath = Join-Path $workspaceRoot "cursive\\src\\00_core\\unicode.cpp"
    $literalImplPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\lexer_literals.cpp"

    foreach ($path in @($headerPath, $unicodeImplPath, $literalImplPath)) {
        if (-not (Test-Path $path)) {
            throw "Case 'issue562_unicode_scalar_domain_conformance' missing compiler source file: $path"
        }
    }

    $headerText = Get-Content -Path $headerPath -Raw
    $unicodeImplText = Get-Content -Path $unicodeImplPath -Raw
    $literalImplText = Get-Content -Path $literalImplPath -Raw

    $headerPatterns = @(
        'class\s+UnicodeScalar\s*\{',
        'constexpr\s+UnicodeScalar\s*\(\s*std::uint32_t\s+value\s*\)\s*:\s*value_\(Validate\(value\)\)',
        'static\s+constexpr\s+bool\s+IsValue\s*\(\s*std::uint32_t\s+value\s*\)',
        'constexpr\s+operator\s+std::uint32_t\s*\(\s*\)\s+const',
        'static\s+constexpr\s+std::uint32_t\s+Validate\s*\(\s*std::uint32_t\s+value\s*\)'
    )
    foreach ($pattern in $headerPatterns) {
        if ($headerText -notmatch $pattern) {
            throw "Case 'issue562_unicode_scalar_domain_conformance' missing expected source_text.h pattern '$pattern'."
        }
    }
    if ($headerText -match 'using\s+UnicodeScalar\s*=\s*std::uint32_t\s*;') {
        throw "Case 'issue562_unicode_scalar_domain_conformance' found legacy raw UnicodeScalar alias in source_text.h."
    }

    foreach ($impl in @(
        @{ Label = "unicode.cpp"; Text = $unicodeImplText },
        @{ Label = "lexer_literals.cpp"; Text = $literalImplText }
    )) {
        if ($impl.Text -notmatch 'UnicodeScalar::IsValue\s*\(\s*value\s*\)') {
            throw "Case 'issue562_unicode_scalar_domain_conformance' expected $($impl.Label) to route scalar-domain checks through UnicodeScalar::IsValue."
        }
    }

    Write-Host "[compiler-static] issue562_unicode_scalar_domain_conformance: header_patterns=$($headerPatterns.Count) impl_checks=2"
}

function Invoke-Issue563ScalarsSequenceConformanceCase {
    $headerPath = Join-Path $workspaceRoot "cursive\\include\\00_core\\source_text.h"
    if (-not (Test-Path $headerPath)) {
        throw "Case 'issue563_scalars_sequence_conformance' missing compiler source file: $headerPath"
    }

    $headerText = Get-Content -Path $headerPath -Raw
    $requiredPatterns = @(
        'struct\s+DecodeResult\s*\{\s*std::vector<UnicodeScalar>\s+scalars;',
        'struct\s+StripBOMResult\s*\{\s*std::vector<UnicodeScalar>\s+scalars;',
        'StripBOMResult\s+StripBOM\s*\(\s*const\s+std::vector<UnicodeScalar>&\s+scalars\s*\);',
        'std::vector<UnicodeScalar>\s+NormalizeLF\s*\(\s*const\s+std::vector<UnicodeScalar>&\s+scalars\s*\);',
        'struct\s+SourceFile\s*\{[\s\S]*std::vector<UnicodeScalar>\s+scalars;'
    )
    foreach ($pattern in $requiredPatterns) {
        if ($headerText -notmatch $pattern) {
            throw "Case 'issue563_scalars_sequence_conformance' missing expected source_text.h pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue563_scalars_sequence_conformance: required_patterns=$($requiredPatterns.Count)"
}

function Invoke-Issue564StringAliasConformanceCase {
    $headerPath = Join-Path $workspaceRoot "cursive\\include\\00_core\\source_text.h"
    if (-not (Test-Path $headerPath)) {
        throw "Case 'issue564_string_alias_conformance' missing compiler source file: $headerPath"
    }

    $headerText = Get-Content -Path $headerPath -Raw
    foreach ($pattern in @(
        'using\s+Scalars\s*=\s*std::vector<UnicodeScalar>\s*;',
        'using\s+String\s*=\s*Scalars\s*;'
    )) {
        if ($headerText -notmatch $pattern) {
            throw "Case 'issue564_string_alias_conformance' missing expected source_text.h pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue564_string_alias_conformance: alias_patterns=2"
}

function Invoke-Issue565NormalizeOutsideIdentifiersConformanceCase {
    $headerPath = Join-Path $workspaceRoot "cursive\\include\\00_core\\source_text.h"
    $loadImplPath = Join-Path $workspaceRoot "cursive\\src\\00_core\\source_load.cpp"
    foreach ($path in @($headerPath, $loadImplPath)) {
        if (-not (Test-Path $path)) {
            throw "Case 'issue565_normalize_outside_identifiers_conformance' missing compiler source file: $path"
        }
    }

    $headerText = Get-Content -Path $headerPath -Raw
    $loadImplText = Get-Content -Path $loadImplPath -Raw

    if ($headerText -notmatch 'inline\s+Scalars\s+NormalizeOutsideIdentifiers\s*\(\s*const\s+Scalars&\s+scalars\s*\)\s*\{\s*return\s+scalars;\s*\}') {
        throw "Case 'issue565_normalize_outside_identifiers_conformance' missing NormalizeOutsideIdentifiers identity helper in source_text.h."
    }
    if ($loadImplText -notmatch 'NormalizeLF\s*\(\s*NormalizeOutsideIdentifiers\s*\(\s*stripped\.scalars\s*\)\s*\)') {
        throw "Case 'issue565_normalize_outside_identifiers_conformance' expected LoadSource to route normalization through NormalizeOutsideIdentifiers before NormalizeLF."
    }

    Write-Host "[compiler-static] issue565_normalize_outside_identifiers_conformance: header_identity=1 loadsource_pipeline=1"
}

function Invoke-Issue566SourceScalarsProjectionConformanceCase {
    $headerPath = Join-Path $workspaceRoot "cursive\\include\\00_core\\source_text.h"
    $loadImplPath = Join-Path $workspaceRoot "cursive\\src\\00_core\\source_load.cpp"
    $tokenizePath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\tokenize.cpp"
    $lexerPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\lexer.cpp"
    foreach ($path in @($headerPath, $loadImplPath, $tokenizePath, $lexerPath)) {
        if (-not (Test-Path $path)) {
            throw "Case 'issue566_source_scalars_projection_conformance' missing compiler source file: $path"
        }
    }

    $headerText = Get-Content -Path $headerPath -Raw
    $loadImplText = Get-Content -Path $loadImplPath -Raw
    $tokenizeText = Get-Content -Path $tokenizePath -Raw
    $lexerText = Get-Content -Path $lexerPath -Raw

    if ($headerText -notmatch 'struct\s+SourceFile\s*\{[\s\S]*std::vector<UnicodeScalar>\s+scalars;') {
        throw "Case 'issue566_source_scalars_projection_conformance' expected SourceFile to carry a scalars field."
    }
    if ($loadImplText -notmatch 'source\.scalars\s*=\s*std::move\(scalars\);') {
        throw "Case 'issue566_source_scalars_projection_conformance' expected BuildSpanSource to assign the decoded scalar sequence into source.scalars."
    }
    if ($tokenizeText -notmatch 'const\s+auto&\s+scalars\s*=\s*source\.scalars;') {
        throw "Case 'issue566_source_scalars_projection_conformance' expected tokenize.cpp to project T from source.scalars."
    }
    if ($lexerText -notmatch 'const\s+auto&\s+scalars\s*=\s*source\.scalars;') {
        throw "Case 'issue566_source_scalars_projection_conformance' expected lexer.cpp to project T from source.scalars."
    }

    Write-Host "[compiler-static] issue566_source_scalars_projection_conformance: sourcefile_field=1 buildspansource_assign=1 tokenize_projection=1 lexer_projection=1"
}

function Invoke-Issue567LexSensitivePosConformanceCase {
    $headerPath = Join-Path $workspaceRoot "cursive\\include\\02_source\\lexer\\lexer.h"
    $securityImplPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\lexer_security.cpp"
    $tokenizePath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\tokenize.cpp"
    foreach ($path in @($headerPath, $securityImplPath, $tokenizePath)) {
        if (-not (Test-Path $path)) {
            throw "Case 'issue567_lex_sensitive_pos_conformance' missing compiler source file: $path"
        }
    }

    $headerText = Get-Content -Path $headerPath -Raw
    $securityImplText = Get-Content -Path $securityImplPath -Raw
    $tokenizeText = Get-Content -Path $tokenizePath -Raw

    if ($headerText -notmatch 'std::vector<std::size_t>\s+LexSensitivePos\s*\(\s*const\s+core::SourceFile&\s+source\s*\);') {
        throw "Case 'issue567_lex_sensitive_pos_conformance' missing LexSensitivePos declaration in lexer.h."
    }
    foreach ($pattern in @(
        'std::vector<std::size_t>\s+LexSensitivePos\s*\(\s*const\s+core::SourceFile&\s+source\s*\)',
        'ScanLineComment\(source,\s*i\)',
        'ScanBlockComment\(source,\s*i\)',
        'ScanStringLiteral\(source,\s*i\)',
        'ScanCharLiteral\(source,\s*i\)',
        'core::IsSensitive\(scalars\[i\]\)',
        'sensitive\.push_back\(i\)'
    )) {
        if ($securityImplText -notmatch $pattern) {
            throw "Case 'issue567_lex_sensitive_pos_conformance' missing expected lexer_security.cpp pattern '$pattern'."
        }
    }
    if ($tokenizeText -notmatch 'LexSecure\s*\(\s*source,\s*lexed\.output\.tokens,\s*LexSensitivePos\(source\)\s*\)') {
        throw "Case 'issue567_lex_sensitive_pos_conformance' expected Tokenize to call LexSecure with LexSensitivePos(source)."
    }

    Write-Host "[compiler-static] issue567_lex_sensitive_pos_conformance: header_decl=1 security_patterns=7 tokenize_call=1"
}

function Invoke-Issue568LiteralSpanConformanceCase {
    $unicodeImplPath = Join-Path $workspaceRoot "cursive\\src\\00_core\\unicode.cpp"
    if (-not (Test-Path $unicodeImplPath)) {
        throw "Case 'issue568_literal_span_conformance' missing compiler source file: $unicodeImplPath"
    }

    $unicodeImplText = Get-Content -Path $unicodeImplPath -Raw
    foreach ($pattern in @(
        'std::vector<ByteSpan>\s+LiteralSpan\s*\(\s*const\s+std::vector<UnicodeScalar>&\s+scalars\s*\)',
        'const\s+auto\s+offsets\s*=\s*Utf8Offsets\(scalars\);',
        'const\s+auto\s+spans\s*=\s*LiteralSpan\(scalars\);',
        'ByteInLiteralSpan\(offsets\[i\],\s*spans,\s*&span_index\)'
    )) {
        if ($unicodeImplText -notmatch $pattern) {
            throw "Case 'issue568_literal_span_conformance' missing expected unicode.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue568_literal_span_conformance: unicode_patterns=4"
}

function Invoke-Issue569TokenizePartialSurfaceConformanceCase {
    $headerPath = Join-Path $workspaceRoot "cursive\\include\\02_source\\lexer\\lexer.h"
    $tokenizeImplPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\tokenize.cpp"
    $parserPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\parser.cpp"
    foreach ($path in @($headerPath, $tokenizeImplPath, $parserPath)) {
        if (-not (Test-Path $path)) {
            throw "Case 'issue569_tokenize_partial_surface_conformance' missing compiler source file: $path"
        }
    }

    $headerText = Get-Content -Path $headerPath -Raw
    $tokenizeImplText = Get-Content -Path $tokenizeImplPath -Raw
    $parserText = Get-Content -Path $parserPath -Raw

    foreach ($pattern in @(
        'using\s+TokenizeResult\s*=\s*std::optional<LexerOutput>\s*;',
        'struct\s+TokenizeDiagnosticResult\s*\{[\s\S]*std::optional<LexerOutput>\s+output;[\s\S]*core::DiagnosticStream\s+diags;',
        'TokenizeResult\s+Tokenize\s*\(\s*const\s+core::SourceFile&\s+source\s*\);',
        'TokenizeDiagnosticResult\s+TokenizeWithDiagnostics\s*\(\s*const\s+core::SourceFile&\s+source\s*\);'
    )) {
        if ($headerText -notmatch $pattern) {
            throw "Case 'issue569_tokenize_partial_surface_conformance' missing expected lexer.h pattern '$pattern'."
        }
    }
    if ($tokenizeImplText -notmatch 'TokenizeResult\s+Tokenize\s*\(\s*const\s+core::SourceFile&\s+source\s*\)\s*\{\s*return\s+TokenizeWithDiagnostics\(source\)\.output;\s*\}') {
        throw "Case 'issue569_tokenize_partial_surface_conformance' expected Tokenize to return only the partial output of TokenizeWithDiagnostics."
    }
    if ($parserText -notmatch 'TokenizeDiagnosticResult\s+tok\s*=\s*TokenizeWithDiagnostics\(source\);') {
        throw "Case 'issue569_tokenize_partial_surface_conformance' expected ParseFile to use TokenizeWithDiagnostics for diagnostics-preserving tokenization."
    }

    Write-Host "[compiler-static] issue569_tokenize_partial_surface_conformance: header_patterns=4 tokenize_wrapper=1 parser_wrapper=1"
}

function Invoke-Issue570RequiredTerminatorSurfaceConformanceCase {
    $headerPath = Join-Path $workspaceRoot "cursive\\include\\02_source\\lexer\\lexer.h"
    $lexerImplPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\lexer.cpp"
    foreach ($path in @($headerPath, $lexerImplPath)) {
        if (-not (Test-Path $path)) {
            throw "Case 'issue570_required_terminator_surface_conformance' missing compiler source file: $path"
        }
    }

    $headerText = Get-Content -Path $headerPath -Raw
    $lexerImplText = Get-Content -Path $lexerImplPath -Raw

    if ($headerText -notmatch 'bool\s+RequiredTerminator\s*\(\s*const\s+std::vector<Token>&\s+tokens,\s*std::size_t\s+index\s*\);') {
        throw "Case 'issue570_required_terminator_surface_conformance' missing RequiredTerminator declaration in lexer.h."
    }
    foreach ($pattern in @(
        'struct\s+NewlineContext',
        'NewlineContext\s+BuildNewlineContext\s*\(\s*const\s+std::vector<Token>&\s+tokens\s*\)',
        'bool\s+RequiredTerminatorImpl\s*\(\s*const\s+std::vector<Token>&\s+tokens,\s*std::size_t\s+i,\s*const\s+NewlineContext&\s+ctx\s*\)',
        'tokens\[i\]\.kind\s*==\s*TokenKind::Newline',
        '!ContinuesLineImpl\(tokens,\s*i,\s*ctx\)',
        'if\s*\(\s*RequiredTerminatorImpl\(tokens,\s*i,\s*ctx\)\s*\)\s*\{\s*out\.push_back\(tok\);',
        'bool\s+RequiredTerminator\s*\(\s*const\s+std::vector<Token>&\s+tokens,\s*std::size_t\s+index\s*\)\s*\{\s*return\s+RequiredTerminatorImpl\(tokens,\s*index,\s*BuildNewlineContext\(tokens\)\);\s*\}'
    )) {
        if ($lexerImplText -notmatch $pattern) {
            throw "Case 'issue570_required_terminator_surface_conformance' missing expected lexer.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue570_required_terminator_surface_conformance: header_decl=1 lexer_patterns=7"
}

function Invoke-Issue571ContinuesLineSurfaceConformanceCase {
    $headerPath = Join-Path $workspaceRoot "cursive\\include\\02_source\\lexer\\lexer.h"
    $lexerImplPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\lexer.cpp"
    foreach ($path in @($headerPath, $lexerImplPath)) {
        if (-not (Test-Path $path)) {
            throw "Case 'issue571_continues_line_surface_conformance' missing compiler source file: $path"
        }
    }

    $headerText = Get-Content -Path $headerPath -Raw
    $lexerImplText = Get-Content -Path $lexerImplPath -Raw

    if ($headerText -notmatch 'bool\s+ContinuesLine\s*\(\s*const\s+std::vector<Token>&\s+tokens,\s*std::size_t\s+index\s*\);') {
        throw "Case 'issue571_continues_line_surface_conformance' missing ContinuesLine declaration in lexer.h."
    }
    foreach ($pattern in @(
        'bool\s+ContinuesLineImpl\s*\(\s*const\s+std::vector<Token>&\s+tokens,\s*std::size_t\s+i,\s*const\s+NewlineContext&\s+ctx\s*\)',
        'tokens\[i\]\.kind\s*!=\s*TokenKind::Newline',
        'bool\s+ContinuesLine\s*\(\s*const\s+std::vector<Token>&\s+tokens,\s*std::size_t\s+index\s*\)\s*\{\s*return\s+ContinuesLineImpl\(tokens,\s*index,\s*BuildNewlineContext\(tokens\)\);\s*\}',
        'RequiredTerminatorImpl\(tokens,\s*i,\s*ctx\)'
    )) {
        if ($lexerImplText -notmatch $pattern) {
            throw "Case 'issue571_continues_line_surface_conformance' missing expected lexer.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue571_continues_line_surface_conformance: header_decl=1 lexer_patterns=4"
}

function Invoke-Issue572SourceLoadStateConformanceCase {
    $headerPath = Join-Path $workspaceRoot "cursive\\include\\00_core\\source_load.h"
    if (-not (Test-Path $headerPath)) {
        throw "Case 'issue572_source_load_state_conformance' missing compiler source file: $headerPath"
    }

    $headerText = Get-Content -Path $headerPath -Raw
    foreach ($pattern in @(
        'struct\s+SourceLoadStartState\s*\{[\s\S]*std::string\s+path;[\s\S]*std::vector<std::uint8_t>\s+bytes;',
        'struct\s+SourceLoadSizedState\s*\{[\s\S]*std::string\s+path;[\s\S]*std::vector<std::uint8_t>\s+bytes;',
        'struct\s+SourceLoadDecodedState\s*\{[\s\S]*std::string\s+path;[\s\S]*std::vector<std::uint8_t>\s+bytes;[\s\S]*Scalars\s+scalars;',
        'struct\s+SourceLoadBomStrippedState\s*\{[\s\S]*bool\s+had_bom\s*=\s*false;[\s\S]*std::optional<std::size_t>\s+j;',
        'struct\s+SourceLoadNormalizedState\s*\{[\s\S]*Scalars\s+scalars;[\s\S]*std::optional<std::size_t>\s+j;',
        'struct\s+SourceLoadLineMappedState\s*\{[\s\S]*Scalars\s+scalars;[\s\S]*std::vector<std::size_t>\s+line_starts;',
        'struct\s+SourceLoadValidatedState\s*\{[\s\S]*SourceFile\s+source;',
        'struct\s+SourceLoadErrorState\s*\{[\s\S]*std::string\s+code;',
        'using\s+SourceLoadState\s*=\s*std::variant<[\s\S]*SourceLoadStartState[\s\S]*SourceLoadSizedState[\s\S]*SourceLoadDecodedState[\s\S]*SourceLoadBomStrippedState[\s\S]*SourceLoadNormalizedState[\s\S]*SourceLoadLineMappedState[\s\S]*SourceLoadValidatedState[\s\S]*SourceLoadErrorState[\s\S]*>;'
    )) {
        if ($headerText -notmatch $pattern) {
            throw "Case 'issue572_source_load_state_conformance' missing expected source_load.h pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue572_source_load_state_conformance: header_patterns=9"
}

function Invoke-Issue573StepSizeTransitionConformanceCase {
    $sourcePath = Join-Path $workspaceRoot "cursive\\src\\00_core\\source_load.cpp"
    if (-not (Test-Path $sourcePath)) {
        throw "Case 'issue573_step_size_transition_conformance' missing compiler source file: $sourcePath"
    }

    $sourceText = Get-Content -Path $sourcePath -Raw
    foreach ($pattern in @(
        'SourceLoadSizedState\s+StepSize\s*\(\s*SourceLoadStartState\s+start\s*\)',
        'SPEC_RULE\("Step-Size"\)',
        'return\s+SourceLoadSizedState\s*\{\s*std::move\(start\.path\),\s*std::move\(start\.bytes\)\s*\};',
        'SourceLoadStartState\s+start\s*\{\s*std::string\(path\),\s*bytes\s*\};',
        'const\s+SourceLoadSizedState\s+sized\s*=\s*StepSize\(std::move\(start\)\);',
        'const\s+DecodeResult\s+decoded\s*=\s*Decode\(sized\.bytes\);',
        'BuildSpanSource\(sized\.path,\s*sized\.bytes,\s*std::move\(normalized\)\)'
    )) {
        if ($sourceText -notmatch $pattern) {
            throw "Case 'issue573_step_size_transition_conformance' missing expected source_load.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue573_step_size_transition_conformance: source_patterns=7"
}

function Invoke-Issue574StepDecodeTransitionConformanceCase {
    $sourcePath = Join-Path $workspaceRoot "cursive\\src\\00_core\\source_load.cpp"
    if (-not (Test-Path $sourcePath)) {
        throw "Case 'issue574_step_decode_transition_conformance' missing compiler source file: $sourcePath"
    }

    $sourceText = Get-Content -Path $sourcePath -Raw
    foreach ($pattern in @(
        'SourceLoadDecodedState\s+StepDecode\s*\(\s*SourceLoadSizedState\s+sized,\s*Scalars\s+scalars\s*\)',
        'SPEC_RULE\("Step-Decode"\)',
        'return\s+SourceLoadDecodedState\s*\{\s*std::move\(sized\.path\),\s*std::move\(sized\.bytes\),\s*std::move\(scalars\)\s*\};',
        'const\s+DecodeResult\s+decode_result\s*=\s*Decode\(sized\.bytes\);',
        'SourceLoadDecodedState\s+decoded\s*=\s*StepDecode\(std::move\(sized\),\s*std::move\(decode_result\.scalars\)\);',
        'StripBOM\(decoded\.scalars\)'
    )) {
        if ($sourceText -notmatch $pattern) {
            throw "Case 'issue574_step_decode_transition_conformance' missing expected source_load.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue574_step_decode_transition_conformance: source_patterns=6"
}

function Invoke-Issue575StepDecodeErrorTransitionConformanceCase {
    $sourcePath = Join-Path $workspaceRoot "cursive\\src\\00_core\\source_load.cpp"
    if (-not (Test-Path $sourcePath)) {
        throw "Case 'issue575_step_decode_error_transition_conformance' missing compiler source file: $sourcePath"
    }

    $sourceText = Get-Content -Path $sourcePath -Raw
    foreach ($pattern in @(
        'SourceLoadErrorState\s+StepDecodeErr\s*\(\s*SourceLoadSizedState\s+sized\s*\)',
        'static_cast<void>\(sized\);',
        'SPEC_RULE\("Step-Decode-Err"\)',
        'return\s+SourceLoadErrorState\s*\{\s*"E-SRC-0101"\s*\};',
        'const\s+SourceLoadErrorState\s+error\s*=\s*StepDecodeErr\(std::move\(sized\)\);',
        'MakeDiagnosticById\(error\.code\)'
    )) {
        if ($sourceText -notmatch $pattern) {
            throw "Case 'issue575_step_decode_error_transition_conformance' missing expected source_load.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue575_step_decode_error_transition_conformance: source_patterns=6"
}

function Invoke-Issue576StepBomTransitionConformanceCase {
    $sourcePath = Join-Path $workspaceRoot "cursive\\src\\00_core\\source_load.cpp"
    if (-not (Test-Path $sourcePath)) {
        throw "Case 'issue576_step_bom_transition_conformance' missing compiler source file: $sourcePath"
    }

    $sourceText = Get-Content -Path $sourcePath -Raw
    foreach ($pattern in @(
        'SourceLoadBomStrippedState\s+StepBOM\s*\(\s*SourceLoadDecodedState\s+decoded,\s*StripBOMResult\s+stripped\s*\)',
        'SPEC_RULE\("Step-BOM"\)',
        'return\s+SourceLoadBomStrippedState\s*\{\s*std::move\(decoded\.path\),\s*std::move\(decoded\.bytes\),\s*std::move\(stripped\.scalars\),\s*stripped\.had_bom,\s*stripped\.embedded_index\s*\};',
        'StripBOMResult\s+strip_result\s*=\s*StripBOM\(decoded\.scalars\);',
        'SourceLoadBomStrippedState\s+stripped\s*=\s*StepBOM\(std::move\(decoded\),\s*std::move\(strip_result\)\);',
        'NormalizeLF\(NormalizeOutsideIdentifiers\(stripped\.scalars\)\)',
        'BuildSpanSource\(stripped\.path,\s*stripped\.bytes,\s*std::move\(normalized\)\)'
    )) {
        if ($sourceText -notmatch $pattern) {
            throw "Case 'issue576_step_bom_transition_conformance' missing expected source_load.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue576_step_bom_transition_conformance: source_patterns=7"
}

function Invoke-Issue577StepNormTransitionConformanceCase {
    $sourcePath = Join-Path $workspaceRoot "cursive\\src\\00_core\\source_load.cpp"
    if (-not (Test-Path $sourcePath)) {
        throw "Case 'issue577_step_norm_transition_conformance' missing compiler source file: $sourcePath"
    }

    $sourceText = Get-Content -Path $sourcePath -Raw
    foreach ($pattern in @(
        'SourceLoadNormalizedState\s+StepNorm\s*\(\s*SourceLoadBomStrippedState\s+stripped,\s*Scalars\s+scalars\s*\)',
        'SPEC_RULE\("Step-Norm"\)',
        'return\s+SourceLoadNormalizedState\s*\{\s*std::move\(stripped\.path\),\s*std::move\(stripped\.bytes\),\s*std::move\(scalars\),\s*stripped\.j\s*\};',
        'const\s+bool\s+had_bom\s*=\s*stripped\.had_bom;',
        'Scalars\s+normalized_outside_identifiers\s*=\s*NormalizeOutsideIdentifiers\(stripped\.scalars\);',
        'Scalars\s+normalized_scalars\s*=\s*NormalizeLF\(normalized_outside_identifiers\);',
        'SourceLoadNormalizedState\s+normalized\s*=\s*StepNorm\(std::move\(stripped\),\s*std::move\(normalized_scalars\)\);',
        'BuildSpanSource\(normalized\.path,\s*normalized\.bytes,\s*std::move\(normalized\.scalars\)\)',
        'normalized\.j\.has_value\(\)'
    )) {
        if ($sourceText -notmatch $pattern) {
            throw "Case 'issue577_step_norm_transition_conformance' missing expected source_load.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue577_step_norm_transition_conformance: source_patterns=9"
}

function Invoke-Issue578StepEmbeddedBomErrorTransitionConformanceCase {
    $sourcePath = Join-Path $workspaceRoot "cursive\\src\\00_core\\source_load.cpp"
    if (-not (Test-Path $sourcePath)) {
        throw "Case 'issue578_step_embedded_bom_error_transition_conformance' missing compiler source file: $sourcePath"
    }

    $sourceText = Get-Content -Path $sourcePath -Raw
    foreach ($pattern in @(
        'SourceLoadErrorState\s+StepEmbeddedBOMErr\s*\(\s*SourceLoadNormalizedState\s+normalized\s*\)',
        'static_cast<void>\(normalized\);',
        'SPEC_RULE\("Step-EmbeddedBOM-Err"\)',
        'return\s+SourceLoadErrorState\s*\{\s*"E-SRC-0103"\s*\};',
        'if\s*\(normalized\.j\.has_value\(\)\)\s*\{\s*const\s+SourceLoadErrorState\s+error\s*=\s*StepEmbeddedBOMErr\(std::move\(normalized\)\);',
        'MakeDiagnosticById\(error\.code,\s*SpanAtIndex\(source,\s*offsets,\s*bom_index\)\)'
    )) {
        if ($sourceText -notmatch $pattern) {
            throw "Case 'issue578_step_embedded_bom_error_transition_conformance' missing expected source_load.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue578_step_embedded_bom_error_transition_conformance: source_patterns=6"
}

function Invoke-Issue579StepLineMapTransitionConformanceCase {
    $sourcePath = Join-Path $workspaceRoot "cursive\\src\\00_core\\source_load.cpp"
    if (-not (Test-Path $sourcePath)) {
        throw "Case 'issue579_step_line_map_transition_conformance' missing compiler source file: $sourcePath"
    }

    $sourceText = Get-Content -Path $sourcePath -Raw
    foreach ($pattern in @(
        'SourceLoadLineMappedState\s+StepLineMap\s*\(\s*SourceLoadNormalizedState\s+normalized\s*\)',
        'std::vector<std::size_t>\s+line_starts\s*=\s*LineStarts\(normalized\.scalars\);',
        'SPEC_RULE\("Step-LineMap"\)',
        'return\s+SourceLoadLineMappedState\s*\{\s*std::move\(normalized\.path\),\s*std::move\(normalized\.bytes\),\s*std::move\(normalized\.scalars\),\s*std::move\(line_starts\)\s*\};',
        'SourceFile\s+source\s*=\s*BuildSpanSource\(normalized\.path,\s*normalized\.bytes,\s*normalized\.scalars\);',
        'SourceLoadLineMappedState\s+line_mapped\s*=\s*StepLineMap\(std::move\(normalized\)\);',
        'source\s*=\s*BuildSpanSource\(\s*line_mapped\.path,\s*line_mapped\.bytes,\s*std::move\(line_mapped\.scalars\)\s*\);',
        'source\.line_starts\s*=\s*std::move\(line_mapped\.line_starts\);',
        'source\.line_count\s*=\s*source\.line_starts\.size\(\);',
        'offsets\s*=\s*Utf8Offsets\(source\.scalars\);'
    )) {
        if ($sourceText -notmatch $pattern) {
            throw "Case 'issue579_step_line_map_transition_conformance' missing expected source_load.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue579_step_line_map_transition_conformance: source_patterns=10"
}

function Invoke-Issue580StepProhibitedTransitionConformanceCase {
    $sourcePath = Join-Path $workspaceRoot "cursive\\src\\00_core\\source_load.cpp"
    if (-not (Test-Path $sourcePath)) {
        throw "Case 'issue580_step_prohibited_transition_conformance' missing compiler source file: $sourcePath"
    }

    $sourceText = Get-Content -Path $sourcePath -Raw
    foreach ($pattern in @(
        'SourceLoadValidatedState\s+StepProhibited\s*\(\s*SourceLoadLineMappedState\s+line_mapped\s*\)',
        'SPEC_RULE\("Step-Prohibited"\)',
        'SourceFile\s+source\s*=\s*BuildSpanSource\(\s*line_mapped\.path,\s*line_mapped\.bytes,\s*std::move\(line_mapped\.scalars\)\s*\);',
        'source\.line_starts\s*=\s*std::move\(line_mapped\.line_starts\);',
        'source\.line_count\s*=\s*source\.line_starts\.size\(\);',
        'if\s*\(!NoProhibited\(source\.scalars\)\)',
        'SourceLoadValidatedState\s+validated\s*=\s*StepProhibited\(std::move\(line_mapped\)\);',
        'result\.source\s*=\s*std::move\(validated\.source\);'
    )) {
        if ($sourceText -notmatch $pattern) {
            throw "Case 'issue580_step_prohibited_transition_conformance' missing expected source_load.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue580_step_prohibited_transition_conformance: source_patterns=8"
}

function Invoke-Issue581StepProhibitedErrorTransitionConformanceCase {
    $sourcePath = Join-Path $workspaceRoot "cursive\\src\\00_core\\source_load.cpp"
    if (-not (Test-Path $sourcePath)) {
        throw "Case 'issue581_step_prohibited_error_transition_conformance' missing compiler source file: $sourcePath"
    }

    $sourceText = Get-Content -Path $sourcePath -Raw
    foreach ($pattern in @(
        'SourceLoadErrorState\s+StepProhibitedErr\s*\(\s*SourceLoadLineMappedState\s+line_mapped\s*\)',
        'static_cast<void>\(line_mapped\);',
        'SPEC_RULE\("Step-Prohibited-Err"\)',
        'return\s+SourceLoadErrorState\s*\{\s*"E-SRC-0104"\s*\};',
        'if\s*\(!NoProhibited\(source\.scalars\)\)\s*\{\s*const\s+SourceLoadErrorState\s+error\s*=\s*StepProhibitedErr\(std::move\(line_mapped\)\);',
        'MakeDiagnosticById\(error\.code,\s*SpanAtIndex\(source,\s*offsets,\s*prohibited_index\)\)'
    )) {
        if ($sourceText -notmatch $pattern) {
            throw "Case 'issue581_step_prohibited_error_transition_conformance' missing expected source_load.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue581_step_prohibited_error_transition_conformance: source_patterns=6"
}

function Invoke-Issue582SpanTempSourceConformanceCase {
    $sourcePath = Join-Path $workspaceRoot "cursive\\src\\00_core\\source_load.cpp"
    if (-not (Test-Path $sourcePath)) {
        throw "Case 'issue582_span_temp_source_conformance' missing compiler source file: $sourcePath"
    }

    $sourceText = Get-Content -Path $sourcePath -Raw
    foreach ($pattern in @(
        'struct\s+SourceLoadSpanTemp\s*\{[\s\S]*std::string\s+path;[\s\S]*std::vector<std::uint8_t>\s+bytes;[\s\S]*std::string\s+text;[\s\S]*std::size_t\s+byte_len\s*=\s*0;[\s\S]*std::vector<std::size_t>\s+line_starts;[\s\S]*std::size_t\s+line_count\s*=\s*0;[\s\S]*\}',
        'SourceLoadSpanTemp\s+BuildSpanTemp\s*\(\s*const\s+SourceFile&\s+source\s*\)',
        'SourceFile\s+ToSpanSource\s*\(\s*const\s+SourceLoadSpanTemp&\s+source\s*\)',
        'Span\s+SpanOfTemp\s*\(\s*const\s+SourceLoadSpanTemp&\s+source,\s*std::size_t\s+start,\s*std::size_t\s+end\s*\)',
        'Span\s+SpanAtIndex\s*\(\s*const\s+SourceLoadSpanTemp&\s+source,\s*const\s+std::vector<std::size_t>&\s+offsets,\s*std::size_t\s+index\s*\)',
        'const\s+SourceLoadSpanTemp\s+span_source\s*=\s*BuildSpanTemp\(source\);',
        'SpanOfTemp\(span_source,\s*0,\s*end\)',
        'SpanAtIndex\(span_source,\s*offsets,\s*bom_index\)',
        'SpanAtIndex\(span_source,\s*offsets,\s*prohibited_index\)'
    )) {
        if ($sourceText -notmatch $pattern) {
            throw "Case 'issue582_span_temp_source_conformance' missing expected source_load.cpp pattern '$pattern'."
        }
    }

    if ($sourceText -match 'struct\s+SourceLoadSpanTemp\s*\{[\s\S]*scalars;') {
        throw "Case 'issue582_span_temp_source_conformance' expected SourceLoadSpanTemp to omit any scalars field."
    }

    Write-Host "[compiler-static] issue582_span_temp_source_conformance: source_patterns=9 no_scalars=1"
}

function Invoke-Issue583SpanAtLineStartConformanceCase {
    $sourcePath = Join-Path $workspaceRoot "cursive\\src\\00_core\\source_load.cpp"
    if (-not (Test-Path $sourcePath)) {
        throw "Case 'issue583_span_at_line_start_conformance' missing compiler source file: $sourcePath"
    }

    $sourceText = Get-Content -Path $sourcePath -Raw
    foreach ($pattern in @(
        'Span\s+SpanAtLineStart\s*\(\s*const\s+SourceLoadSpanTemp&\s+source,\s*std::size_t\s+index\s*\)',
        'index\s*<\s*source\.line_starts\.size\(\)\s*\?\s*source\.line_starts\[index\]\s*:\s*source\.byte_len',
        'start\s*<\s*source\.byte_len\s*\?\s*std::min\(start\s*\+\s*1,\s*source\.byte_len\)\s*:\s*source\.byte_len',
        'return\s+SpanOfTemp\(source,\s*start,\s*end\);'
    )) {
        if ($sourceText -notmatch $pattern) {
            throw "Case 'issue583_span_at_line_start_conformance' missing expected source_load.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue583_span_at_line_start_conformance: source_patterns=4"
}

function Invoke-Issue584LexerInputProjectionConformanceCase {
    $headerPath = Join-Path $workspaceRoot "cursive\\include\\02_source\\lexer\\lexer.h"
    $sourcePath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\tokenize.cpp"
    foreach ($path in @($headerPath, $sourcePath)) {
        if (-not (Test-Path $path)) {
            throw "Case 'issue584_lexer_input_projection_conformance' missing compiler source file: $path"
        }
    }

    $headerText = Get-Content -Path $headerPath -Raw
    $sourceText = Get-Content -Path $sourcePath -Raw

    foreach ($pattern in @(
        'struct\s+LexerInput\s*\{[\s\S]*const\s+std::vector<core::UnicodeScalar>\*\s+scalars\s*=\s*nullptr;[\s\S]*std::string_view\s+text;[\s\S]*std::size_t\s+byte_len\s*=\s*0;[\s\S]*\}',
        'LexerInput\s+MakeLexerInput\s*\(\s*const\s+core::SourceFile&\s+source\s*\);'
    )) {
        if ($headerText -notmatch $pattern) {
            throw "Case 'issue584_lexer_input_projection_conformance' missing expected lexer.h pattern '$pattern'."
        }
    }

    foreach ($pattern in @(
        'LexerInput\s+MakeLexerInput\s*\(\s*const\s+core::SourceFile&\s+source\s*\)\s*\{\s*return\s+LexerInput\{\s*&source\.scalars,\s*source\.text,\s*source\.byte_len\s*\};\s*\}',
        'const\s+LexerInput\s+input\s*=\s*MakeLexerInput\(source\);',
        'const\s+auto&\s+scalars\s*=\s*\*input\.scalars;',
        'std::string\s+LexemeSlice\s*\(\s*const\s+LexerInput&\s+input,',
        'end\s*>\s*input\.byte_len'
    )) {
        if ($sourceText -notmatch $pattern) {
            throw "Case 'issue584_lexer_input_projection_conformance' missing expected tokenize.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue584_lexer_input_projection_conformance: header_patterns=2 source_patterns=5"
}

function Invoke-Issue585TokenEofConformanceCase {
    $headerPath = Join-Path $workspaceRoot "cursive\\include\\02_source\\lexer\\token.h"
    $tokenPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\token.cpp"
    $builderPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\ast\\builder\\builder_common.cpp"
    foreach ($path in @($headerPath, $tokenPath, $builderPath)) {
        if (-not (Test-Path $path)) {
            throw "Case 'issue585_token_eof_conformance' missing compiler source file: $path"
        }
    }

    $headerText = Get-Content -Path $headerPath -Raw
    $tokenText = Get-Content -Path $tokenPath -Raw
    $builderText = Get-Content -Path $builderPath -Raw

    foreach ($pattern in @(
        'enum\s+class\s+TokenKind[\s\S]*Newline,\s*Eof,\s*Unknown',
        'Token\s+MakeEofToken\s*\(\s*const\s+core::SourceFile&\s+source\s*\);'
    )) {
        if ($headerText -notmatch $pattern) {
            throw "Case 'issue585_token_eof_conformance' missing expected token.h pattern '$pattern'."
        }
    }

    foreach ($pattern in @(
        'Token\s+MakeEofToken\s*\(\s*const\s+core::SourceFile&\s+source\s*\)',
        'eof\.kind\s*=\s*TokenKind::Eof;',
        'eof\.lexeme\.clear\(\);',
        'eof\.span\s*=\s*core::SpanOf\(source,\s*source\.byte_len,\s*source\.byte_len\);'
    )) {
        if ($tokenText -notmatch $pattern) {
            throw "Case 'issue585_token_eof_conformance' missing expected token.cpp pattern '$pattern'."
        }
    }

    if ($builderText -notmatch 'tok\.kind\s*=\s*lexer::TokenKind::Eof;') {
        throw "Case 'issue585_token_eof_conformance' missing EOF sentinel update in builder_common.cpp."
    }

    Write-Host "[compiler-static] issue585_token_eof_conformance: header_patterns=2 token_patterns=4 builder_patterns=1"
}

function Invoke-Issue586TokenRangeConformanceCase {
    $headerPath = Join-Path $workspaceRoot "cursive\\include\\02_source\\lexer\\token.h"
    $tokenPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\token.cpp"
    foreach ($path in @($headerPath, $tokenPath)) {
        if (-not (Test-Path $path)) {
            throw "Case 'issue586_token_range_conformance' missing compiler source file: $path"
        }
    }

    $headerText = Get-Content -Path $headerPath -Raw
    $tokenText = Get-Content -Path $tokenPath -Raw

    if ($headerText -notmatch 'std::optional<std::pair<std::size_t,\s*std::size_t>>\s+TokenRange\s*\(\s*const\s+core::SourceFile&\s+source,\s*const\s+Token&\s+token\s*\);') {
        throw "Case 'issue586_token_range_conformance' missing TokenRange declaration in token.h."
    }

    foreach ($pattern in @(
        'std::optional<std::pair<std::size_t,\s*std::size_t>>\s+TokenRange\s*\(\s*const\s+core::SourceFile&\s+source,\s*const\s+Token&\s+token\s*\)',
        'const\s+auto\s+offsets\s*=\s*core::Utf8Offsets\(source\.scalars\);',
        'std::find\(offsets\.begin\(\),\s*offsets\.end\(\),\s*token\.span\.start_offset\)',
        'std::find\(offsets\.begin\(\),\s*offsets\.end\(\),\s*token\.span\.end_offset\)',
        'const\s+std::size_t\s+i\s*=',
        'const\s+std::size_t\s+j\s*=',
        'const\s+core::Span\s+expected\s*=\s*core::SpanOf\(source,\s*offsets\[i\],\s*offsets\[j\]\);',
        'return\s+std::pair<std::size_t,\s*std::size_t>\{i,\s*j\};'
    )) {
        if ($tokenText -notmatch $pattern) {
            throw "Case 'issue586_token_range_conformance' missing expected token.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue586_token_range_conformance: header_decl=1 token_patterns=8"
}

function Invoke-Issue610ParserTokEofConformanceCase {
    $headerPath = Join-Path $workspaceRoot "cursive\\include\\02_source\\parser\\parser.h"
    $sourcePath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\parser_state.cpp"
    foreach ($path in @($headerPath, $sourcePath)) {
        if (-not (Test-Path $path)) {
            throw "Case 'issue610_parser_tok_eof_conformance' missing compiler source file: $path"
        }
    }

    $headerText = Get-Content -Path $headerPath -Raw
    $sourceText = Get-Content -Path $sourcePath -Raw

    foreach ($pattern in @(
        'const\s+Token\*\s+Tok\s*\(\s*const\s+Parser&\s+parser\s*\);',
        'const\s+core::Span&\s+TokSpan\s*\(\s*const\s+Parser&\s+parser\s*\);'
    )) {
        if ($headerText -notmatch $pattern) {
            throw "Case 'issue610_parser_tok_eof_conformance' missing expected parser.h pattern '$pattern'."
        }
    }

    if ($headerText -match 'mutable\s+Token\s+eof;') {
        throw "Case 'issue610_parser_tok_eof_conformance' found removed cached EOF field in parser.h."
    }

    foreach ($pattern in @(
        'Token\s+MakeParserEofToken\s*\(\s*const\s+Parser&\s+parser\s*\)',
        'Token&\s+ParserEofTokenCache\s*\(\s*\)',
        'Token&\s+eof\s*=\s*ParserEofTokenCache\(\);',
        'eof\s*=\s*MakeParserEofToken\(parser\);',
        'return\s*&eof;',
        'return\s*&\(\*parser\.tokens\)\[parser\.index\];',
        'return\s+Tok\(parser\)->span;'
    )) {
        if ($sourceText -notmatch $pattern) {
            throw "Case 'issue610_parser_tok_eof_conformance' missing expected parser_state.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue610_parser_tok_eof_conformance: header_patterns=2 header_forbidden=1 source_patterns=6"
}

function Invoke-Issue611ParserEofSpanHelperConformanceCase {
    $headerPath = Join-Path $workspaceRoot "cursive\\include\\02_source\\parser\\parser.h"
    $statePath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\parser_state.cpp"
    $builderPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\ast\\builder\\builder_common.cpp"
    $quotePath = Join-Path $workspaceRoot "cursive\\src\\03_comptime\\quote.cpp"
    $typeExprPath = Join-Path $workspaceRoot "cursive\\src\\04_analysis\\typing\\type_expr.cpp"
    foreach ($path in @($headerPath, $statePath, $builderPath, $quotePath, $typeExprPath)) {
        if (-not (Test-Path $path)) {
            throw "Case 'issue611_parser_eof_span_helper_conformance' missing compiler source file: $path"
        }
    }

    $headerText = Get-Content -Path $headerPath -Raw
    $stateText = Get-Content -Path $statePath -Raw
    $builderText = Get-Content -Path $builderPath -Raw
    $quoteText = Get-Content -Path $quotePath -Raw
    $typeExprText = Get-Content -Path $typeExprPath -Raw

    if ($headerText -notmatch 'const\s+core::SourceFile\*\s+source\s*=\s*nullptr;') {
        throw "Case 'issue611_parser_eof_span_helper_conformance' missing parser source provenance field in parser.h."
    }

    foreach ($pattern in @(
        'core::Span\s+PointSpanAtEnd\s*\(\s*const\s+Token&\s+token\s*\)',
        'Token\s+MakeParserEofToken\s*\(\s*const\s+Parser&\s+parser\s*\)',
        'parser\.source\s*=\s*&source;',
        'if\s*\(parser\.source\)\s*\{\s*return\s+MakeEofToken\(\*parser\.source\);',
        'eof\.span\s*=\s*PointSpanAtEnd\(parser\.tokens->back\(\)\);'
    )) {
        if ($stateText -notmatch $pattern) {
            throw "Case 'issue611_parser_eof_span_helper_conformance' missing expected parser_state.cpp pattern '$pattern'."
        }
    }

    foreach ($forbidden in @(
        'RefreshEofToken\s*\(',
        'parser\.eof\b'
    )) {
        if ($stateText -match $forbidden) {
            throw "Case 'issue611_parser_eof_span_helper_conformance' found removed eager EOF initialization pattern '$forbidden'."
        }
    }

    if ($builderText -notmatch 'lexer::Token\s+start_tok\s*=\s*\*Tok\(start\);') {
        throw "Case 'issue611_parser_eof_span_helper_conformance' expected builder_common.cpp to source EOF tokens through Tok(start)."
    }

    foreach ($forbidden in @(
        'tok\.span\s*=\s*parser\.eof\.span;',
        'parser\.eof\.span\s*=\s*tokens\.back\(\)\.span;'
    )) {
        if ($builderText -match $forbidden -or $quoteText -match $forbidden -or $typeExprText -match $forbidden) {
            throw "Case 'issue611_parser_eof_span_helper_conformance' found removed direct EOF-span seeding pattern '$forbidden'."
        }
    }

    Write-Host "[compiler-static] issue611_parser_eof_span_helper_conformance: header_patterns=1 state_patterns=5 state_forbidden=2 builder_checks=2 quote_checks=1 type_expr_checks=1"
}

function Invoke-Issue612ParserTokensBetweenConformanceCase {
    $headerPath = Join-Path $workspaceRoot "cursive\\include\\02_source\\parser\\parser.h"
    $statePath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\parser_state.cpp"
    $quotePath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\expr\\quote_expr.cpp"
    foreach ($path in @($headerPath, $statePath, $quotePath)) {
        if (-not (Test-Path $path)) {
            throw "Case 'issue612_parser_tokens_between_conformance' missing compiler source file: $path"
        }
    }

    $headerText = Get-Content -Path $headerPath -Raw
    $stateText = Get-Content -Path $statePath -Raw
    $quoteText = Get-Content -Path $quotePath -Raw

    foreach ($pattern in @(
        'std::pair<std::size_t,\s*std::size_t>\s+TokensBetween\s*\(\s*const\s+Parser&\s+start,\s*const\s+Parser&\s+end\s*\);'
    )) {
        if ($headerText -notmatch $pattern) {
            throw "Case 'issue612_parser_tokens_between_conformance' missing expected parser.h pattern '$pattern'."
        }
    }

    foreach ($pattern in @(
        'std::pair<std::size_t,\s*std::size_t>\s+TokensBetween\s*\(\s*const\s+Parser&\s+start,\s*const\s+Parser&\s+end\s*\)\s*\{\s*SPEC_DEF\("TokensBetween",\s*"5\.5"\);\s*return\s+std::pair<std::size_t,\s*std::size_t>\{start\.index,\s*end\.index\};\s*\}'
    )) {
        if ($stateText -notmatch $pattern) {
            throw "Case 'issue612_parser_tokens_between_conformance' missing expected parser_state.cpp pattern '$pattern'."
        }
    }

    if ($quoteText -match 'std::vector<Token>\s+TokensBetween\s*\(') {
        throw "Case 'issue612_parser_tokens_between_conformance' found legacy quote-local TokensBetween helper in quote_expr.cpp."
    }

    foreach ($pattern in @(
        'const\s+auto\s+\[from,\s*to\]\s*=\s*TokensBetween\(start,\s*end\);',
        'quote\.tokens\s*=\s*SliceTokensBetween\(content_start,\s*content_end\);'
    )) {
        if ($quoteText -notmatch $pattern) {
            throw "Case 'issue612_parser_tokens_between_conformance' missing expected quote_expr.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue612_parser_tokens_between_conformance: header_patterns=1 state_patterns=1 quote_checks=3"
}

function Invoke-Issue613ParserInitialStateConformanceCase {
    $headerPath = Join-Path $workspaceRoot "cursive\\include\\02_source\\parser\\parser.h"
    $sourcePath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\parser_state.cpp"
    foreach ($path in @($headerPath, $sourcePath)) {
        if (-not (Test-Path $path)) {
            throw "Case 'issue613_parser_initial_state_conformance' missing compiler source file: $path"
        }
    }

    $headerText = Get-Content -Path $headerPath -Raw
    $sourceText = Get-Content -Path $sourcePath -Raw

    if ($headerText -match 'mutable\s+Token\s+eof;') {
        throw "Case 'issue613_parser_initial_state_conformance' found cached EOF token embedded in Parser state."
    }

    foreach ($pattern in @(
        'Parser\s+MakeParser\s*\(\s*const\s+std::vector<Token>&\s+tokens,\s*const\s+std::vector<DocComment>&\s+docs,\s*const\s+core::SourceFile&\s+source\s*\)',
        'Parser\s+parser;',
        'parser\.tokens\s*=\s*&tokens;',
        'parser\.source\s*=\s*&source;',
        'parser\.index\s*=\s*0;',
        'parser\.docs\s*=\s*&docs;',
        'parser\.doc_index\s*=\s*0;',
        'parser\.depth\s*=\s*0;'
    )) {
        if ($sourceText -notmatch $pattern) {
            throw "Case 'issue613_parser_initial_state_conformance' missing expected parser_state.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue613_parser_initial_state_conformance: header_forbidden=1 source_patterns=8"
}

function Invoke-Issue614ParseItemsEmptyEofOnlyConformanceCase {
    $parserPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\parser.cpp"
    if (-not (Test-Path $parserPath)) {
        throw "Case 'issue614_parse_items_empty_eof_only_conformance' missing parser implementation file: $parserPath"
    }

    $parserText = Get-Content -Path $parserPath -Raw

    $hasImmediateEofCheck = $parserText -match 'Parser\s+cur\s*=\s*parser;\s*for\s*\(\s*;\s*;\s*\)\s*\{\s*if\s*\(AtEof\(cur\)\)\s*\{\s*SPEC_RULE\("ParseItems-Empty"\)'
    $hasConsRule = $parserText -match 'SPEC_RULE\("ParseItems-Cons"\)'
    $hasParseItemStep = $parserText -match 'ParseItemResult\s+item\s*=\s*ParseItem\(cur\);'
    $hasRemovedNewlineSkip = $parserText -notmatch 'while\s*\(!AtEof\(cur\)\)\s*\{\s*const\s+Token\*\s+tok\s*=\s*Tok\(cur\);\s*if\s*\(!tok\s*\|\|\s*tok->kind\s*!=\s*TokenKind::Newline\)\s*\{\s*break;\s*\}\s*Advance\(cur\);\s*\}'

    if ((-not $hasImmediateEofCheck) -or (-not $hasConsRule) -or (-not $hasParseItemStep) -or (-not $hasRemovedNewlineSkip)) {
        throw "Case 'issue614_parse_items_empty_eof_only_conformance' expected ParseItemsInternal to check EOF before any newline skipping and to recurse through ParseItem for the non-empty case."
    }

    Write-Host "[compiler-static] issue614_parse_items_empty_eof_only_conformance: immediate_eof=$hasImmediateEofCheck cons_rule=$hasConsRule parse_item_step=$hasParseItemStep removed_newline_skip=$hasRemovedNewlineSkip"
}

function Invoke-Issue615DocSeqSurfaceConformanceCase {
    $headerPath = Join-Path $workspaceRoot "cursive\\include\\02_source\\parser\\parser.h"
    $parserPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\parser.cpp"
    $docsPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\parser_docs.cpp"
    $registryPath = Join-Path $workspaceRoot "cursive\\src\\00_core\\generated\\static_rule_registry.inc"
    foreach ($path in @($headerPath, $parserPath, $docsPath, $registryPath)) {
        if (-not (Test-Path $path)) {
            throw "Case 'issue615_doc_seq_surface_conformance' missing compiler source file: $path"
        }
    }

    $headerText = Get-Content -Path $headerPath -Raw
    $parserText = Get-Content -Path $parserPath -Raw
    $docsText = Get-Content -Path $docsPath -Raw
    $registryText = Get-Content -Path $registryPath -Raw

    if ($headerText -notmatch 'const\s+std::vector<DocComment>&\s+DocSeq\s*\(\s*const\s+std::vector<DocComment>&\s+docs\s*\);') {
        throw "Case 'issue615_doc_seq_surface_conformance' missing DocSeq declaration in parser.h."
    }
    foreach ($pattern in @(
        'const\s+std::vector<DocComment>&\s+DocSeq\s*\(\s*const\s+std::vector<DocComment>&\s+docs\s*\)\s*\{\s*SPEC_RULE\("DocSeq"\);\s*return\s+docs;\s*\}',
        'std::vector<DocComment>\s+ModuleDocs\s*\(\s*const\s+std::vector<DocComment>&\s+docs\s*\)',
        'void\s+AttachLineDocs\s*\(\s*std::vector<ASTItem>&\s+items,\s*const\s+std::vector<DocComment>&\s+docs\s*\)'
    )) {
        if ($docsText -notmatch $pattern) {
            throw "Case 'issue615_doc_seq_surface_conformance' missing expected parser_docs.cpp pattern '$pattern'."
        }
    }
    foreach ($pattern in @(
        'return\s+ParseItemsInternal\(parser,\s*ModuleDocs\(DocSeq\(\*parser\.docs\)\)\);',
        'const\s+std::vector<DocComment>&\s+doc_seq\s*=\s*DocSeq\(tok\.output->docs\);',
        'AttachLineDocs\(items\.items,\s*doc_seq\);'
    )) {
        if ($parserText -notmatch $pattern) {
            throw "Case 'issue615_doc_seq_surface_conformance' missing expected parser.cpp pattern '$pattern'."
        }
    }
    if ($registryText -notmatch '\{"DocSeq",\s*"ParseJudgment",\s*std::nullopt,\s*"02_source/parser/parser_docs\.cpp",\s*std::string_view\("D"\)\}') {
        throw "Case 'issue615_doc_seq_surface_conformance' missing DocSeq static rule registry entry."
    }

    Write-Host "[compiler-static] issue615_doc_seq_surface_conformance: header_decl=1 docs_patterns=3 parser_patterns=3 registry_entry=1"
}

function Invoke-Issue616ItemSeqSurfaceConformanceCase {
    $headerPath = Join-Path $workspaceRoot "cursive\\include\\02_source\\parser\\parser.h"
    $parserPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\parser.cpp"
    $docsPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\parser_docs.cpp"
    $registryPath = Join-Path $workspaceRoot "cursive\\src\\00_core\\generated\\static_rule_registry.inc"
    foreach ($path in @($headerPath, $parserPath, $docsPath, $registryPath)) {
        if (-not (Test-Path $path)) {
            throw "Case 'issue616_item_seq_surface_conformance' missing compiler source file: $path"
        }
    }

    $headerText = Get-Content -Path $headerPath -Raw
    $parserText = Get-Content -Path $parserPath -Raw
    $docsText = Get-Content -Path $docsPath -Raw
    $registryText = Get-Content -Path $registryPath -Raw

    if ($headerText -notmatch 'std::vector<ASTItem>\s+ItemSeq\s*\(\s*std::vector<ASTItem>\s+items\s*\);') {
        throw "Case 'issue616_item_seq_surface_conformance' missing ItemSeq declaration in parser.h."
    }
    if ($docsText -notmatch 'std::vector<ASTItem>\s+ItemSeq\s*\(\s*std::vector<ASTItem>\s+items\s*\)\s*\{\s*SPEC_RULE\("ItemSeq\(Items\)"\);\s*return\s+items;\s*\}') {
        throw "Case 'issue616_item_seq_surface_conformance' missing ItemSeq identity helper in parser_docs.cpp."
    }
    foreach ($pattern in @(
        'std::vector<ASTItem>\s+item_seq\s*=\s*ItemSeq\(std::move\(items\.items\)\);',
        'AttachLineDocs\(item_seq,\s*doc_seq\);',
        'FirstTopLevelErrorItemSpan\(item_seq\);',
        'file\.items\s*=\s*std::move\(item_seq\);'
    )) {
        if ($parserText -notmatch $pattern) {
            throw "Case 'issue616_item_seq_surface_conformance' missing expected parser.cpp pattern ''$pattern''."
        }
    }
    if ($registryText -notmatch '\{"ItemSeq\(Items\)",\s*"ParseJudgment",\s*std::nullopt,\s*"02_source/parser/parser_docs\.cpp",\s*std::nullopt\}') {
        throw "Case 'issue616_item_seq_surface_conformance' missing ItemSeq static rule registry entry."
    }

    Write-Host "[compiler-static] issue616_item_seq_surface_conformance: header_decl=1 docs_helper=1 parser_patterns=4 registry_entry=1"
}

function Invoke-Issue618ParseItemsConsTraceConformanceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue618_parse_items_cons_trace_conformance" `
        -Source (New-Issue618ParseItemsConsTraceSource) `
        -ConformanceFileName "issue618_parse_items_cons_trace_conformance.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue618_parse_items_cons_trace_conformance' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue618_parse_items_cons_trace_conformance' expected zero compile-time errors but observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $parseItemsConsCount = @($logLines | Where-Object { $_ -like "*`tParseItems-Cons`t*" }).Count
    $parseItemsEmptyCount = @($logLines | Where-Object { $_ -like "*`tParseItems-Empty`t*" }).Count
    $parseItemErrCount = @($logLines | Where-Object { $_ -like "*`tParse-Item-Err`t*" }).Count
    if ($parseItemsConsCount -ne 3 -or $parseItemsEmptyCount -ne 1 -or $parseItemErrCount -ne 0) {
        throw "Case 'issue618_parse_items_cons_trace_conformance' expected exactly three ParseItems-Cons traces, one ParseItems-Empty trace, and zero Parse-Item-Err traces (cons=$parseItemsConsCount empty=$parseItemsEmptyCount item_err=$parseItemErrCount)."
    }

    $parserPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\parser.cpp"
    if (-not (Test-Path $parserPath)) {
        throw "Case 'issue618_parse_items_cons_trace_conformance' missing parser implementation file: $parserPath"
    }

    $parserText = Get-Content -Path $parserPath -Raw
    $hasParseItemStep = $parserText -match 'ParseItemResult\s+item\s*=\s*ParseItem\(cur\);'
    $hasPushParsedItem = $parserText -match 'result\.items\.push_back\(std::move\(item\.item\)\);'
    $hasRemovedStallRecovery = $parserText -notmatch 'if\s*\(item\.parser\.tokens\s*==\s*cur\.tokens\s*&&\s*item\.parser\.index\s*==\s*cur\.index\)\s*\{\s*EmitParseSyntaxErr\(cur,\s*TokSpan\(cur\)\);\s*Parser\s+next\s*=\s*AdvanceOrEOF\(cur\);\s*result\.items\.push_back\(ErrorItem\{SpanBetween\(cur,\s*next\),\s*\{\}\}\);\s*cur\s*=\s*next;\s*continue;\s*\}'
    if ((-not $hasParseItemStep) -or (-not $hasPushParsedItem) -or (-not $hasRemovedStallRecovery)) {
        throw "Case 'issue618_parse_items_cons_trace_conformance' expected ParseItems-Cons to append the ParseItem result directly without the old stall-recovery branch."
    }

    Write-Host "[compiler-static] issue618_parse_items_cons_trace_conformance: exit=$($result.ExitCode) errors=$errorCount parse_items_cons=$parseItemsConsCount parse_items_empty=$parseItemsEmptyCount parse_item_err=$parseItemErrCount direct_append=$hasPushParsedItem removed_stall_recovery=$hasRemovedStallRecovery"
}

function Invoke-Issue619Phase1DiagRulesSurfaceConformanceCase {
    $parserPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\parser_recovery.cpp"
    if (-not (Test-Path $parserPath)) {
        throw "Case 'issue619_phase1_diag_rules_surface_conformance' missing parser recovery implementation file: $parserPath"
    }

    $parserText = Get-Content -Path $parserPath -Raw
    foreach ($pattern in @(
        'SPEC_DEF\("StmtParseErrRule",\s*"5\.8"\);',
        'SPEC_DEF\("ItemParseErrRule",\s*"5\.8"\);',
        'SPEC_DEF\("Phase1DiagRules",\s*"5\.9"\);'
    )) {
        if ($parserText -notmatch $pattern) {
            throw "Case 'issue619_phase1_diag_rules_surface_conformance' missing parser recovery alias pattern '$pattern'."
        }
    }

    $hasOrderedAliasBlock = $parserText -match 'static\s+inline\s+void\s+SpecDefsParserRecovery\(\)\s*\{\s*SPEC_DEF\("StmtParseErrRule",\s*"5\.8"\);\s*SPEC_DEF\("ItemParseErrRule",\s*"5\.8"\);\s*SPEC_DEF\("Phase1DiagRules",\s*"5\.9"\);\s*\}'
    if (-not $hasOrderedAliasBlock) {
        throw "Case 'issue619_phase1_diag_rules_surface_conformance' expected SpecDefsParserRecovery to declare the full phase-1 diagnostic alias block in order."
    }

    Write-Host "[compiler-static] issue619_phase1_diag_rules_surface_conformance: parser_aliases=3 ordered_block=1"
}

function Invoke-Issue620QuoteProbeParseSyntaxErrConformanceCase {
    $headerPath = Join-Path $workspaceRoot "cursive\\include\\02_source\\parser\\parser.h"
    $parserPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\parser_recovery.cpp"
    foreach ($path in @($headerPath, $parserPath)) {
        if (-not (Test-Path $path)) {
            throw "Case 'issue620_quote_probe_parse_syntax_err_conformance' missing compiler source file: $path"
        }
    }

    $headerText = Get-Content -Path $headerPath -Raw
    $parserText = Get-Content -Path $parserPath -Raw
    if ($headerText -notmatch 'void\s+EmitGenericParseSyntaxErr\s*\(\s*Parser&\s+parser,\s*const\s+core::Span&\s+span\s*\);') {
        throw "Case 'issue620_quote_probe_parse_syntax_err_conformance' missing EmitGenericParseSyntaxErr declaration in parser.h."
    }

    $hasRawHelperWithoutSpecRule = $parserText -match 'void\s+EmitParseSyntaxErr\s*\(\s*Parser&\s+parser,\s*const\s+core::Span&\s+span\s*\)\s*\{\s*auto\s+diag\s*=\s*core::MakeDiagnosticById\("E-SRC-0520",\s*span\);'
    $hasGenericHelperWithSpecRule = $parserText -match 'void\s+EmitGenericParseSyntaxErr\s*\(\s*Parser&\s+parser,\s*const\s+core::Span&\s+span\s*\)\s*\{\s*SpecDefsParserRecovery\(\);\s*SPEC_RULE\("Parse-Syntax-Err"\);\s*EmitParseSyntaxErr\(parser,\s*span\);'
    if ((-not $hasRawHelperWithoutSpecRule) -or (-not $hasGenericHelperWithSpecRule)) {
        throw "Case 'issue620_quote_probe_parse_syntax_err_conformance' expected parser_recovery.cpp to keep raw E-SRC-0520 emission separate from generic Parse-Syntax-Err attribution."
    }

    $result = Invoke-CheckWithConformance `
        -CaseId "issue620_quote_probe_parse_syntax_err_conformance" `
        -Source (New-Issue620QuoteStmtProbeSource) `
        -ConformanceFileName "issue620_quote_probe_parse_syntax_err_conformance.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue620_quote_probe_parse_syntax_err_conformance' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue620_quote_probe_parse_syntax_err_conformance' expected zero compile-time errors but observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $parseQuoteCount = @($logLines | Where-Object { $_ -like "*`tParse-Quote`t*" }).Count
    $parseSyntaxErrCount = @($logLines | Where-Object { $_ -like "*`tParse-Syntax-Err`t*" }).Count
    if ($parseQuoteCount -lt 1 -or $parseSyntaxErrCount -ne 0) {
        throw "Case 'issue620_quote_probe_parse_syntax_err_conformance' expected Parse-Quote with no Parse-Syntax-Err trace from quote-kind probing (quote=$parseQuoteCount parse_syntax_err=$parseSyntaxErrCount)."
    }

    Write-Host "[compiler-static] issue620_quote_probe_parse_syntax_err_conformance: exit=$($result.ExitCode) errors=$errorCount parse_quote=$parseQuoteCount parse_syntax_err=$parseSyntaxErrCount raw_helper=$hasRawHelperWithoutSpecRule generic_helper=$hasGenericHelperWithSpecRule"
}

function Invoke-Issue621ParseSyntaxErrPremisesHoldConformanceCase {
    $headerPath = Join-Path $workspaceRoot "cursive\\include\\02_source\\parser\\parser.h"
    $parserRecoveryPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\parser_recovery.cpp"
    $registryPath = Join-Path $workspaceRoot "cursive\\src\\00_core\\generated\\static_rule_registry.inc"
    $parserRoot = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser"
    foreach ($path in @($headerPath, $parserRecoveryPath, $registryPath, $parserRoot)) {
        if (-not (Test-Path $path)) {
            throw "Case 'issue621_parse_syntax_err_premises_hold_conformance' missing compiler source path: $path"
        }
    }

    $headerText = Get-Content -Path $headerPath -Raw
    $parserRecoveryText = Get-Content -Path $parserRecoveryPath -Raw
    $registryText = Get-Content -Path $registryPath -Raw

    if ($headerText -notmatch 'void\s+EmitGenericParseSyntaxErr\s*\(\s*Parser&\s+parser,\s*const\s+core::Span&\s+span\s*\);') {
        throw "Case 'issue621_parse_syntax_err_premises_hold_conformance' missing EmitGenericParseSyntaxErr declaration in parser.h."
    }

    $hasGenericHelperWithSpecRule = $parserRecoveryText -match 'void\s+EmitGenericParseSyntaxErr\s*\(\s*Parser&\s+parser,\s*const\s+core::Span&\s+span\s*\)\s*\{\s*SpecDefsParserRecovery\(\);\s*SPEC_RULE\("Parse-Syntax-Err"\);\s*EmitParseSyntaxErr\(parser,\s*span\);'
    if (-not $hasGenericHelperWithSpecRule) {
        throw "Case 'issue621_parse_syntax_err_premises_hold_conformance' expected parser_recovery.cpp to attribute Parse-Syntax-Err through the generic helper."
    }

    $expectedGenericProducerPaths = @(
        "cursive/src/02_source/parser/item/parse_item.cpp"
        "cursive/src/02_source/parser/parser_paths.cpp"
        "cursive/src/02_source/parser/parser_recovery.cpp"
        "cursive/src/02_source/parser/pattern/pattern_common.cpp"
        "cursive/src/02_source/parser/stmt/error_stmt.cpp"
        "cursive/src/02_source/parser/stmt/parse_stmt.cpp"
        "cursive/src/02_source/parser/type/type_common.cpp"
    )

    $observedGenericProducerPaths = @(
        Get-ChildItem -Path $parserRoot -Recurse -Filter '*.cpp' | Where-Object {
            (Get-Content -Path $_.FullName -Raw) -match 'EmitGenericParseSyntaxErr\s*\('
        } | ForEach-Object {
            $_.FullName.Substring($workspaceRoot.Length + 1).Replace('\', '/')
        } | Sort-Object -Unique
    )

    $missingGenericProducerPaths = @(
        $expectedGenericProducerPaths | Where-Object {
            $observedGenericProducerPaths -notcontains $_
        }
    )
    $unexpectedGenericProducerPaths = @(
        $observedGenericProducerPaths | Where-Object {
            $expectedGenericProducerPaths -notcontains $_
        }
    )
    if ($missingGenericProducerPaths.Count -ne 0 -or $unexpectedGenericProducerPaths.Count -ne 0) {
        throw "Case 'issue621_parse_syntax_err_premises_hold_conformance' expected generic Parse-Syntax-Err attribution only in the parser recovery helper and the six generic parse-rule producers (missing=$($missingGenericProducerPaths -join ',') unexpected=$($unexpectedGenericProducerPaths -join ','))."
    }

    $hasPremisesRegistryEntry = $registryText -match '\{"Parse-Syntax-Err",\s*"ParseJudgment",\s*std::nullopt,\s*"02_source/parser/expr/path\.cpp",\s*std::string_view\("GenericParseRules = \{Parse-Ident-Err, Parse-Type-Err, Parse-Pattern-Err, Parse-Primary-Err, Parse-Statement-Err, Parse-Item-Err\}\\nr ∈ GenericParseRules\\nPremisesHold\(r, P\)"\)\}'
    if (-not $hasPremisesRegistryEntry) {
        throw "Case 'issue621_parse_syntax_err_premises_hold_conformance' missing Parse-Syntax-Err static rule registry premises entry."
    }

    Write-Host "[compiler-static] issue621_parse_syntax_err_premises_hold_conformance: helper_decl=1 helper_rule=1 producer_paths=$($observedGenericProducerPaths.Count) registry_premises=1"
}

function Invoke-Issue587TokenInCommentConformanceCase {
    $headerPath = Join-Path $workspaceRoot "cursive\\include\\02_source\\lexer\\lexer.h"
    $sourcePath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\lexer_ws.cpp"
    foreach ($path in @($headerPath, $sourcePath)) {
        if (-not (Test-Path $path)) {
            throw "Case 'issue587_token_in_comment_conformance' missing compiler source file: $path"
        }
    }

    $headerText = Get-Content -Path $headerPath -Raw
    $sourceText = Get-Content -Path $sourcePath -Raw

    if ($headerText -notmatch 'bool\s+TokenInComment\s*\(\s*const\s+core::SourceFile&\s+source,\s*const\s+Token&\s+token\s*\);') {
        throw "Case 'issue587_token_in_comment_conformance' missing TokenInComment declaration in lexer.h."
    }

    foreach ($pattern in @(
        'bool\s+TokenInComment\s*\(\s*const\s+core::SourceFile&\s+source,\s*const\s+Token&\s+token\s*\)',
        'const\s+auto\s+range\s*=\s*TokenRange\(source,\s*token\);',
        'const\s+auto\s+\[i,\s*j\]\s*=\s*\*range;',
        'CommentScanResult\s+line\s*=\s*ScanLineComment\(source,\s*p\);',
        'CommentScanResult\s+block\s*=\s*ScanBlockComment\(source,\s*p\);',
        'p\s*<=\s*i\s*&&\s*j\s*<=\s*line\.next',
        'p\s*<=\s*i\s*&&\s*j\s*<=\s*block\.next'
    )) {
        if ($sourceText -notmatch $pattern) {
            throw "Case 'issue587_token_in_comment_conformance' missing expected lexer_ws.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue587_token_in_comment_conformance: header_decl=1 source_patterns=7"
}

function Invoke-Issue588ScalarIndexConformanceCase {
    $sourceTextPath = Join-Path $workspaceRoot "cursive\\include\\00_core\\source_text.h"
    $tokenPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\token.cpp"
    $spanTempPath = Join-Path $workspaceRoot "cursive\\src\\00_core\\source_load.cpp"
    foreach ($path in @($sourceTextPath, $tokenPath, $spanTempPath)) {
        if (-not (Test-Path $path)) {
            throw "Case 'issue588_scalar_index_conformance' missing compiler source file: $path"
        }
    }

    $sourceText = Get-Content -Path $sourceTextPath -Raw
    $tokenText = Get-Content -Path $tokenPath -Raw
    $spanTempText = Get-Content -Path $spanTempPath -Raw

    foreach ($pattern in @(
        'offsets\.reserve\(scalars\.size\(\)\s*\+\s*1\);',
        'offsets\.push_back\(0\);',
        'for\s*\(UnicodeScalar\s+u\s*:\s*scalars\)\s*\{[\s\S]*offsets\.push_back\(acc\);',
        'return\s+offsets;'
    )) {
        if ($sourceText -notmatch $pattern) {
            throw "Case 'issue588_scalar_index_conformance' missing expected source_text.h pattern '$pattern'."
        }
    }

    foreach ($pattern in @(
        'std::find\(offsets\.begin\(\),\s*offsets\.end\(\),\s*token\.span\.start_offset\)',
        'std::find\(offsets\.begin\(\),\s*offsets\.end\(\),\s*token\.span\.end_offset\)',
        'return\s+std::pair<std::size_t,\s*std::size_t>\{i,\s*j\};'
    )) {
        if ($tokenText -notmatch $pattern) {
            throw "Case 'issue588_scalar_index_conformance' missing expected token.cpp pattern '$pattern'."
        }
    }

    foreach ($pattern in @(
        'index\s*<\s*source\.line_starts\.size\(\)\s*\?\s*source\.line_starts\[index\]\s*:\s*source\.byte_len',
        'start\s*<\s*source\.byte_len\s*\?\s*std::min\(start\s*\+\s*1,\s*source\.byte_len\)\s*:\s*source\.byte_len'
    )) {
        if ($spanTempText -notmatch $pattern) {
            throw "Case 'issue588_scalar_index_conformance' missing expected source_load.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue588_scalar_index_conformance: source_text_patterns=4 token_patterns=3 span_patterns=2"
}

function Invoke-Issue589LexemeScalarSliceConformanceCase {
    $tokenHeaderPath = Join-Path $workspaceRoot "cursive\\include\\02_source\\lexer\\token.h"
    $lexerHeaderPath = Join-Path $workspaceRoot "cursive\\include\\02_source\\lexer\\lexer.h"
    $tokenizePath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\tokenize.cpp"
    $identPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\lexer_ident.cpp"
    $literalPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\lexer_literals.cpp"
    foreach ($path in @($tokenHeaderPath, $lexerHeaderPath, $tokenizePath, $identPath, $literalPath)) {
        if (-not (Test-Path $path)) {
            throw "Case 'issue589_lexeme_scalar_slice_conformance' missing compiler source file: $path"
        }
    }

    $tokenHeaderText = Get-Content -Path $tokenHeaderPath -Raw
    $lexerHeaderText = Get-Content -Path $lexerHeaderPath -Raw
    $tokenizeText = Get-Content -Path $tokenizePath -Raw
    $identText = Get-Content -Path $identPath -Raw
    $literalText = Get-Content -Path $literalPath -Raw

    foreach ($pattern in @(
        'using\s+Lexeme\s*=\s*std::string;',
        'using\s+LexemeScalars\s*=\s*core::Scalars;'
    )) {
        if ($tokenHeaderText -notmatch $pattern) {
            throw "Case 'issue589_lexeme_scalar_slice_conformance' missing expected token.h pattern '$pattern'."
        }
    }

    if ($lexerHeaderText -notmatch 'LexemeScalars\s+LexemeSliceScalars\s*\(\s*const\s+std::vector<core::UnicodeScalar>&\s+scalars,\s*std::size_t\s+i,\s*std::size_t\s+j\s*\);') {
        throw "Case 'issue589_lexeme_scalar_slice_conformance' missing LexemeSliceScalars declaration in lexer.h."
    }

    foreach ($pattern in @(
        'LexemeScalars\s+LexemeSliceScalars\s*\(\s*const\s+std::vector<core::UnicodeScalar>&\s+scalars,\s*std::size_t\s+i,\s*std::size_t\s+j\s*\)',
        'if\s*\(\s*j\s*<\s*i\s*\|\|\s*j\s*>\s*scalars\.size\(\)\s*\)\s*\{\s*return\s*\{\s*\};\s*\}',
        'return\s+LexemeScalars\(scalars\.begin\(\)\s*\+\s*static_cast<std::ptrdiff_t>\(i\),\s*scalars\.begin\(\)\s*\+\s*static_cast<std::ptrdiff_t>\(j\)\);',
        'const\s+LexemeScalars\s+lexeme\s*=\s*LexemeSliceScalars\(\s*\*input\.scalars,\s*i,\s*j\s*\);',
        'return\s+core::EncodeUtf8\(lexeme\);'
    )) {
        if ($tokenizeText -notmatch $pattern) {
            throw "Case 'issue589_lexeme_scalar_slice_conformance' missing expected tokenize.cpp pattern '$pattern'."
        }
    }

    if ($identText -notmatch 'result\.lexeme\s*=\s*core::EncodeUtf8\(LexemeSliceScalars\(scalars,\s*start,\s*end\)\);') {
        throw "Case 'issue589_lexeme_scalar_slice_conformance' missing scalar-slice identifier encoding in lexer_ident.cpp."
    }

    if ($literalText -notmatch 'return\s+core::EncodeUtf8\(LexemeSliceScalars\(source\.scalars,\s*i,\s*j\)\);') {
        throw "Case 'issue589_lexeme_scalar_slice_conformance' missing scalar-slice literal encoding in lexer_literals.cpp."
    }

    Write-Host "[compiler-static] issue589_lexeme_scalar_slice_conformance: token_header_patterns=2 lexer_header_decl=1 tokenize_patterns=5 ident_patterns=1 literal_patterns=1"
}

function Invoke-Issue590ReservedNamespacePrefixConformanceCase {
    $scopesHeaderPath = Join-Path $workspaceRoot "cursive\\include\\04_analysis\\resolve\\scopes.h"
    $scopesPath = Join-Path $workspaceRoot "cursive\\src\\04_analysis\\resolve\\scopes.cpp"
    $introPath = Join-Path $workspaceRoot "cursive\\src\\04_analysis\\resolve\\scopes_intro.cpp"
    $ifCasePath = Join-Path $workspaceRoot "cursive\\src\\04_analysis\\typing\\if_case_check.cpp"
    $stmtCommonPath = Join-Path $workspaceRoot "cursive\\src\\04_analysis\\typing\\stmt\\stmt_common.cpp"
    foreach ($path in @($scopesHeaderPath, $scopesPath, $introPath, $ifCasePath, $stmtCommonPath)) {
        if (-not (Test-Path $path)) {
            throw "Case 'issue590_reserved_namespace_prefix_conformance' missing compiler source file: $path"
        }
    }

    $headerText = Get-Content -Path $scopesHeaderPath -Raw
    $scopesText = Get-Content -Path $scopesPath -Raw
    $introText = Get-Content -Path $introPath -Raw
    $ifCaseText = Get-Content -Path $ifCasePath -Raw
    $stmtCommonText = Get-Content -Path $stmtCommonPath -Raw

    if ($headerText -match 'ReservedCursive\s*\(' -or $headerText -match 'ReservedId\s*\(') {
        throw "Case 'issue590_reserved_namespace_prefix_conformance' found stale bare-cursive reserved-id declarations in scopes.h."
    }

    foreach ($pattern in @(
        'bool\s+ReservedGen\s*\(\s*std::string_view\s+x\s*\)',
        'bool\s+ReservedModulePath\s*\(\s*const\s+ast::ModulePath&\s+path\s*\)',
        'if\s*\(!path\.empty\(\)\s*&&\s*IdEq\(path\[0\],\s*"cursive"\)\)\s*\{\s*return\s+true;\s*\}',
        'scope\.emplace\(IdKeyOf\(name\),\s*Entity\{EntityKind::Type'
    )) {
        if ($scopesText -notmatch $pattern) {
            throw "Case 'issue590_reserved_namespace_prefix_conformance' missing expected scopes.cpp pattern '$pattern'."
        }
    }

    foreach ($forbidden in @(
        'bool\s+ReservedCursive\s*\(',
        'bool\s+ReservedId\s*\(',
        'IdKeyOf\("cursive"\)',
        'ModuleAlias,\s*ast::ModulePath\{"cursive"\}'
    )) {
        if ($scopesText -match $forbidden) {
            throw "Case 'issue590_reserved_namespace_prefix_conformance' found stale bare-cursive reservation pattern '$forbidden' in scopes.cpp."
        }
    }

    foreach ($forbidden in @(
        'ReservedCursive\(name\)',
        'Intro-Reserved-Cursive-Err',
        'Shadow-Reserved-Cursive-Err',
        'ReservedCursive\(key\)'
    )) {
        if ($introText -match $forbidden) {
            throw "Case 'issue590_reserved_namespace_prefix_conformance' found stale bare-cursive restriction pattern '$forbidden' in scopes_intro.cpp."
        }
    }

    foreach ($forbidden in @(
        'ReservedCursive\(name\)',
        'Intro-Reserved-Cursive-Err',
        'Shadow-Reserved-Cursive-Err'
    )) {
        if ($ifCaseText -match $forbidden) {
            throw "Case 'issue590_reserved_namespace_prefix_conformance' found stale bare-cursive restriction pattern '$forbidden' in if_case_check.cpp."
        }
        if ($stmtCommonText -match $forbidden) {
            throw "Case 'issue590_reserved_namespace_prefix_conformance' found stale bare-cursive restriction pattern '$forbidden' in stmt_common.cpp."
        }
    }

    Write-Host "[compiler-static] issue590_reserved_namespace_prefix_conformance: header_forbidden=2 scopes_patterns=4 scopes_forbidden=4 intro_forbidden=4 if_case_forbidden=3 stmt_forbidden=3"
}

function Invoke-Issue591UniverseProtectedSetConformanceCase {
    $scopesPath = Join-Path $workspaceRoot "cursive\\src\\04_analysis\\resolve\\scopes.cpp"
    if (-not (Test-Path $scopesPath)) {
        throw "Case 'issue591_universe_protected_set_conformance' missing compiler source file: $scopesPath"
    }

    $scopesText = Get-Content -Path $scopesPath -Raw
    $match = [regex]::Match($scopesText, 'UniverseProtectedNames\s*\(\)\s*\{[\s\S]*?static\s+const\s+std::vector<std::string_view>\s+names\s*=\s*\{(?<body>[\s\S]*?)\};', [System.Text.RegularExpressions.RegexOptions]::Singleline)
    if (-not $match.Success) {
        throw "Case 'issue591_universe_protected_set_conformance' could not locate UniverseProtectedNames set in scopes.cpp."
    }

    $body = $match.Groups['body'].Value
    $required = @(
        "i8", "i16", "i32", "i64", "i128", "u8", "u16", "u32", "u64", "u128",
        "f16", "f32", "f64", "bool", "char", "usize", "isize", "Self", "Drop",
        "Bitcopy", "Clone", "Eq", "Hash", "Hasher", "Iterator", "Step", "FfiSafe",
        "string", "bytes", "Modal", "Region", "RegionOptions", "CancelToken",
        "Context", "System", "Network", "ExecutionDomain", "Reactor", "CpuSet",
        "Priority", "Async", "Future", "Sequence", "Stream", "Pipe", "Exchange",
        "Tracked", "Spawned"
    )
    foreach ($entry in $required) {
        if ($body -notmatch ('"' + [regex]::Escape($entry) + '"')) {
            throw "Case 'issue591_universe_protected_set_conformance' missing expected UniverseProtected name '$entry'."
        }
    }

    foreach ($forbidden in @(
        "ProjectFiles", "TypeEmitter", "Introspect", "ComptimeDiagnostics",
        "Type", "Ast", "TypeCategory", "FieldInfo", "VariantInfo", "StateInfo", "SourceSpan"
    )) {
        if ($body -match ('"' + [regex]::Escape($forbidden) + '"')) {
            throw "Case 'issue591_universe_protected_set_conformance' found stale extra UniverseProtected name '$forbidden'."
        }
    }

    Write-Host "[compiler-static] issue591_universe_protected_set_conformance: required=$($required.Count) forbidden=11"
}

function Invoke-Issue592BlockStateConformanceCase {
    $headerPath = Join-Path $workspaceRoot "cursive\\include\\02_source\\lexer\\lexer.h"
    $sourcePath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\lexer_ws.cpp"
    foreach ($path in @($headerPath, $sourcePath)) {
        if (-not (Test-Path $path)) {
            throw "Case 'issue592_block_state_conformance' missing compiler source file: $path"
        }
    }

    $headerText = Get-Content -Path $headerPath -Raw
    $sourceText = Get-Content -Path $sourcePath -Raw

    foreach ($pattern in @(
        'struct\s+BlockScanState\s*\{[\s\S]*std::size_t\s+index\s*=\s*0;[\s\S]*std::size_t\s+depth\s*=\s*0;[\s\S]*std::size_t\s+start_index\s*=\s*0;[\s\S]*\}',
        'struct\s+BlockDoneState\s*\{[\s\S]*std::size_t\s+next\s*=\s*0;[\s\S]*\}',
        'using\s+BlockState\s*=\s*std::variant<BlockScanState,\s*BlockDoneState>;'
    )) {
        if ($headerText -notmatch $pattern) {
            throw "Case 'issue592_block_state_conformance' missing expected lexer.h pattern '$pattern'."
        }
    }

    foreach ($pattern in @(
        'BlockState\s+state\s*=\s*BlockScanState\{start,\s*0,\s*start\};',
        'auto\*\s+scan\s*=\s*std::get_if<BlockScanState>\(&state\);',
        'state\s*=\s*BlockDoneState\{scan->index\s*\+\s*2\};',
        'if\s*\(const\s+auto\*\s+done\s*=\s*std::get_if<BlockDoneState>\(&state\)\)',
        'const\s+auto\*\s+unterminated\s*=\s*std::get_if<BlockScanState>\(&state\);'
    )) {
        if ($sourceText -notmatch $pattern) {
            throw "Case 'issue592_block_state_conformance' missing expected lexer_ws.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue592_block_state_conformance: header_patterns=3 source_patterns=5"
}

function Invoke-Issue593AtHelperConformanceCase {
    $sourcePath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\lexer_literals.cpp"
    if (-not (Test-Path $sourcePath)) {
        throw "Case 'issue593_at_helper_conformance' missing compiler source file: $sourcePath"
    }

    $sourceText = Get-Content -Path $sourcePath -Raw

    foreach ($pattern in @(
        'char\s+At\s*\(\s*std::string_view\s+s,\s*std::size_t\s+i\s*\)\s*\{\s*return\s+s\[i\];\s*\}',
        'At\(s,\s*0\)\s*==\s*''_''',
        'At\(s,\s*s\.size\(\)\s*-\s*1\)\s*==\s*''_''',
        'At\(s,\s*i\s*-\s*1\)\s*==\s*''e''',
        'At\(s,\s*i\s*\+\s*1\)\s*==\s*''E''',
        'At\(s,\s*s\.size\(\)\s*-\s*suf\.size\(\)\s*-\s*1\)\s*==\s*''_''',
        'At\(digits,\s*0\)\s*==\s*''0'''
    )) {
        if ($sourceText -notmatch $pattern) {
            throw "Case 'issue593_at_helper_conformance' missing expected lexer_literals.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue593_at_helper_conformance: source_patterns=7"
}

function Invoke-Issue594RemoveHelperConformanceCase {
    $sourcePath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\lexer_literals.cpp"
    if (-not (Test-Path $sourcePath)) {
        throw "Case 'issue594_remove_helper_conformance' missing compiler source file: $sourcePath"
    }

    $sourceText = Get-Content -Path $sourcePath -Raw

    foreach ($pattern in @(
        'std::string\s+Remove\s*\(\s*std::string_view\s+s,\s*char\s+c\s*\)\s*\{',
        'out\.reserve\(s\.size\(\)\);',
        'for\s*\(char\s+x\s*:\s*s\)\s*\{',
        'if\s*\(x\s*!=\s*c\)\s*\{\s*out\.push_back\(x\);',
        'return\s+out;',
        'const\s+std::string\s+digits\s*=\s*Remove\(s,\s*''_''\);'
    )) {
        if ($sourceText -notmatch $pattern) {
            throw "Case 'issue594_remove_helper_conformance' missing expected lexer_literals.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue594_remove_helper_conformance: source_patterns=6"
}

function Invoke-Issue595ConcatHelperConformanceCase {
    $sourcePath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\lexer_literals.cpp"
    if (-not (Test-Path $sourcePath)) {
        throw "Case 'issue595_concat_helper_conformance' missing compiler source file: $sourcePath"
    }

    $sourceText = Get-Content -Path $sourcePath -Raw

    foreach ($pattern in @(
        'std::string\s+Concat\s*\(\s*std::initializer_list<std::string_view>\s+parts\s*\)\s*\{',
        'if\s*\(parts\.size\(\)\s*==\s*0\)\s*\{\s*return\s+std::string\(\);\s*\}',
        'std::string\s+ConcatSuffix\s*\(',
        'StartsWith\(s,\s*Concat\(\{\"0x\",\s*"_"\}\)\)',
        'EndsWith\(s,\s*Concat\(\{\"_\",\s*suf\}\)\)'
    )) {
        if ($sourceText -notmatch $pattern) {
            throw "Case 'issue595_concat_helper_conformance' missing expected lexer_literals.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue595_concat_helper_conformance: source_patterns=5"
}

function Invoke-Issue596ConcatSingletonConformanceCase {
    $sourcePath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\lexer_literals.cpp"
    if (-not (Test-Path $sourcePath)) {
        throw "Case 'issue596_concat_singleton_conformance' missing compiler source file: $sourcePath"
    }

    $sourceText = Get-Content -Path $sourcePath -Raw

    foreach ($pattern in @(
        'std::string\s+Concat\s*\(\s*std::initializer_list<std::string_view>\s+parts\s*\)\s*\{',
        'if\s*\(parts\.size\(\)\s*==\s*1\)\s*\{\s*return\s+std::string\(\*parts\.begin\(\)\);\s*\}'
    )) {
        if ($sourceText -notmatch $pattern) {
            throw "Case 'issue596_concat_singleton_conformance' missing expected lexer_literals.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue596_concat_singleton_conformance: source_patterns=2"
}

function Invoke-Issue597ConcatRecursiveConformanceCase {
    $sourcePath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\lexer_literals.cpp"
    if (-not (Test-Path $sourcePath)) {
        throw "Case 'issue597_concat_recursive_conformance' missing compiler source file: $sourcePath"
    }

    $sourceText = Get-Content -Path $sourcePath -Raw

    foreach ($pattern in @(
        'std::string\s+ConcatSuffix\s*\(\s*std::initializer_list<std::string_view>\s+parts,\s*std::initializer_list<std::string_view>::const_iterator\s+first\s*\)\s*\{',
        'return\s+std::string\(\*first\)\s*\+\s*ConcatSuffix\(parts,\s*next\);',
        'auto\s+tail\s*=\s*parts\.begin\(\);\s*\+\+tail;\s*return\s+std::string\(\*parts\.begin\(\)\)\s*\+\s*ConcatSuffix\(parts,\s*tail\);'
    )) {
        if ($sourceText -notmatch $pattern) {
            throw "Case 'issue597_concat_recursive_conformance' missing expected lexer_literals.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue597_concat_recursive_conformance: source_patterns=3"
}

function Invoke-Issue598HexValueSequenceConformanceCase {
    $sourcePath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\lexer_literals.cpp"
    if (-not (Test-Path $sourcePath)) {
        throw "Case 'issue598_hex_value_sequence_conformance' missing compiler source file: $sourcePath"
    }

    $sourceText = Get-Content -Path $sourcePath -Raw

    foreach ($pattern in @(
        'std::uint64_t\s+HexValue\s*\(\s*std::span<const\s+UnicodeScalar>\s+digits\s*\)\s*\{',
        'for\s*\(UnicodeScalar\s+digit\s*:\s*digits\)\s*\{\s*value\s*=\s*\(value\s*<<\s*4\)\s*\|\s*HexValue\(digit\);',
        'const\s+auto\s+hex_digits\s*=\s*std::span<const\s+UnicodeScalar>\(scalars\.data\(\)\s*\+\s*digits_start,\s*digits\);',
        'const\s+std::uint64_t\s+value\s*=\s*HexValue\(hex_digits\);'
    )) {
        if ($sourceText -notmatch $pattern) {
            throw "Case 'issue598_hex_value_sequence_conformance' missing expected lexer_literals.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue598_hex_value_sequence_conformance: source_patterns=4"
}

function Invoke-Issue599DecDigitValueConformanceCase {
    $sourcePath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\lexer_literals.cpp"
    if (-not (Test-Path $sourcePath)) {
        throw "Case 'issue599_dec_digit_value_conformance' missing compiler source file: $sourcePath"
    }

    $sourceText = Get-Content -Path $sourcePath -Raw

    foreach ($pattern in @(
        'unsigned\s+int\s+DecDigitValue\s*\(\s*UnicodeScalar\s+c\s*\)\s*\{',
        'return\s+static_cast<unsigned\s+int>\(c\s*-\s*''0''\);',
        'if\s*\(c\s*>=\s*''0''\s*&&\s*c\s*<=\s*''9''\)\s*\{\s*return\s+DecDigitValue\(c\);'
    )) {
        if ($sourceText -notmatch $pattern) {
            throw "Case 'issue599_dec_digit_value_conformance' missing expected lexer_literals.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue599_dec_digit_value_conformance: source_patterns=3"
}

function Invoke-Issue600OctDigitValueConformanceCase {
    $sourcePath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\lexer_literals.cpp"
    if (-not (Test-Path $sourcePath)) {
        throw "Case 'issue600_oct_digit_value_conformance' missing compiler source file: $sourcePath"
    }

    $sourceText = Get-Content -Path $sourcePath -Raw

    foreach ($pattern in @(
        'unsigned\s+int\s+OctDigitValue\s*\(\s*UnicodeScalar\s+c\s*\)\s*\{',
        'return\s+static_cast<unsigned\s+int>\(c\s*-\s*''0''\);',
        'bool\s+IsOctDigit\s*\(\s*UnicodeScalar\s+c\s*\)\s*\{\s*return\s+c\s*>=\s*''0''\s*&&\s*c\s*<=\s*''7'';'
    )) {
        if ($sourceText -notmatch $pattern) {
            throw "Case 'issue600_oct_digit_value_conformance' missing expected lexer_literals.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue600_oct_digit_value_conformance: source_patterns=3"
}

function Invoke-Issue601BinDigitValueConformanceCase {
    $sourcePath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\lexer_literals.cpp"
    if (-not (Test-Path $sourcePath)) {
        throw "Case 'issue601_bin_digit_value_conformance' missing compiler source file: $sourcePath"
    }

    $sourceText = Get-Content -Path $sourcePath -Raw

    foreach ($pattern in @(
        'unsigned\s+int\s+BinDigitValue\s*\(\s*UnicodeScalar\s+c\s*\)\s*\{',
        'return\s+static_cast<unsigned\s+int>\(c\s*-\s*''0''\);',
        'bool\s+IsBinDigit\s*\(\s*UnicodeScalar\s+c\s*\)\s*\{\s*return\s+c\s*==\s*''0''\s*\|\|\s*c\s*==\s*''1'';'
    )) {
        if ($sourceText -notmatch $pattern) {
            throw "Case 'issue601_bin_digit_value_conformance' missing expected lexer_literals.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue601_bin_digit_value_conformance: source_patterns=3"
}

function Invoke-Issue602DecValueSequenceConformanceCase {
    $sourcePath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\lexer_literals.cpp"
    if (-not (Test-Path $sourcePath)) {
        throw "Case 'issue602_dec_value_sequence_conformance' missing compiler source file: $sourcePath"
    }

    $sourceText = Get-Content -Path $sourcePath -Raw

    foreach ($pattern in @(
        'std::uint64_t\s+DecValue\s*\(\s*std::span<const\s+UnicodeScalar>\s+digits\s*\)\s*\{',
        'for\s*\(UnicodeScalar\s+digit\s*:\s*digits\)\s*\{\s*value\s*=\s*\(value\s*\*\s*10u\)\s*\+\s*DecDigitValue\(digit\);',
        'return\s+value;'
    )) {
        if ($sourceText -notmatch $pattern) {
            throw "Case 'issue602_dec_value_sequence_conformance' missing expected lexer_literals.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue602_dec_value_sequence_conformance: source_patterns=3"
}

function Invoke-Issue603OctValueSequenceConformanceCase {
    $sourcePath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\lexer_literals.cpp"
    if (-not (Test-Path $sourcePath)) {
        throw "Case 'issue603_oct_value_sequence_conformance' missing compiler source file: $sourcePath"
    }

    $sourceText = Get-Content -Path $sourcePath -Raw

    foreach ($pattern in @(
        'std::uint64_t\s+OctValue\s*\(\s*std::span<const\s+UnicodeScalar>\s+digits\s*\)\s*\{',
        'for\s*\(UnicodeScalar\s+digit\s*:\s*digits\)\s*\{\s*value\s*=\s*\(value\s*\*\s*8u\)\s*\+\s*OctDigitValue\(digit\);',
        'return\s+value;'
    )) {
        if ($sourceText -notmatch $pattern) {
            throw "Case 'issue603_oct_value_sequence_conformance' missing expected lexer_literals.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue603_oct_value_sequence_conformance: source_patterns=3"
}

function Invoke-Issue604BinValueSequenceConformanceCase {
    $sourcePath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\lexer_literals.cpp"
    if (-not (Test-Path $sourcePath)) {
        throw "Case 'issue604_bin_value_sequence_conformance' missing compiler source file: $sourcePath"
    }

    $sourceText = Get-Content -Path $sourcePath -Raw

    foreach ($pattern in @(
        'std::uint64_t\s+BinValue\s*\(\s*std::span<const\s+UnicodeScalar>\s+digits\s*\)\s*\{',
        'for\s*\(UnicodeScalar\s+digit\s*:\s*digits\)\s*\{\s*value\s*=\s*\(value\s*\*\s*2u\)\s*\+\s*BinDigitValue\(digit\);',
        'return\s+value;'
    )) {
        if ($sourceText -notmatch $pattern) {
            throw "Case 'issue604_bin_value_sequence_conformance' missing expected lexer_literals.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue604_bin_value_sequence_conformance: source_patterns=3"
}

function Invoke-Issue605SuffixMatchConformanceCase {
    $sourcePath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\lexer_literals.cpp"
    if (-not (Test-Path $sourcePath)) {
        throw "Case 'issue605_suffix_match_conformance' missing compiler source file: $sourcePath"
    }

    $sourceText = Get-Content -Path $sourcePath -Raw

    foreach ($pattern in @(
        'std::size_t\s+SuffixMatch\s*\(\s*const\s+std::vector<UnicodeScalar>&\s+scalars,\s*std::size_t\s+start,\s*std::span<const\s+std::string_view>\s+suffixes\s*\)\s*\{',
        'for\s*\(std::string_view\s+suffix\s*:\s*suffixes\)\s*\{\s*const\s+std::size_t\s+len\s*=\s*MatchSuffix\(scalars,\s*start,\s*suffix\);',
        'if\s*\(len\s*>\s*longest\)\s*\{\s*longest\s*=\s*len;\s*\}',
        'return\s+SuffixMatch\(scalars,\s*start,\s*kIntSuffixes\);',
        'return\s+SuffixMatch\(scalars,\s*start,\s*kFloatSuffixes\);'
    )) {
        if ($sourceText -notmatch $pattern) {
            throw "Case 'issue605_suffix_match_conformance' missing expected lexer_literals.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue605_suffix_match_conformance: source_patterns=5"
}

function Invoke-Issue606HasDotConformanceCase {
    $sourcePath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\lexer_literals.cpp"
    if (-not (Test-Path $sourcePath)) {
        throw "Case 'issue606_has_dot_conformance' missing compiler source file: $sourcePath"
    }

    $sourceText = Get-Content -Path $sourcePath -Raw

    foreach ($pattern in @(
        'bool\s+HasDot\s*\(\s*const\s+std::vector<UnicodeScalar>&\s+scalars,\s*std::size_t\s+start,\s*std::size_t\s+end\s*\)\s*\{',
        'for\s*\(std::size_t\s+p\s*=\s*start;\s*p\s*<\s*end;\s*\+\+p\)\s*\{\s*if\s*\(scalars\[p\]\s*==\s*''\.''\)\s*\{\s*return\s+true;',
        'if\s*\(!HasDot\(scalars,\s*start,\s*p\)\)\s*\{\s*return\s+result;'
    )) {
        if ($sourceText -notmatch $pattern) {
            throw "Case 'issue606_has_dot_conformance' missing expected lexer_literals.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue606_has_dot_conformance: source_patterns=3"
}

function Invoke-Issue607HasExpConformanceCase {
    $sourcePath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\lexer_literals.cpp"
    if (-not (Test-Path $sourcePath)) {
        throw "Case 'issue607_has_exp_conformance' missing compiler source file: $sourcePath"
    }

    $sourceText = Get-Content -Path $sourcePath -Raw

    foreach ($pattern in @(
        'bool\s+HasExp\s*\(\s*const\s+std::vector<UnicodeScalar>&\s+scalars,\s*std::size_t\s+start,\s*std::size_t\s+end\s*\)\s*\{',
        'for\s*\(std::size_t\s+p\s*=\s*start;\s*p\s*<\s*end;\s*\+\+p\)\s*\{\s*if\s*\(scalars\[p\]\s*==\s*''e''\s*\|\|\s*scalars\[p\]\s*==\s*''E''\)\s*\{\s*return\s+true;',
        'saw_exp\s*=\s*HasExp\(scalars,\s*start,\s*p\);'
    )) {
        if ($sourceText -notmatch $pattern) {
            throw "Case 'issue607_has_exp_conformance' missing expected lexer_literals.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue607_has_exp_conformance: source_patterns=3"
}

function Invoke-Issue608HasFloatCoreConformanceCase {
    $sourcePath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\lexer_literals.cpp"
    if (-not (Test-Path $sourcePath)) {
        throw "Case 'issue608_has_float_core_conformance' missing compiler source file: $sourcePath"
    }

    $sourceText = Get-Content -Path $sourcePath -Raw

    foreach ($pattern in @(
        'bool\s+HasFloatCore\s*\(\s*const\s+std::vector<UnicodeScalar>&\s+scalars,\s*std::size_t\s+start,\s*std::size_t\s+end\s*\)\s*\{',
        'return\s+HasDot\(scalars,\s*start,\s*end\);',
        'if\s*\(!HasFloatCore\(scalars,\s*start,\s*p\)\)\s*\{\s*return\s+result;'
    )) {
        if ($sourceText -notmatch $pattern) {
            throw "Case 'issue608_has_float_core_conformance' missing expected lexer_literals.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue608_has_float_core_conformance: source_patterns=3"
}

function Invoke-Issue609DecimalLeadingZeroConformanceCase {
    $sourcePath = Join-Path $workspaceRoot "cursive\\src\\02_source\\lexer\\lexer_literals.cpp"
    if (-not (Test-Path $sourcePath)) {
        throw "Case 'issue609_decimal_leading_zero_conformance' missing compiler source file: $sourcePath"
    }

    $sourceText = Get-Content -Path $sourcePath -Raw

    foreach ($pattern in @(
        'bool\s+DecimalLeadingZero\s*\(\s*const\s+core::SourceFile&\s+source,\s*const\s+std::vector<std::size_t>&\s+offsets,\s*std::size_t\s+i,\s*std::size_t\s+j\s*\)\s*\{',
        'const\s+std::string\s+lexeme\s*=\s*LexemeSlice\(source,\s*offsets,\s*i,\s*j\);',
        'return\s+MatchesDecimalIntegerLexeme\(lexeme\)\s*&&\s*DecimalLeadingZero\(lexeme\);',
        'underscore_ok\s*&&\s*DecimalLeadingZero\(source,\s*offsets,\s*start,\s*j\)'
    )) {
        if ($sourceText -notmatch $pattern) {
            throw "Case 'issue609_decimal_leading_zero_conformance' missing expected lexer_literals.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] issue609_decimal_leading_zero_conformance: source_patterns=4"
}

function Invoke-Issue5131SpecCanonicalityCase {
    $specPath = $canonicalSpecPath
    if (-not (Test-Path $specPath)) {
        throw "Case 'issue5131_spec_canonicality' missing canonical spec file: $specPath"
    }

    $text = Get-Content -Path $specPath -Raw
    $presentChecks = @(
        @{
            Pattern = 'attribute_name\s*::=\s*identifier\s*\|\s*vendor_prefix\s*"::"\s*identifier'
            Label = "attribute_name scoped vendor syntax"
        },
        @{
            Pattern = 'vendor_prefix\s*::=\s*identifier\s*\("::"\s*identifier\)\*'
            Label = "vendor_prefix scoped segments"
        },
        @{
            Pattern = 'AttrTarget = \{Record, Enum, Modal, Procedure, Method, Field, Binding, Statement, Expression, KeyBlock, ExternBlock, TypeAlias\}'
            Label = "AttrTarget includes method/statement/keyblock"
        },
        @{
            Pattern = 'AttrTargets\(inline\) = \{Procedure, Method\}'
            Label = "inline method allowlist"
        },
        @{
            Pattern = 'AttrTargets\(cold\) = \{Procedure, Method\}'
            Label = "cold method allowlist"
        },
        @{
            Pattern = 'AttrTargets\(deprecated\) = \{Record, Enum, Modal, Procedure, Method, Field, Binding, TypeAlias\}'
            Label = "deprecated method allowlist"
        },
        @{
            Pattern = 'AttrTargets\(dynamic\) = \{Record, Enum, Modal, Procedure, Method, Expression\}'
            Label = "dynamic method allowlist"
        },
        @{
            Pattern = 'AttrTargets\(log\) = \{Procedure, Method, Binding, Expression, Statement\}'
            Label = "log method allowlist"
        },
        @{
            Pattern = 'AttrTargets\(mangle\) = \{Procedure\}'
            Label = "mangle FFI target"
        },
        @{
            Pattern = 'AttrTargets\(library\) = \{ExternBlock\}'
            Label = "library FFI target"
        },
        @{
            Pattern = 'AttrTargets\(unwind\) = \{Procedure\}'
            Label = "unwind FFI target"
        },
        @{
            Pattern = 'AttrTargets\(export\) = \{Procedure\}'
            Label = "export FFI target"
        },
        @{
            Pattern = 'AttrTargets\(host_export\) = \{Procedure\}'
            Label = "host_export FFI target"
        },
        @{
            Pattern = 'AttrTargets\(ffi_pass_by_value\) = \{Record, Enum\}'
            Label = "ffi_pass_by_value FFI target"
        },
        @{
            Pattern = 'FFI-specific attributes `mangle`, `library`, `unwind`, `export`, `host_export`, and `ffi_pass_by_value` are defined by §23\.4\.'
            Label = "FFI attribute ownership cross-reference"
        }
    )

    $absentChecks = @(
        @{
            Pattern = 'IsPunc\(Tok\(P\), "\."\)'
            Label = "legacy dotted vendor tail end rule"
        },
        @{
            Pattern = 'IsPunc\(Tok\(P\), "\."\)[\s\S]*ParseVendorPrefixTail'
            Label = "legacy dotted vendor tail cons rule"
        },
        @{
            Pattern = 'AttrTarget = \{Record, Enum, Modal, Procedure, Field, Binding, Expression, ExternBlock, TypeAlias\}'
            Label = "legacy AttrTarget set missing method/statement/keyblock"
        }
    )

    $implementationChecks = @(
        @{
            Path = "cursive\\src\\04_analysis\\resolve\\resolve_attributes.cpp"
            Pattern = 'GetAttributeRegistry\(\)\.Lookup\(name\)\s*!=\s*nullptr'
            Label = "attribute-name validation delegates to canonical registry"
        },
        @{
            Path = "cursive\\src\\04_analysis\\resolve\\resolve_attributes.cpp"
            Pattern = 'GetAttributeRegistry\(\)\.IsValidForTarget\(attr_name,\s*\*target\)'
            Label = "attribute-target validation delegates to canonical registry"
        }
    )

    foreach ($check in $presentChecks) {
        if ($text -notmatch $check.Pattern) {
            throw "Case 'issue5131_spec_canonicality' missing expected canonical pattern '$($check.Label)'."
        }
    }

    foreach ($check in $absentChecks) {
        if ($text -match $check.Pattern) {
            throw "Case 'issue5131_spec_canonicality' found stale/non-canonical pattern '$($check.Label)'."
        }
    }

    foreach ($check in $implementationChecks) {
        $fullPath = Join-Path $workspaceRoot $check.Path
        if (-not (Test-Path $fullPath)) {
            throw "Case 'issue5131_spec_canonicality' missing implementation file: $fullPath"
        }

        $implementationText = Get-Content -Path $fullPath -Raw
        if ($implementationText -notmatch $check.Pattern) {
            throw "Case 'issue5131_spec_canonicality' missing expected implementation pattern '$($check.Label)' in $fullPath."
        }
    }

    Write-Host "[compiler-static] issue5131_spec_canonicality: present_checks=$($presentChecks.Count) absent_checks=$($absentChecks.Count) implementation_checks=$($implementationChecks.Count)"
}

function Invoke-Issue5131AttrSpecTrailingCommaConformanceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue5131_attr_spec_trailing_comma_multiline_trace" `
        -Source (New-Issue5131AttrSpecTrailingCommaMultilineSource) `
        -ConformanceFileName "issue5131_attr_spec_trailing_comma_multiline_trace.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue5131_attr_spec_trailing_comma_multiline_trace' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue5131_attr_spec_trailing_comma_multiline_trace' expected zero compile-time errors but observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $tailRuleCount = @($logLines | Where-Object {
        $_ -like "*`tParse-AttrSpecListTail-TrailingComma`t*"
    }).Count
    $errRuleCount = @($logLines | Where-Object {
        $_ -like "*`tTrailing-Comma-Err`t*"
    }).Count
    if ($tailRuleCount -lt 1) {
        throw "Case 'issue5131_attr_spec_trailing_comma_multiline_trace' expected Parse-AttrSpecListTail-TrailingComma in conformance trace."
    }
    if ($errRuleCount -ne 0) {
        throw "Case 'issue5131_attr_spec_trailing_comma_multiline_trace' must not emit Trailing-Comma-Err for a permitted multiline trailing comma."
    }

    Write-Host "[compiler-static] issue5131_attr_spec_trailing_comma_multiline_trace: exit=$($result.ExitCode) errors=$errorCount parse_attr_spec_tail_trailing=$tailRuleCount trailing_comma_err=$errRuleCount"
}

function Invoke-Issue5131AttrSpecTrailingCommaSingleLineTraceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue5131_attr_spec_trailing_comma_single_line_trace" `
        -Source (New-Issue5131AttrSpecTrailingCommaSingleLineSource) `
        -ConformanceFileName "issue5131_attr_spec_trailing_comma_single_line_trace.log"

    if ($result.ExitCode -ne 1) {
        throw "Case 'issue5131_attr_spec_trailing_comma_single_line_trace' expected exit 1 but got $($result.ExitCode)."
    }

    $errorCodes = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    } | ForEach-Object { $_.code })
    if ($errorCodes -notcontains "E-SRC-0521") {
        throw "Case 'issue5131_attr_spec_trailing_comma_single_line_trace' expected E-SRC-0521 in diagnostics."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $tailRuleCount = @($logLines | Where-Object {
        $_ -like "*`tParse-AttrSpecListTail-TrailingComma`t*"
    }).Count
    $errRuleCount = @($logLines | Where-Object {
        $_ -like "*`tTrailing-Comma-Err`t*"
    }).Count
    if ($tailRuleCount -ne 0) {
        throw "Case 'issue5131_attr_spec_trailing_comma_single_line_trace' must not emit Parse-AttrSpecListTail-TrailingComma for an invalid single-line trailing comma."
    }
    if ($errRuleCount -lt 1) {
        throw "Case 'issue5131_attr_spec_trailing_comma_single_line_trace' expected Trailing-Comma-Err in conformance trace."
    }

    Write-Host "[compiler-static] issue5131_attr_spec_trailing_comma_single_line_trace: exit=$($result.ExitCode) error_codes=$($errorCodes -join ',') parse_attr_spec_tail_trailing=$tailRuleCount trailing_comma_err=$errRuleCount"
}

function Invoke-Issue53ClassMethodWfTraceCase {
    $dup = Invoke-CheckWithConformance `
        -CaseId "issue53_class_dup_params_trace" `
        -Source (New-Issue33ClassDuplicateParamNamesSource) `
        -ConformanceFileName "issue53_class_dup_params_trace.log"

    if ($dup.ExitCode -ne 1) {
        throw "Case 'issue53_class_dup_params_trace' expected exit 1 but got $($dup.ExitCode)."
    }
    $dupErrorCount = @($dup.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($dupErrorCount -lt 1) {
        throw "Case 'issue53_class_dup_params_trace' expected at least one compile-time error."
    }
    $dupDiagCount = @($dup.DiagJson.diagnostics | Where-Object { $_.code -eq "E-SEM-2713" }).Count
    if ($dupDiagCount -lt 1) {
        throw "Case 'issue53_class_dup_params_trace' expected diagnostic code E-SEM-2713."
    }
    $dupLog = Get-Content -Path $dup.ConformancePath
    $dupWfCount = @($dupLog | Where-Object { $_ -like "*`tWF-Class-Method`t*" }).Count
    $dupRuleCount = @($dupLog | Where-Object { $_ -like "*`tParamBinds-Duplicate-Err`t*" }).Count
    if ($dupWfCount -lt 1 -or $dupRuleCount -lt 1) {
        throw "Case 'issue53_class_dup_params_trace' expected WF-Class-Method and ParamBinds-Duplicate-Err in conformance trace."
    }

    $self = Invoke-CheckWithConformance `
        -CaseId "issue53_class_self_param_trace" `
        -Source (New-Issue33ClassSelfParamForbiddenSource) `
        -ConformanceFileName "issue53_class_self_param_trace.log"

    if ($self.ExitCode -ne 1) {
        throw "Case 'issue53_class_self_param_trace' expected exit 1 but got $($self.ExitCode)."
    }
    $selfErrorCount = @($self.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($selfErrorCount -lt 1) {
        throw "Case 'issue53_class_self_param_trace' expected at least one compile-time error."
    }
    $selfDiagCount = @($self.DiagJson.diagnostics | Where-Object { $_.code -eq "E-SEM-3011" }).Count
    if ($selfDiagCount -lt 1) {
        throw "Case 'issue53_class_self_param_trace' expected diagnostic code E-SEM-3011."
    }
    $selfLog = Get-Content -Path $self.ConformancePath
    $selfWfCount = @($selfLog | Where-Object { $_ -like "*`tWF-Class-Method`t*" }).Count
    $selfRuleCount = @($selfLog | Where-Object { $_ -like "*`tMethod-Context-Err`t*" }).Count
    if ($selfWfCount -lt 1 -or $selfRuleCount -lt 1) {
        throw "Case 'issue53_class_self_param_trace' expected WF-Class-Method and Method-Context-Err in conformance trace."
    }

    Write-Host "[compiler-static] issue53_class_method_wf_trace: dup_wf=$dupWfCount dup_rule=$dupRuleCount self_wf=$selfWfCount self_rule=$selfRuleCount"
}

function Invoke-Issue53ClassCycleTraceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue53_class_cycle_trace" `
        -Source (New-Issue53ClassCycleSource) `
        -ConformanceFileName "issue53_class_cycle_trace.log"

    if ($result.ExitCode -ne 1) {
        throw "Case 'issue53_class_cycle_trace' expected exit 1 but got $($result.ExitCode)."
    }
    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -lt 1) {
        throw "Case 'issue53_class_cycle_trace' expected at least one compile-time error."
    }
    $diagCount = @($result.DiagJson.diagnostics | Where-Object { $_.code -eq "E-TYP-2508" }).Count
    if ($diagCount -lt 1) {
        throw "Case 'issue53_class_cycle_trace' expected diagnostic code E-TYP-2508."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $linFailCount = @($logLines | Where-Object { $_ -like "*`tLin-Fail`t*" }).Count
    $cycleCount = @($logLines | Where-Object { $_ -like "*`tSuperclass-Cycle`t*" }).Count
    if ($linFailCount -lt 1 -or $cycleCount -lt 1) {
        throw "Case 'issue53_class_cycle_trace' expected Lin-Fail and Superclass-Cycle in conformance trace."
    }

    Write-Host "[compiler-static] issue53_class_cycle_trace: exit=$($result.ExitCode) errors=$errorCount e_typ_2508=$diagCount lin_fail=$linFailCount superclass_cycle=$cycleCount"
}

function Invoke-Issue53RecordMethodSemanticsTraceCase {
    $self = Invoke-CheckWithConformance `
        -CaseId "issue53_record_self_param_forbidden" `
        -Source (New-Issue53RecordSelfParamSource) `
        -ConformanceFileName "issue53_record_self_param_forbidden.log"

    if ($self.ExitCode -ne 1) {
        throw "Case 'issue53_record_self_param_forbidden' expected exit 1 but got $($self.ExitCode)."
    }
    $selfErrorCount = @($self.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($selfErrorCount -lt 1) {
        throw "Case 'issue53_record_self_param_forbidden' expected at least one compile-time error."
    }
    $selfDiagCount = @($self.DiagJson.diagnostics | Where-Object { $_.code -eq "E-SEM-3011" }).Count
    if ($selfDiagCount -lt 1) {
        throw "Case 'issue53_record_self_param_forbidden' expected diagnostic code E-SEM-3011."
    }
    $selfLog = Get-Content -Path $self.ConformancePath
    $selfWfCount = @($selfLog | Where-Object { $_ -like "*`tWF-Record-Method`t*" }).Count
    $selfRuleCount = @($selfLog | Where-Object { $_ -like "*`tMethod-Context-Err`t*" }).Count
    if ($selfWfCount -lt 1 -or $selfRuleCount -lt 1) {
        throw "Case 'issue53_record_self_param_forbidden' expected WF-Record-Method and Method-Context-Err in conformance trace."
    }

    $dup = Invoke-CheckWithConformance `
        -CaseId "issue53_record_duplicate_params_forbidden" `
        -Source (New-Issue53RecordDupParamSource) `
        -ConformanceFileName "issue53_record_duplicate_params_forbidden.log"

    if ($dup.ExitCode -ne 1) {
        throw "Case 'issue53_record_duplicate_params_forbidden' expected exit 1 but got $($dup.ExitCode)."
    }
    $dupErrorCount = @($dup.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($dupErrorCount -lt 1) {
        throw "Case 'issue53_record_duplicate_params_forbidden' expected at least one compile-time error."
    }
    $dupDiagCount = @($dup.DiagJson.diagnostics | Where-Object { $_.code -eq "E-SEM-2713" }).Count
    if ($dupDiagCount -lt 1) {
        throw "Case 'issue53_record_duplicate_params_forbidden' expected diagnostic code E-SEM-2713."
    }
    $dupLog = Get-Content -Path $dup.ConformancePath
    $dupWfCount = @($dupLog | Where-Object { $_ -like "*`tWF-Record-Method`t*" }).Count
    $dupRuleCount = @($dupLog | Where-Object { $_ -like "*`tParamBinds-Duplicate-Err`t*" }).Count
    if ($dupWfCount -lt 1 -or $dupRuleCount -lt 1) {
        throw "Case 'issue53_record_duplicate_params_forbidden' expected WF-Record-Method and ParamBinds-Duplicate-Err in conformance trace."
    }

    $explicitReturn = Invoke-CheckWithConformance `
        -CaseId "issue53_record_nonunit_requires_explicit_return" `
        -Source (New-Issue53RecordNoExplicitReturnSource) `
        -ConformanceFileName "issue53_record_nonunit_requires_explicit_return.log"

    if ($explicitReturn.ExitCode -ne 1) {
        throw "Case 'issue53_record_nonunit_requires_explicit_return' expected exit 1 but got $($explicitReturn.ExitCode)."
    }
    $explicitErrorCount = @($explicitReturn.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($explicitErrorCount -lt 1) {
        throw "Case 'issue53_record_nonunit_requires_explicit_return' expected at least one compile-time error."
    }
    $explicitDiagCount = @($explicitReturn.DiagJson.diagnostics | Where-Object { $_.code -eq "E-TYP-1507" }).Count
    if ($explicitDiagCount -lt 1) {
        throw "Case 'issue53_record_nonunit_requires_explicit_return' expected diagnostic code E-TYP-1507."
    }
    $explicitLog = Get-Content -Path $explicitReturn.ConformancePath
    $explicitBodyRuleCount = @($explicitLog | Where-Object { $_ -like "*`tT-Record-Method-Body`t*" }).Count
    $explicitDiagRuleCount = @($explicitLog | Where-Object { $_ -like "*`tWF-ProcBody-ExplicitReturn-Err`t*" }).Count
    if ($explicitBodyRuleCount -lt 1 -or $explicitDiagRuleCount -lt 1) {
        throw "Case 'issue53_record_nonunit_requires_explicit_return' expected T-Record-Method-Body and WF-ProcBody-ExplicitReturn-Err in conformance trace."
    }

    Write-Host "[compiler-static] issue53_record_method_semantics_trace: self_wf=$selfWfCount dup_wf=$dupWfCount explicit_body=$explicitBodyRuleCount"
}

function Invoke-Issue53RecordMethodCallTraceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue53_record_method_call_trace" `
        -Source (New-Issue53RecordMethodCallTraceSource) `
        -ConformanceFileName "issue53_record_method_call_trace.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue53_record_method_call_trace' expected exit 0 but got $($result.ExitCode)."
    }
    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue53_record_method_call_trace' expected zero compile-time errors but observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $recordCallRuleCount = @($logLines | Where-Object { $_ -like "*`tT-Record-MethodCall`t*" }).Count
    if ($recordCallRuleCount -lt 1) {
        throw "Case 'issue53_record_method_call_trace' expected T-Record-MethodCall in conformance trace."
    }

    Write-Host "[compiler-static] issue53_record_method_call_trace: exit=$($result.ExitCode) errors=$errorCount t_record_method_call=$recordCallRuleCount"
}

function Invoke-Issue54ModalDefinitionsTraceCase {
    $noStates = Invoke-CheckWithConformance `
        -CaseId "issue54_modal_no_states" `
        -Source (New-Issue54ModalNoStatesSource) `
        -ConformanceFileName "issue54_modal_no_states.log"
    if ($noStates.ExitCode -ne 1) {
        throw "Case 'issue54_modal_no_states' expected exit 1 but got $($noStates.ExitCode)."
    }
    $noStatesDiag = @($noStates.DiagJson.diagnostics | Where-Object { $_.code -eq "E-TYP-2050" }).Count
    if ($noStatesDiag -lt 1) {
        throw "Case 'issue54_modal_no_states' expected diagnostic code E-TYP-2050."
    }
    $noStatesLog = Get-Content -Path $noStates.ConformancePath
    $noStatesRule = @($noStatesLog | Where-Object { $_ -like "*`tModal-NoStates-Err`t*" }).Count
    if ($noStatesRule -lt 1) {
        throw "Case 'issue54_modal_no_states' expected Modal-NoStates-Err in conformance trace."
    }

    $dupState = Invoke-CheckWithConformance `
        -CaseId "issue54_modal_dup_state" `
        -Source (New-Issue54ModalDupStateSource) `
        -ConformanceFileName "issue54_modal_dup_state.log"
    if ($dupState.ExitCode -ne 1) {
        throw "Case 'issue54_modal_dup_state' expected exit 1 but got $($dupState.ExitCode)."
    }
    $dupDiag = @($dupState.DiagJson.diagnostics | Where-Object { $_.code -eq "E-TYP-2051" }).Count
    if ($dupDiag -lt 1) {
        throw "Case 'issue54_modal_dup_state' expected diagnostic code E-TYP-2051."
    }
    $dupLog = Get-Content -Path $dupState.ConformancePath
    $dupRule = @($dupLog | Where-Object { $_ -like "*`tModal-DupState-Err`t*" }).Count
    if ($dupRule -lt 1) {
        throw "Case 'issue54_modal_dup_state' expected Modal-DupState-Err in conformance trace."
    }

    $nameCollision = Invoke-CheckWithConformance `
        -CaseId "issue54_modal_state_name_collision" `
        -Source (New-Issue54ModalStateNameCollisionSource) `
        -ConformanceFileName "issue54_modal_state_name_collision.log"
    if ($nameCollision.ExitCode -ne 1) {
        throw "Case 'issue54_modal_state_name_collision' expected exit 1 but got $($nameCollision.ExitCode)."
    }
    $collisionDiag = @($nameCollision.DiagJson.diagnostics | Where-Object { $_.code -eq "E-TYP-2054" }).Count
    if ($collisionDiag -lt 1) {
        throw "Case 'issue54_modal_state_name_collision' expected diagnostic code E-TYP-2054."
    }
    $collisionLog = Get-Content -Path $nameCollision.ConformancePath
    $collisionRule = @($collisionLog | Where-Object { $_ -like "*`tModal-StateName-Err`t*" }).Count
    if ($collisionRule -lt 1) {
        throw "Case 'issue54_modal_state_name_collision' expected Modal-StateName-Err in conformance trace."
    }

    $payloadDup = Invoke-CheckWithConformance `
        -CaseId "issue54_modal_payload_dup_field" `
        -Source (New-Issue54ModalPayloadDupFieldSource) `
        -ConformanceFileName "issue54_modal_payload_dup_field.log"
    if ($payloadDup.ExitCode -ne 1) {
        throw "Case 'issue54_modal_payload_dup_field' expected exit 1 but got $($payloadDup.ExitCode)."
    }
    $payloadDiag = @($payloadDup.DiagJson.diagnostics | Where-Object { $_.code -eq "E-TYP-2058" }).Count
    if ($payloadDiag -lt 1) {
        throw "Case 'issue54_modal_payload_dup_field' expected diagnostic code E-TYP-2058."
    }
    $payloadLog = Get-Content -Path $payloadDup.ConformancePath
    $payloadRule = @($payloadLog | Where-Object { $_ -like "*`tModal-Payload-DupField`t*" }).Count
    if ($payloadRule -lt 1) {
        throw "Case 'issue54_modal_payload_dup_field' expected Modal-Payload-DupField in conformance trace."
    }

    Write-Host "[compiler-static] issue54_modal_definitions_trace: no_states=$noStatesRule dup_state=$dupRule state_name=$collisionRule payload_dup=$payloadRule"
}

function Invoke-Issue54ModalStateIntroTraceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue54_modal_state_intro_trace" `
        -Source (New-Issue54ModalStateIntroTraceSource) `
        -ConformanceFileName "issue54_modal_state_intro_trace.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue54_modal_state_intro_trace' expected exit 0 but got $($result.ExitCode)."
    }
    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue54_modal_state_intro_trace' expected zero compile-time errors but observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $wfModalState = @($logLines | Where-Object { $_ -like "*`tWF-ModalState`t*" }).Count
    $stateSpecificWf = @($logLines | Where-Object { $_ -like "*`tState-Specific-WF`t*" }).Count
    $stateIntro = @($logLines | Where-Object { $_ -like "*`tT-Modal-State-Intro`t*" }).Count
    if ($wfModalState -lt 1 -or $stateSpecificWf -lt 1 -or $stateIntro -lt 1) {
        throw "Case 'issue54_modal_state_intro_trace' expected WF-ModalState, State-Specific-WF, and T-Modal-State-Intro in conformance trace."
    }

    Write-Host "[compiler-static] issue54_modal_state_intro_trace: exit=$($result.ExitCode) wf_modal_state=$wfModalState state_specific_wf=$stateSpecificWf state_intro=$stateIntro"
}

function Invoke-Issue54ModalClassParseTraceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue54_modal_class_parse_trace" `
        -Source (New-Issue54ModalClassParseTraceSource) `
        -ConformanceFileName "issue54_modal_class_parse_trace.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue54_modal_class_parse_trace' expected exit 0 but got $($result.ExitCode)."
    }
    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue54_modal_class_parse_trace' expected zero compile-time errors but observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $modalYesCount = @($logLines | Where-Object { $_ -like "*`tParse-ModalOpt-Yes`t*" }).Count
    $modalNoCount = @($logLines | Where-Object { $_ -like "*`tParse-ModalOpt-No`t*" }).Count
    $modalClassCount = @($logLines | Where-Object { $_ -like "*`tParse-Modal-Class`t*" }).Count
    $classCount = @($logLines | Where-Object { $_ -like "*`tParse-Class`t*" }).Count
    if ($modalYesCount -lt 1 -or $modalNoCount -lt 1 -or
        $modalClassCount -lt 1 -or $classCount -lt 1) {
        throw "Case 'issue54_modal_class_parse_trace' expected Parse-ModalOpt-Yes, Parse-ModalOpt-No, Parse-Modal-Class, and Parse-Class in conformance trace."
    }

    Write-Host "[compiler-static] issue54_modal_class_parse_trace: exit=$($result.ExitCode) parse_modalopt_yes=$modalYesCount parse_modalopt_no=$modalNoCount parse_modal_class=$modalClassCount parse_class=$classCount"
}

function Invoke-Issue54RegionSurfaceConformanceCase {
    Invoke-ExpectedSuccessCase `
        -Id "issue54_region_canonical_surface" `
        -Source (New-Issue54RegionCanonicalSurfaceSource)

    $mark = Invoke-CheckWithConformance `
        -CaseId "issue54_region_mark_not_surface" `
        -Source (New-Issue54RegionMarkNotSurfaceSource) `
        -ConformanceFileName "issue54_region_mark_not_surface.log"
    if ($mark.ExitCode -ne 1) {
        throw "Case 'issue54_region_mark_not_surface' expected exit 1 but got $($mark.ExitCode)."
    }
    $markDiag = @($mark.DiagJson.diagnostics | Where-Object { $_.code -eq "E-TYP-2053" }).Count
    if ($markDiag -lt 1) {
        throw "Case 'issue54_region_mark_not_surface' expected diagnostic code E-TYP-2053."
    }
    $markTrace = Get-Content -Path $mark.ConformancePath
    $markRule = @($markTrace | Where-Object { $_ -like "*`tModal-Method-NotFound`t*" }).Count
    if ($markRule -lt 1) {
        throw "Case 'issue54_region_mark_not_surface' expected Modal-Method-NotFound in conformance trace."
    }

    $resetTo = Invoke-CheckWithConformance `
        -CaseId "issue54_region_reset_to_not_surface" `
        -Source (New-Issue54RegionResetToNotSurfaceSource) `
        -ConformanceFileName "issue54_region_reset_to_not_surface.log"
    if ($resetTo.ExitCode -ne 1) {
        throw "Case 'issue54_region_reset_to_not_surface' expected exit 1 but got $($resetTo.ExitCode)."
    }
    $resetToDiag = @($resetTo.DiagJson.diagnostics | Where-Object { $_.code -eq "E-TYP-2053" }).Count
    if ($resetToDiag -lt 1) {
        throw "Case 'issue54_region_reset_to_not_surface' expected diagnostic code E-TYP-2053."
    }
    $resetToTrace = Get-Content -Path $resetTo.ConformancePath
    $resetToRule = @($resetToTrace | Where-Object { $_ -like "*`tModal-Method-NotFound`t*" }).Count
    if ($resetToRule -lt 1) {
        throw "Case 'issue54_region_reset_to_not_surface' expected Modal-Method-NotFound in conformance trace."
    }

    Write-Host "[compiler-static] issue54_region_surface_conformance: mark_diag=$markDiag reset_to_diag=$resetToDiag modal_method_not_found=$($markRule + $resetToRule)"
}

function Invoke-Issue54RegionUnsafeGateTraceCase {
    $resetUnsafe = Invoke-CheckWithConformance `
        -CaseId "issue54_region_reset_unchecked_unsafe_err" `
        -Source (New-Issue54RegionResetUncheckedUnsafeErrSource) `
        -ConformanceFileName "issue54_region_reset_unchecked_unsafe_err.log"
    if ($resetUnsafe.ExitCode -ne 1) {
        throw "Case 'issue54_region_reset_unchecked_unsafe_err' expected exit 1 but got $($resetUnsafe.ExitCode)."
    }
    $resetDiag = @($resetUnsafe.DiagJson.diagnostics | Where-Object { $_.code -eq "E-MEM-3030" }).Count
    $resetUnknown = @($resetUnsafe.DiagJson.diagnostics | Where-Object { $_.code -eq "E-UNS-0101" }).Count
    if ($resetDiag -lt 1 -or $resetUnknown -ne 0) {
        throw "Case 'issue54_region_reset_unchecked_unsafe_err' expected E-MEM-3030 and no E-UNS-0101."
    }
    $resetLog = Get-Content -Path $resetUnsafe.ConformancePath
    $resetRule = @($resetLog | Where-Object { $_ -like "*`tRegion-Unchecked-Unsafe-Err`t*" }).Count
    if ($resetRule -lt 1) {
        throw "Case 'issue54_region_reset_unchecked_unsafe_err' expected Region-Unchecked-Unsafe-Err in conformance trace."
    }

    $freeUnsafe = Invoke-CheckWithConformance `
        -CaseId "issue54_region_free_unchecked_unsafe_err" `
        -Source (New-Issue54RegionFreeUncheckedUnsafeErrSource) `
        -ConformanceFileName "issue54_region_free_unchecked_unsafe_err.log"
    if ($freeUnsafe.ExitCode -ne 1) {
        throw "Case 'issue54_region_free_unchecked_unsafe_err' expected exit 1 but got $($freeUnsafe.ExitCode)."
    }
    $freeDiag = @($freeUnsafe.DiagJson.diagnostics | Where-Object { $_.code -eq "E-MEM-3030" }).Count
    $freeUnknown = @($freeUnsafe.DiagJson.diagnostics | Where-Object { $_.code -eq "E-UNS-0101" }).Count
    if ($freeDiag -lt 1 -or $freeUnknown -ne 0) {
        throw "Case 'issue54_region_free_unchecked_unsafe_err' expected E-MEM-3030 and no E-UNS-0101."
    }
    $freeLog = Get-Content -Path $freeUnsafe.ConformancePath
    $freeRule = @($freeLog | Where-Object { $_ -like "*`tRegion-Unchecked-Unsafe-Err`t*" }).Count
    if ($freeRule -lt 1) {
        throw "Case 'issue54_region_free_unchecked_unsafe_err' expected Region-Unchecked-Unsafe-Err in conformance trace."
    }

    Invoke-ExpectedSuccessCase `
        -Id "issue54_region_unchecked_inside_unsafe_allowed" `
        -Source (New-Issue54RegionUncheckedUnsafeAllowedSource)

    Write-Host "[compiler-static] issue54_region_unsafe_gate_trace: reset_rule=$resetRule free_rule=$freeRule e_mem_3030=$($resetDiag + $freeDiag)"
}

function Invoke-Issue54RegionAllocLoweringTraceCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue54_region_alloc_lowering_trace" `
        -Source (New-Issue54RegionAllocLoweringBuildSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue54_region_alloc_lowering_trace.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue54_region_alloc_lowering_trace' expected exit 0 but got $($result.ExitCode)."
    }
    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue54_region_alloc_lowering_trace' expected zero compile-time errors but observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $lowerAllocCount = @($logLines | Where-Object { $_ -like "*`tLower-MethodCall-Region-Alloc`t*" }).Count
    if ($lowerAllocCount -lt 1) {
        throw "Case 'issue54_region_alloc_lowering_trace' expected Lower-MethodCall-Region-Alloc in conformance trace."
    }

    Write-Host "[compiler-static] issue54_region_alloc_lowering_trace: exit=$($result.ExitCode) lower_region_alloc=$lowerAllocCount"
}

function Invoke-Issue54RegionResetLoweringTraceCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue54_region_reset_lowering_trace" `
        -Source (New-Issue54RegionResetLoweringBuildSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue54_region_reset_lowering_trace.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue54_region_reset_lowering_trace' expected exit 0 but got $($result.ExitCode)."
    }
    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue54_region_reset_lowering_trace' expected zero compile-time errors but observed $errorCount."
    }

    $irPath = Join-Path $result.CaseRoot "build\\probe\\ir\\probe.ll"
    if (-not (Test-Path $irPath)) {
        throw "Case 'issue54_region_reset_lowering_trace' missing LLVM IR file: $irPath"
    }

    $irText = Get-Content -Path $irPath -Raw
    $resetBuiltinCount = ([regex]::Matches(
        $irText,
        "cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3areset_x5funchecked")).Count
    $freezeBuiltinCount = ([regex]::Matches(
        $irText,
        "cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3afreeze")).Count
    if ($resetBuiltinCount -lt 1 -or $freezeBuiltinCount -lt 1) {
        throw "Case 'issue54_region_reset_lowering_trace' expected region reset/freeze runtime builtin calls in LLVM IR."
    }

    Write-Host "[compiler-static] issue54_region_reset_lowering_trace: exit=$($result.ExitCode) reset_builtin=$resetBuiltinCount freeze_builtin=$freezeBuiltinCount"
}

function Invoke-Issue54RegionFreezeThawFreeLoweringTraceCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue54_region_freeze_thaw_free_lowering_trace" `
        -Source (New-Issue54RegionFreezeThawFreeLoweringBuildSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue54_region_freeze_thaw_free_lowering_trace.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue54_region_freeze_thaw_free_lowering_trace' expected exit 0 but got $($result.ExitCode)."
    }
    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue54_region_freeze_thaw_free_lowering_trace' expected zero compile-time errors but observed $errorCount."
    }

    $irPath = Join-Path $result.CaseRoot "build\\probe\\ir\\probe.ll"
    if (-not (Test-Path $irPath)) {
        throw "Case 'issue54_region_freeze_thaw_free_lowering_trace' missing LLVM IR file: $irPath"
    }

    $irText = Get-Content -Path $irPath -Raw
    $freezeBuiltinCount = ([regex]::Matches(
        $irText,
        "cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3afreeze")).Count
    $thawBuiltinCount = ([regex]::Matches(
        $irText,
        "cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3athaw")).Count
    $freeBuiltinCount = ([regex]::Matches(
        $irText,
        "cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3afree_x5funchecked")).Count
    if ($freezeBuiltinCount -lt 1 -or $thawBuiltinCount -lt 1 -or $freeBuiltinCount -lt 1) {
        throw "Case 'issue54_region_freeze_thaw_free_lowering_trace' expected region freeze/thaw/free runtime builtin calls in LLVM IR."
    }

    Write-Host "[compiler-static] issue54_region_freeze_thaw_free_lowering_trace: exit=$($result.ExitCode) freeze_builtin=$freezeBuiltinCount thaw_builtin=$thawBuiltinCount free_builtin=$freeBuiltinCount"
}

function Invoke-Issue54ResolveDuplicateBindingDiagnosticCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue54_resolve_duplicate_binding_diag" `
        -Source (New-Issue54ResolveDuplicateBindingDiagnosticSource) `
        -ConformanceFileName "issue54_resolve_duplicate_binding_diag.log"

    if ($result.ExitCode -ne 1) {
        throw "Case 'issue54_resolve_duplicate_binding_diag' expected exit 1 but got $($result.ExitCode)."
    }
    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -lt 1) {
        throw "Case 'issue54_resolve_duplicate_binding_diag' expected at least one compile-time error."
    }
    $dupCodeCount = @($result.DiagJson.diagnostics | Where-Object { $_.code -eq "E-MOD-1302" }).Count
    if ($dupCodeCount -lt 1) {
        throw "Case 'issue54_resolve_duplicate_binding_diag' expected diagnostic code E-MOD-1302."
    }
    $logLines = Get-Content -Path $result.ConformancePath
    $introDupCount = @($logLines | Where-Object { $_ -like "*`tIntro-Dup`t*" }).Count
    if ($introDupCount -lt 1) {
        throw "Case 'issue54_resolve_duplicate_binding_diag' expected Intro-Dup in conformance trace."
    }

    Write-Host "[compiler-static] issue54_resolve_duplicate_binding_diag: exit=$($result.ExitCode) errors=$errorCount e_mod_1302=$dupCodeCount intro_dup=$introDupCount"
}

function Invoke-Issue55StateSpecificFieldConformanceCase {
    $typing = Invoke-CheckWithConformance `
        -CaseId "issue55_modal_field_typing_trace" `
        -Source (New-Issue55ModalFieldTypingTraceSource) `
        -ConformanceFileName "issue55_modal_field_typing_trace.log"

    if ($typing.ExitCode -ne 0) {
        throw "Case 'issue55_modal_field_typing_trace' expected exit 0 but got $($typing.ExitCode)."
    }
    $typingErrors = @($typing.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($typingErrors -ne 0) {
        throw "Case 'issue55_modal_field_typing_trace' expected zero compile-time errors but observed $typingErrors."
    }
    $typingLog = Get-Content -Path $typing.ConformancePath
    $modalFieldCount = @($typingLog | Where-Object { $_ -like "*`tT-Modal-Field`t*" }).Count
    $modalFieldPermCount = @($typingLog | Where-Object { $_ -like "*`tT-Modal-Field-Perm`t*" }).Count
    $legacyFieldAccessCount = @($typingLog | Where-Object { $_ -like "*`tT-FieldAccess`t*" }).Count
    if ($modalFieldCount -lt 1 -or $modalFieldPermCount -lt 1 -or $legacyFieldAccessCount -ne 0) {
        throw "Case 'issue55_modal_field_typing_trace' expected canonical modal field traces (T-Modal-Field/T-Modal-Field-Perm) and no legacy T-FieldAccess (modal=$modalFieldCount perm=$modalFieldPermCount legacy=$legacyFieldAccessCount)."
    }

    $missing = Invoke-CheckWithConformance `
        -CaseId "issue55_modal_field_missing_diag" `
        -Source (New-Issue55ModalFieldMissingDiagSource) `
        -ConformanceFileName "issue55_modal_field_missing_diag.log"
    if ($missing.ExitCode -ne 1) {
        throw "Case 'issue55_modal_field_missing_diag' expected exit 1 but got $($missing.ExitCode)."
    }
    $missingCodeCount = @($missing.DiagJson.diagnostics | Where-Object { $_.code -eq "E-TYP-2052" }).Count
    if ($missingCodeCount -lt 1) {
        throw "Case 'issue55_modal_field_missing_diag' expected diagnostic code E-TYP-2052."
    }
    $missingLog = Get-Content -Path $missing.ConformancePath
    $missingRuleCount = @($missingLog | Where-Object { $_ -like "*`tModal-Field-Missing`t*" }).Count
    if ($missingRuleCount -lt 1) {
        throw "Case 'issue55_modal_field_missing_diag' expected Modal-Field-Missing in conformance trace."
    }

    $general = Invoke-CheckWithConformance `
        -CaseId "issue55_modal_field_general_diag" `
        -Source (New-Issue55ModalFieldGeneralDiagSource) `
        -ConformanceFileName "issue55_modal_field_general_diag.log"
    if ($general.ExitCode -ne 1) {
        throw "Case 'issue55_modal_field_general_diag' expected exit 1 but got $($general.ExitCode)."
    }
    $generalCodeCount = @($general.DiagJson.diagnostics | Where-Object { $_.code -eq "E-TYP-2057" }).Count
    if ($generalCodeCount -lt 1) {
        throw "Case 'issue55_modal_field_general_diag' expected diagnostic code E-TYP-2057."
    }
    $generalLog = Get-Content -Path $general.ConformancePath
    $generalRuleCount = @($generalLog | Where-Object { $_ -like "*`tModal-Field-General-Err`t*" }).Count
    if ($generalRuleCount -lt 1) {
        throw "Case 'issue55_modal_field_general_diag' expected Modal-Field-General-Err in conformance trace."
    }

    $crossModuleFiles = @{
        "mod/Other.cursive" = (New-Issue55ModalFieldCrossModuleModSource)
    }
    $visibility = Invoke-CheckWithConformance `
        -CaseId "issue55_modal_field_not_visible_cross_module" `
        -Source (New-Issue55ModalFieldCrossModuleMainSource) `
        -ConformanceFileName "issue55_modal_field_not_visible_cross_module.log" `
        -ExtraFiles $crossModuleFiles
    if ($visibility.ExitCode -ne 1) {
        throw "Case 'issue55_modal_field_not_visible_cross_module' expected exit 1 but got $($visibility.ExitCode)."
    }
    $visibilityCodeCount = @($visibility.DiagJson.diagnostics | Where-Object { $_.code -eq "E-TYP-2064" }).Count
    if ($visibilityCodeCount -lt 1) {
        throw "Case 'issue55_modal_field_not_visible_cross_module' expected diagnostic code E-TYP-2064."
    }
    $visibilityLog = Get-Content -Path $visibility.ConformancePath
    $visibilityRuleCount = @($visibilityLog | Where-Object { $_ -like "*`tModal-Field-NotVisible`t*" }).Count
    if ($visibilityRuleCount -lt 1) {
        throw "Case 'issue55_modal_field_not_visible_cross_module' expected Modal-Field-NotVisible in conformance trace."
    }

    Write-Host "[compiler-static] issue55_state_specific_field_conformance: t_modal_field=$modalFieldCount t_modal_field_perm=$modalFieldPermCount missing=$missingRuleCount general=$generalRuleCount not_visible=$visibilityRuleCount"
}

function Invoke-Issue56TransitionsAndMethodsConformanceCase {
    $trace = Invoke-CheckWithConformance `
        -CaseId "issue56_modal_call_trace" `
        -Source (New-Issue56ModalCallTraceSource) `
        -ConformanceFileName "issue56_modal_call_trace.log"
    if ($trace.ExitCode -ne 0) {
        throw "Case 'issue56_modal_call_trace' expected exit 0 but got $($trace.ExitCode)."
    }
    $traceErrors = @($trace.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($traceErrors -ne 0) {
        throw "Case 'issue56_modal_call_trace' expected zero compile-time errors but observed $traceErrors."
    }
    $traceLog = Get-Content -Path $trace.ConformancePath
    $tModalTransitionCount = @($traceLog | Where-Object { $_ -like "*`tT-Modal-Transition`t*" }).Count
    $tModalMethodCount = @($traceLog | Where-Object { $_ -like "*`tT-Modal-Method`t*" }).Count
    $legacyMethodCallCount = @($traceLog | Where-Object { $_ -like "*`tT-MethodCall`t*" }).Count
    $wfStateMethodCount = @($traceLog | Where-Object { $_ -like "*`tWF-State-Method`t*" }).Count
    $wfTransitionCount = @($traceLog | Where-Object { $_ -like "*`tWF-Transition`t*" }).Count
    $methodBodyCount = @($traceLog | Where-Object { $_ -like "*`tT-Modal-Method-Body`t*" }).Count
    $transitionBodyCount = @($traceLog | Where-Object { $_ -like "*`tT-Modal-Transition-Body`t*" }).Count
    if ($tModalTransitionCount -lt 1 -or $tModalMethodCount -lt 1 -or $legacyMethodCallCount -ne 0) {
        throw "Case 'issue56_modal_call_trace' expected canonical modal call traces and no generic T-MethodCall (transition=$tModalTransitionCount method=$tModalMethodCount legacy=$legacyMethodCallCount)."
    }
    if ($wfStateMethodCount -lt 1 -or $wfTransitionCount -lt 1 -or $methodBodyCount -lt 1 -or $transitionBodyCount -lt 1) {
        throw "Case 'issue56_modal_call_trace' expected WF-State-Method, WF-Transition, T-Modal-Method-Body, and T-Modal-Transition-Body traces (wf_method=$wfStateMethodCount wf_transition=$wfTransitionCount method_body=$methodBodyCount transition_body=$transitionBodyCount)."
    }

    $target = Invoke-CheckWithConformance `
        -CaseId "issue56_transition_target_diag" `
        -Source (New-Issue56TransitionTargetDiagSource) `
        -ConformanceFileName "issue56_transition_target_diag.log"
    if ($target.ExitCode -ne 1) {
        throw "Case 'issue56_transition_target_diag' expected exit 1 but got $($target.ExitCode)."
    }
    $targetDiagCount = @($target.DiagJson.diagnostics | Where-Object { $_.code -eq "E-TYP-2059" }).Count
    if ($targetDiagCount -lt 1) {
        throw "Case 'issue56_transition_target_diag' expected diagnostic code E-TYP-2059."
    }
    $targetLog = Get-Content -Path $target.ConformancePath
    $targetRuleCount = @($targetLog | Where-Object { $_ -like "*`tTransition-Target-Err`t*" }).Count
    $legacyTargetRuleCount = @($targetLog | Where-Object { $_ -like "*`tWF-Transition-TargetNotFound`t*" }).Count
    if ($targetRuleCount -lt 1 -or $legacyTargetRuleCount -ne 0) {
        throw "Case 'issue56_transition_target_diag' expected canonical Transition-Target-Err and no WF-Transition-TargetNotFound (canonical=$targetRuleCount legacy=$legacyTargetRuleCount)."
    }

    $sourcePerm = Invoke-CheckWithConformance `
        -CaseId "issue56_transition_source_perm_diag" `
        -Source (New-Issue56TransitionSourcePermDiagSource) `
        -ConformanceFileName "issue56_transition_source_perm_diag.log"
    if ($sourcePerm.ExitCode -ne 1) {
        throw "Case 'issue56_transition_source_perm_diag' expected exit 1 but got $($sourcePerm.ExitCode)."
    }
    $sourcePermDiagCount = @($sourcePerm.DiagJson.diagnostics | Where-Object { $_.code -eq "E-TYP-2056" }).Count
    if ($sourcePermDiagCount -lt 1) {
        throw "Case 'issue56_transition_source_perm_diag' expected diagnostic code E-TYP-2056."
    }
    $sourcePermLog = Get-Content -Path $sourcePerm.ConformancePath
    $sourceRuleCount = @($sourcePermLog | Where-Object { $_ -like "*`tTransition-Source-Err`t*" }).Count
    $legacyRecvPermRuleCount = @($sourcePermLog | Where-Object { $_ -like "*`tMethodCall-RecvPerm-Err`t*" }).Count
    if ($sourceRuleCount -lt 1 -or $legacyRecvPermRuleCount -ne 0) {
        throw "Case 'issue56_transition_source_perm_diag' expected Transition-Source-Err and no MethodCall-RecvPerm-Err (canonical=$sourceRuleCount legacy=$legacyRecvPermRuleCount)."
    }

    Write-Host "[compiler-static] issue56_transitions_methods_conformance: t_modal_transition=$tModalTransitionCount t_modal_method=$tModalMethodCount wf_state_method=$wfStateMethodCount wf_transition=$wfTransitionCount target_rule=$targetRuleCount source_rule=$sourceRuleCount"
}

function Invoke-Issue56VisibilityConformanceCase {
    $sameModule = Invoke-CheckWithConformance `
        -CaseId "issue56_visibility_same_module" `
        -Source (New-Issue56VisibilitySameModuleSource) `
        -ConformanceFileName "issue56_visibility_same_module.log"
    if ($sameModule.ExitCode -ne 0) {
        throw "Case 'issue56_visibility_same_module' expected exit 0 but got $($sameModule.ExitCode)."
    }
    $sameModuleErrors = @($sameModule.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($sameModuleErrors -ne 0) {
        throw "Case 'issue56_visibility_same_module' expected zero compile-time errors but observed $sameModuleErrors."
    }
    $sameLog = Get-Content -Path $sameModule.ConformancePath
    $sameMethodCount = @($sameLog | Where-Object { $_ -like "*`tT-Modal-Method`t*" }).Count
    $sameTransitionCount = @($sameLog | Where-Object { $_ -like "*`tT-Modal-Transition`t*" }).Count
    if ($sameMethodCount -lt 1 -or $sameTransitionCount -lt 1) {
        throw "Case 'issue56_visibility_same_module' expected modal method/transition traces (method=$sameMethodCount transition=$sameTransitionCount)."
    }

    $crossModuleFiles = @{
        "mod/Other.cursive" = (New-Issue56VisibilityCrossModuleModSource)
    }
    $privateCross = Invoke-CheckWithConformance `
        -CaseId "issue56_visibility_private_cross_module" `
        -Source (New-Issue56VisibilityCrossModuleMainSource) `
        -ConformanceFileName "issue56_visibility_private_cross_module.log" `
        -ExtraFiles $crossModuleFiles
    if ($privateCross.ExitCode -ne 1) {
        throw "Case 'issue56_visibility_private_cross_module' expected exit 1 but got $($privateCross.ExitCode)."
    }
    $privateCrossDiagCount = @($privateCross.DiagJson.diagnostics | Where-Object { $_.code -eq "E-TYP-2064" }).Count
    if ($privateCrossDiagCount -lt 1) {
        throw "Case 'issue56_visibility_private_cross_module' expected diagnostic code E-TYP-2064."
    }
    $privateCrossLog = Get-Content -Path $privateCross.ConformancePath
    $privateCrossRuleCount = @($privateCrossLog | Where-Object { $_ -like "*`tModal-Method-NotVisible`t*" }).Count
    if ($privateCrossRuleCount -lt 1) {
        throw "Case 'issue56_visibility_private_cross_module' expected Modal-Method-NotVisible in conformance trace."
    }

    $internalCross = Invoke-CheckWithConformance `
        -CaseId "issue56_visibility_internal_cross_module" `
        -Source (New-Issue56VisibilityCrossModuleInternalMainSource) `
        -ConformanceFileName "issue56_visibility_internal_cross_module.log" `
        -ExtraFiles $crossModuleFiles
    if ($internalCross.ExitCode -ne 0) {
        throw "Case 'issue56_visibility_internal_cross_module' expected exit 0 but got $($internalCross.ExitCode)."
    }
    $internalCrossErrors = @($internalCross.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($internalCrossErrors -ne 0) {
        throw "Case 'issue56_visibility_internal_cross_module' expected zero compile-time errors but observed $internalCrossErrors."
    }
    $internalCrossLog = Get-Content -Path $internalCross.ConformancePath
    $internalCrossMethodCount = @($internalCrossLog | Where-Object { $_ -like "*`tT-Modal-Method`t*" }).Count
    if ($internalCrossMethodCount -lt 1) {
        throw "Case 'issue56_visibility_internal_cross_module' expected T-Modal-Method in conformance trace."
    }

    Write-Host "[compiler-static] issue56_visibility_conformance: same_module_method=$sameMethodCount same_module_transition=$sameTransitionCount private_cross_not_visible=$privateCrossRuleCount internal_cross_method=$internalCrossMethodCount"
}

function Invoke-Issue617ProtectedVisibilityRejectedCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue617_protected_visibility_rejected" `
        -Source (New-Issue617ProtectedVisibilityRejectedSource) `
        -ConformanceFileName "issue617_protected_visibility_rejected.log"
    if ($result.ExitCode -ne 1) {
        throw "Case 'issue617_protected_visibility_rejected' expected exit 1 but got $($result.ExitCode)."
    }
    $diagCount = @($result.DiagJson.diagnostics | Where-Object { $_.code -eq "E-SRC-0520" }).Count
    if ($diagCount -lt 1) {
        throw "Case 'issue617_protected_visibility_rejected' expected diagnostic code E-SRC-0520."
    }

    $traceLog = Get-Content -Path $result.ConformancePath
    $parseVisOptCount = @($traceLog | Where-Object { $_ -like "*`tParse-Vis-Opt`t*" }).Count
    $parseVisDefaultCount = @($traceLog | Where-Object { $_ -like "*`tParse-Vis-Default`t*" }).Count
    $parseItemErrCount = @($traceLog | Where-Object { $_ -like "*`tParse-Item-Err`t*" }).Count
    if ($parseVisOptCount -ne 0 -or $parseVisDefaultCount -lt 1 -or $parseItemErrCount -lt 1) {
        throw "Case 'issue617_protected_visibility_rejected' expected Parse-Vis-Default and Parse-Item-Err without Parse-Vis-Opt (opt=$parseVisOptCount default=$parseVisDefaultCount item_err=$parseItemErrCount)."
    }

    Write-Host "[compiler-static] issue617_protected_visibility_rejected: e_src_0520=$diagCount parse_vis_opt=$parseVisOptCount parse_vis_default=$parseVisDefaultCount parse_item_err=$parseItemErrCount"
}

function Invoke-Issue57LexicalIdentifierSecurityConformanceCase {
    $confusable = Invoke-CheckWithConformance `
        -CaseId "issue57_confusable_identifier_rejected" `
        -Source (New-Issue57ConfusableIdentifierSource) `
        -ConformanceFileName "issue57_confusable_identifier_rejected.log"
    if ($confusable.ExitCode -ne 1) {
        throw "Case 'issue57_confusable_identifier_rejected' expected exit 1 but got $($confusable.ExitCode)."
    }
    $confusableDiagCount = @($confusable.DiagJson.diagnostics | Where-Object { $_.code -eq "E-SRC-0310" }).Count
    if ($confusableDiagCount -lt 1) {
        throw "Case 'issue57_confusable_identifier_rejected' expected diagnostic code E-SRC-0310."
    }
    $confusableLog = Get-Content -Path $confusable.ConformancePath
    $confusableRuleCount = @($confusableLog | Where-Object { $_ -like "*`tConfusable-Err`t*" }).Count
    if ($confusableRuleCount -lt 1) {
        throw "Case 'issue57_confusable_identifier_rejected' expected Confusable-Err in conformance trace."
    }

    $mixed = Invoke-CheckWithConformance `
        -CaseId "issue57_mixed_script_identifier_rejected" `
        -Source (New-Issue57MixedScriptIdentifierSource) `
        -ConformanceFileName "issue57_mixed_script_identifier_rejected.log"
    if ($mixed.ExitCode -ne 1) {
        throw "Case 'issue57_mixed_script_identifier_rejected' expected exit 1 but got $($mixed.ExitCode)."
    }
    $mixedDiagCount = @($mixed.DiagJson.diagnostics | Where-Object { $_.code -eq "E-SRC-0311" }).Count
    if ($mixedDiagCount -lt 1) {
        throw "Case 'issue57_mixed_script_identifier_rejected' expected diagnostic code E-SRC-0311."
    }
    $mixedLog = Get-Content -Path $mixed.ConformancePath
    $mixedRuleCount = @($mixedLog | Where-Object { $_ -like "*`tMixedScript-Err`t*" }).Count
    if ($mixedRuleCount -lt 1) {
        throw "Case 'issue57_mixed_script_identifier_rejected' expected MixedScript-Err in conformance trace."
    }

    Invoke-ExpectedSuccessCase `
        -Id "issue57_single_script_unicode_identifier_allowed" `
        -Source (New-Issue57SingleScriptUnicodeIdentifierSource)

    Write-Host "[compiler-static] issue57_lexical_identifier_security_conformance: confusable=$confusableRuleCount mixed=$mixedRuleCount single_script_ok=1"
}

function Invoke-Issue621ReservedKeywordIdentifierConformanceCase {
    Invoke-ExpectedDiagCodeCaseWithForbiddenCodes `
        -Id "issue621_reserved_keyword_identifier_rejected" `
        -Source (New-Issue621ReservedKeywordIdentifierSource) `
        -ExpectedCodes @("E-CNF-0401") `
        -ForbiddenCodes @("E-SRC-0520")
}

function Invoke-Issue58StringBytesConformanceCase {
    $failures = New-Object System.Collections.Generic.List[string]

    try {
        Invoke-ExpectedSuccessCase `
            -Id "issue58_bytes_spec_surface_ok" `
            -Source (New-Issue58BytesSpecSurfaceSource)
    } catch {
        $failures.Add($_.Exception.Message) | Out-Null
    }

    try {
        Invoke-ExpectedSuccessCase `
            -Id "issue58_bytes_as_slice_allowed" `
            -Source (New-Issue58BytesAsSliceSource)
    } catch {
        $failures.Add($_.Exception.Message) | Out-Null
    }

    $analysisPath = Join-Path $workspaceRoot "cursive\\src\\04_analysis\\memory\\string_bytes.cpp"
    if (-not (Test-Path $analysisPath)) {
        $failures.Add("Case 'issue58_string_bytes_surface_conformance' missing analysis file: $analysisPath") | Out-Null
    } else {
        $analysisText = Get-Content -Path $analysisPath -Raw
        if ($analysisText -notmatch 'IdEq\(name,\s*"as_slice"\)') {
            $failures.Add("Case 'issue58_string_bytes_surface_conformance' missing bytes::as_slice from 5.8 analysis builtin typing surface.") | Out-Null
        }
        if ($analysisText -notmatch 'SPEC_RULE\("BytesAsSlice-Ok"\)') {
            $failures.Add("Case 'issue58_string_bytes_surface_conformance' missing BytesAsSlice-Ok rule anchor in analysis surface.") | Out-Null
        }
    }

    $builtinsPath = Join-Path $workspaceRoot "cursive\\src\\05_codegen\\intrinsics\\builtins.cpp"
    if (-not (Test-Path $builtinsPath)) {
        $failures.Add("Case 'issue58_string_bytes_surface_conformance' missing codegen builtins file: $builtinsPath") | Out-Null
    } else {
        $builtinsText = Get-Content -Path $builtinsPath -Raw
        $regexOptions = [System.Text.RegularExpressions.RegexOptions]::Singleline

        $stringTable = [System.Text.RegularExpressions.Regex]::Match(
            $builtinsText,
            'kStringBuiltins\s*=\s*\{\{(?<body>.*?)\}\};',
            $regexOptions)
        if (-not $stringTable.Success) {
            $failures.Add("Case 'issue58_string_bytes_surface_conformance' could not locate kStringBuiltins dispatch table in builtins.cpp.") | Out-Null
        } elseif ($stringTable.Groups["body"].Value -match '"string::drop_managed"') {
            $failures.Add("Case 'issue58_string_bytes_surface_conformance' found drop hook in BuiltinSym kStringBuiltins table; drop hooks must be separate from StringBuiltins per 6.12.15/6.12.16.") | Out-Null
        }

        $bytesTable = [System.Text.RegularExpressions.Regex]::Match(
            $builtinsText,
            'kBytesBuiltins\s*=\s*\{\{(?<body>.*?)\}\};',
            $regexOptions)
        if (-not $bytesTable.Success) {
            $failures.Add("Case 'issue58_string_bytes_surface_conformance' could not locate kBytesBuiltins dispatch table in builtins.cpp.") | Out-Null
        } else {
            $bytesBody = $bytesTable.Groups["body"].Value
            if ($bytesBody -notmatch '"bytes::as_slice"') {
                $failures.Add("Case 'issue58_string_bytes_surface_conformance' missing bytes::as_slice in BuiltinSym kBytesBuiltins table.") | Out-Null
            }
            if ($bytesBody -match '"bytes::drop_managed"') {
                $failures.Add("Case 'issue58_string_bytes_surface_conformance' found drop hook in BuiltinSym kBytesBuiltins table; drop hooks must be separate from BytesBuiltins per 6.12.15/6.12.16.") | Out-Null
            }
        }

        if ($builtinsText -notmatch 'SPEC_RULE\("StringDropSym-Decl"\)') {
            $failures.Add("Case 'issue58_string_bytes_surface_conformance' missing StringDropSym-Decl anchor in builtins.cpp.") | Out-Null
        }
        if ($builtinsText -notmatch 'SPEC_RULE\("BytesDropSym-Decl"\)') {
            $failures.Add("Case 'issue58_string_bytes_surface_conformance' missing BytesDropSym-Decl anchor in builtins.cpp.") | Out-Null
        }
    }

    if ($failures.Count -gt 0) {
        foreach ($failure in $failures) {
            Write-Host "[compiler-static] issue58 failure: $failure"
        }
        throw "Case 'issue58_string_bytes_surface_conformance' found $($failures.Count) conformance violation(s)."
    }

    Write-Host "[compiler-static] issue58_string_bytes_surface_conformance: bytes_as_slice_allowed=1 builtin_tables_match_spec=1 drop_hooks_separate=1"
}

function Invoke-Issue59CapabilitiesAndContextConformanceCase {
    $failures = New-Object System.Collections.Generic.List[string]

    try {
        Invoke-ExpectedSuccessCase `
            -Id "issue59_capability_context_surface_ok" `
            -Source (New-Issue59CapabilitiesSurfaceSource)
    } catch {
        $failures.Add($_.Exception.Message) | Out-Null
    }

    $specPath = $canonicalSpecPath
    if (-not (Test-Path $specPath)) {
        $failures.Add("Case 'issue59_capability_context_conformance' missing spec file: $specPath") | Out-Null
    } else {
        $specText = Get-Content -Path $specPath -Raw
        if ($specText -notmatch '\*\*\(AllocRaw-Unsafe-Err\)\*\*') {
            $failures.Add("Case 'issue59_capability_context_conformance' missing AllocRaw-Unsafe-Err rule in canonical language spec.") | Out-Null
        }
        if ($specText -notmatch '\*\*\(DeallocRaw-Unsafe-Err\)\*\*') {
            $failures.Add("Case 'issue59_capability_context_conformance' missing DeallocRaw-Unsafe-Err rule in canonical language spec.") | Out-Null
        }
    }

    $diagMapPath = Join-Path $workspaceRoot "cursive\\src\\04_analysis\\typing\\item\\typecheck_diag_map.inc"
    if (-not (Test-Path $diagMapPath)) {
        $failures.Add("Case 'issue59_capability_context_conformance' missing typecheck diagnostic map: $diagMapPath") | Out-Null
    } else {
        $diagMapText = Get-Content -Path $diagMapPath -Raw
        if ($diagMapText -notmatch '\{"AllocRaw-Unsafe-Err",\s*"E-MEM-3030"\}') {
            $failures.Add("Case 'issue59_capability_context_conformance' missing AllocRaw-Unsafe-Err -> E-MEM-3030 mapping.") | Out-Null
        }
        if ($diagMapText -notmatch '\{"DeallocRaw-Unsafe-Err",\s*"E-MEM-3030"\}') {
            $failures.Add("Case 'issue59_capability_context_conformance' missing DeallocRaw-Unsafe-Err -> E-MEM-3030 mapping.") | Out-Null
        }
    }

    try {
        $allocResult = Invoke-CheckWithConformance `
            -CaseId "issue59_alloc_raw_outside_unsafe" `
            -Source (New-Issue59AllocRawOutsideUnsafeSource) `
            -ConformanceFileName "issue59_alloc_raw_outside_unsafe.log"

        if ($allocResult.ExitCode -ne 1) {
            $failures.Add("Case 'issue59_alloc_raw_outside_unsafe' expected exit 1 but got $($allocResult.ExitCode).") | Out-Null
        } else {
            $allocDiagCount = @($allocResult.DiagJson.diagnostics | Where-Object { $_.code -eq "E-MEM-3030" }).Count
            if ($allocDiagCount -lt 1) {
                $failures.Add("Case 'issue59_alloc_raw_outside_unsafe' expected E-MEM-3030 diagnostic.") | Out-Null
            }
            $allocLog = Get-Content -Path $allocResult.ConformancePath
            $allocRuleCount = @($allocLog | Where-Object { $_ -like "*`tAllocRaw-Unsafe-Err`t*" }).Count
            if ($allocRuleCount -lt 1) {
                $failures.Add("Case 'issue59_alloc_raw_outside_unsafe' expected AllocRaw-Unsafe-Err conformance trace row.") | Out-Null
            }
        }
    } catch {
        $failures.Add($_.Exception.Message) | Out-Null
    }

    try {
        $deallocResult = Invoke-CheckWithConformance `
            -CaseId "issue59_dealloc_raw_outside_unsafe" `
            -Source (New-Issue59DeallocRawOutsideUnsafeSource) `
            -ConformanceFileName "issue59_dealloc_raw_outside_unsafe.log"

        if ($deallocResult.ExitCode -ne 1) {
            $failures.Add("Case 'issue59_dealloc_raw_outside_unsafe' expected exit 1 with E-MEM-3030 per 5.9.3 DeallocRaw-Unsafe-Err, but got $($deallocResult.ExitCode).") | Out-Null
        } else {
            $deallocDiagCount = @($deallocResult.DiagJson.diagnostics | Where-Object { $_.code -eq "E-MEM-3030" }).Count
            if ($deallocDiagCount -lt 1) {
                $failures.Add("Case 'issue59_dealloc_raw_outside_unsafe' expected E-MEM-3030 diagnostic.") | Out-Null
            }
            $deallocLog = Get-Content -Path $deallocResult.ConformancePath
            $deallocRuleCount = @($deallocLog | Where-Object { $_ -like "*`tDeallocRaw-Unsafe-Err`t*" }).Count
            if ($deallocRuleCount -lt 1) {
                $failures.Add("Case 'issue59_dealloc_raw_outside_unsafe' expected DeallocRaw-Unsafe-Err conformance trace row.") | Out-Null
            }
        }
    } catch {
        $failures.Add($_.Exception.Message) | Out-Null
    }

    try {
        $methodLeakResult = Invoke-CheckWithConformance `
            -CaseId "issue59_method_capability_leak" `
            -Source (New-Issue59MethodCapabilityLeakSource) `
            -ConformanceFileName "issue59_method_capability_leak.log"

        if ($methodLeakResult.ExitCode -ne 1) {
            $failures.Add("Case 'issue59_method_capability_leak' expected exit 1 but got $($methodLeakResult.ExitCode).") | Out-Null
        } else {
            $methodLeakDiagCount = @($methodLeakResult.DiagJson.diagnostics | Where-Object { $_.code -eq "E-CON-0020" }).Count
            if ($methodLeakDiagCount -lt 1) {
                $failures.Add("Case 'issue59_method_capability_leak' expected E-CON-0020 diagnostic.") | Out-Null
            }

            $methodLeakLog = Get-Content -Path $methodLeakResult.ConformancePath
            $naa3RuleCount = @($methodLeakLog | Where-Object { $_ -like "*`tNAA-3`t*" }).Count
            if ($naa3RuleCount -lt 1) {
                $failures.Add("Case 'issue59_method_capability_leak' expected NAA-3 conformance trace row.") | Out-Null
            }
        }
    } catch {
        $failures.Add($_.Exception.Message) | Out-Null
    }

    try {
        $methodAllowedResult = Invoke-CheckWithConformance `
            -CaseId "issue59_method_capability_flow_allowed" `
            -Source (New-Issue59MethodCapabilityFlowAllowedSource) `
            -ConformanceFileName "issue59_method_capability_flow_allowed.log"

        if ($methodAllowedResult.ExitCode -ne 0) {
            $failures.Add("Case 'issue59_method_capability_flow_allowed' expected exit 0 but got $($methodAllowedResult.ExitCode).") | Out-Null
        } else {
            $methodAllowedErrors = @($methodAllowedResult.DiagJson.diagnostics | Where-Object {
                $_.severity -eq "error" -or $_.severity -eq "panic"
            }).Count
            if ($methodAllowedErrors -ne 0) {
                $failures.Add("Case 'issue59_method_capability_flow_allowed' expected zero compile-time errors but observed $methodAllowedErrors.") | Out-Null
            }

            $methodAllowedLeakCount = @($methodAllowedResult.DiagJson.diagnostics | Where-Object { $_.code -eq "E-CON-0020" }).Count
            if ($methodAllowedLeakCount -ne 0) {
                $failures.Add("Case 'issue59_method_capability_flow_allowed' unexpectedly emitted E-CON-0020.") | Out-Null
            }
        }
    } catch {
        $failures.Add($_.Exception.Message) | Out-Null
    }

    $methodCallPath = Join-Path $workspaceRoot "cursive\\src\\04_analysis\\typing\\expr\\method_call.cpp"
    if (-not (Test-Path $methodCallPath)) {
        $failures.Add("Case 'issue59_capability_context_conformance' missing method-call typing file: $methodCallPath") | Out-Null
    } else {
        $methodCallText = Get-Content -Path $methodCallPath -Raw
        $heapDispatchIsGeneric = [System.Text.RegularExpressions.Regex]::IsMatch(
            $methodCallText,
            'IsHeapAllocatorClassPath\(dyn->path\)\s*\)\s*\{\s*const auto sig = LookupHeapAllocatorMethodSig\(expr.name\);\s*if \(sig\.has_value\(\)\)\s*\{\s*if \(handle_cap_method\(sig->recv_perm, sig->params, sig->ret\)\)\s*\{\s*return result;\s*\}\s*\}\s*\}',
            [System.Text.RegularExpressions.RegexOptions]::Singleline)

        if ($heapDispatchIsGeneric) {
            $failures.Add("Case 'issue59_capability_context_conformance' found heap capability dispatch using generic handle_cap_method without an explicit unsafe gate branch for dealloc_raw.") | Out-Null
        }
    }

    $typingRoot = Join-Path $workspaceRoot "cursive\\src\\04_analysis\\typing"
    $deallocEmitHits = @(Find-PatternHits -Roots @($typingRoot) -Pattern 'diag_id\s*=\s*"DeallocRaw-Unsafe-Err"').Count
    if ($deallocEmitHits -lt 1) {
        $failures.Add("Case 'issue59_capability_context_conformance' found no typing emission path assigning diag_id = 'DeallocRaw-Unsafe-Err'.") | Out-Null
    }

    $callGraphPath = Join-Path $workspaceRoot "cursive\\src\\04_analysis\\caps\\callgraph_caps.cpp"
    if (-not (Test-Path $callGraphPath)) {
        $failures.Add("Case 'issue59_capability_context_conformance' missing call-graph capability analysis file: $callGraphPath") | Out-Null
    } else {
        $callGraphText = Get-Content -Path $callGraphPath -Raw
        $requiredPatterns = @(
            'std::get_if<ast::MethodDecl>',
            'std::get_if<ast::ClassMethodDecl>',
            'std::get_if<ast::StateMethodDecl>',
            'std::get_if<ast::TransitionDecl>',
            'SPEC_RULE\("NAA-3"\)'
        )
        foreach ($pattern in $requiredPatterns) {
            if (-not [System.Text.RegularExpressions.Regex]::IsMatch($callGraphText, $pattern, [System.Text.RegularExpressions.RegexOptions]::Singleline)) {
                $failures.Add("Case 'issue59_capability_context_conformance' missing expected callgraph capability pattern '$pattern'.") | Out-Null
            }
        }
    }

    $capRequirementsPath = Join-Path $workspaceRoot "cursive\\src\\04_analysis\\caps\\cap_requirements.cpp"
    if (-not (Test-Path $capRequirementsPath)) {
        $failures.Add("Case 'issue59_capability_context_conformance' missing capability requirements file: $capRequirementsPath") | Out-Null
    } else {
        $capRequirementsText = Get-Content -Path $capRequirementsPath -Raw
        $forbiddenPlaceholderPatterns = @(
            'This is a simplified implementation that checks the immediate expression',
            'A full implementation would recursively walk the expression tree',
            'Would need type information to determine if receiver is capability'
        )
        foreach ($pattern in $forbiddenPlaceholderPatterns) {
            if ($capRequirementsText -match $pattern) {
                $failures.Add("Case 'issue59_capability_context_conformance' found stale capability expression placeholder text '$pattern'.") | Out-Null
            }
        }
    }

    if ($failures.Count -gt 0) {
        foreach ($failure in $failures) {
            Write-Host "[compiler-static] issue59 failure: $failure"
        }
        throw "Case 'issue59_capability_context_conformance' found $($failures.Count) conformance violation(s)."
    }

    Write-Host "[compiler-static] issue59_capability_context_conformance: capability_surface=1 alloc_unsafe_gate=1 dealloc_unsafe_gate=1 method_naa3=1 callgraph_wiring=1 trace_rules=1"
}

function Invoke-Issue60TypeAnnotOptParseTraceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue60_type_annot_opt_parse_trace" `
        -Source (New-Issue60TypeAnnotOptParseTraceSource) `
        -ConformanceFileName "issue60_type_annot_opt_parse_trace.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue60_type_annot_opt_parse_trace' expected exit 0 but got $($result.ExitCode)."
    }
    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue60_type_annot_opt_parse_trace' expected zero compile-time errors but observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $noneCount = @($logLines | Where-Object { $_ -like "*`tParse-TypeAnnotOpt-None`t*" }).Count
    if ($noneCount -lt 1) {
        throw "Case 'issue60_type_annot_opt_parse_trace' expected Parse-TypeAnnotOpt-None in conformance trace."
    }

    $typeCommonPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\type\\type_common.cpp"
    if (-not (Test-Path $typeCommonPath)) {
        throw "Case 'issue60_type_annot_opt_parse_trace' missing parser implementation file: $typeCommonPath"
    }

    $typeCommonText = Get-Content -Path $typeCommonPath -Raw
    $hasNoneRule = $typeCommonText -match 'SPEC_RULE\("Parse-TypeAnnotOpt-None"\)'
    $hasYesRule = $typeCommonText -match 'SPEC_RULE\("Parse-TypeAnnotOpt-Yes"\)'
    $hasAdvanceThenParse = $typeCommonText -match 'Parser next = parser;\s*Advance\(next\);\s*ParseElemResult<std::shared_ptr<Type>> ty = ParseType\(next\);'
    if ((-not $hasNoneRule) -or (-not $hasYesRule) -or (-not $hasAdvanceThenParse)) {
        throw "Case 'issue60_type_annot_opt_parse_trace' expected Parse-TypeAnnotOpt implementation to contain both SPEC_RULE anchors and advance-before-ParseType semantics."
    }

    Write-Host "[compiler-static] issue60_type_annot_opt_parse_trace: exit=$($result.ExitCode) parse_typeannotopt_none=$noneCount parse_typeannotopt_yes=1"
}

function Invoke-Issue61KeyPathResolutionConformanceCase {
    Invoke-ExpectedSuccessCase `
        -Id "issue61_key_path_resolution_success" `
        -Source (New-Issue61KeyPathResolutionSuccessSource)

    Invoke-ExpectedDiagCodeCaseWithForbiddenCodes `
        -Id "issue61_key_block_unresolved_root" `
        -Source (New-Issue61KeyBlockUnresolvedRootSource) `
        -ExpectedCodes @("E-MOD-1301") `
        -ForbiddenCodes @("E-CON-0031")

    Invoke-ExpectedDiagCodeCaseWithForbiddenCodes `
        -Id "issue61_key_block_unresolved_index" `
        -Source (New-Issue61KeyBlockUnresolvedIndexSource) `
        -ExpectedCodes @("E-MOD-1301") `
        -ForbiddenCodes @("E-CON-0020")

    Invoke-ExpectedDiagCodeCase `
        -Id "issue61_dispatch_key_clause_unresolved_index" `
        -Source (New-Issue61DispatchKeyClauseUnresolvedIndexSource) `
        -ExpectedCodes @("E-MOD-1301")

    Write-Host "[compiler-static] issue61_key_path_resolution_conformance: success=1 unresolved_root=E-MOD-1301 unresolved_key_index=E-MOD-1301 unresolved_dispatch_index=E-MOD-1301"
}

function Invoke-Issue514ListSmallStepParseTraceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue514_list_small_step_parse_trace" `
        -Source (New-Issue514ListSmallStepParseTraceSource) `
        -ConformanceFileName "issue514_list_small_step_parse_trace.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue514_list_small_step_parse_trace' expected exit 0 but got $($result.ExitCode)."
    }
    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue514_list_small_step_parse_trace' expected zero compile-time errors but observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $listStartCount = @($logLines | Where-Object { $_ -like "*`tList-Start`t*" }).Count
    $listConsCount = @($logLines | Where-Object { $_ -like "*`tList-Cons`t*" }).Count
    $listDoneCount = @($logLines | Where-Object { $_ -like "*`tList-Done`t*" }).Count
    $argListEmptyCount = @($logLines | Where-Object { $_ -like "*`tParse-ArgList-Empty`t*" }).Count
    $argListConsCount = @($logLines | Where-Object { $_ -like "*`tParse-ArgList-Cons`t*" }).Count
    $exprListCommaCount = @($logLines | Where-Object { $_ -like "*`tParse-ExprListTail-Comma`t*" }).Count
    $exprListEndCount = @($logLines | Where-Object { $_ -like "*`tParse-ExprListTail-End`t*" }).Count
    $arraySegmentCommaCount = @($logLines | Where-Object { $_ -like "*`tParse-Array-Segment-List-Comma`t*" }).Count
    $arraySegmentSingleCount = @($logLines | Where-Object { $_ -like "*`tParse-Array-Segment-List-Single`t*" }).Count
    if ($listStartCount -lt 4 -or $listConsCount -lt 7 -or $listDoneCount -lt 4) {
        throw "Case 'issue514_list_small_step_parse_trace' expected List-Start/List-Cons/List-Done traces for empty arg lists, non-empty arg lists, tuple expression tails, and array segment lists."
    }
    if ($argListEmptyCount -lt 1 -or $argListConsCount -lt 1 -or
        $exprListCommaCount -lt 1 -or $exprListEndCount -lt 1 -or
        $arraySegmentCommaCount -lt 2 -or $arraySegmentSingleCount -lt 1) {
        throw "Case 'issue514_list_small_step_parse_trace' expected Parse-ArgList-Empty, Parse-ArgList-Cons, Parse-ExprListTail-Comma/End, and Parse-Array-Segment-List-Comma/Single in conformance trace."
    }

    $parserConsumePath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\parser_consume.cpp"
    if (-not (Test-Path $parserConsumePath)) {
        throw "Case 'issue514_list_small_step_parse_trace' missing parser consume implementation file: $parserConsumePath"
    }
    $parserConsumeText = Get-Content -Path $parserConsumePath -Raw
    $hasListStartRule = $parserConsumeText -match 'SPEC_RULE\("List-Start"\)'
    $hasListConsRule = $parserConsumeText -match 'SPEC_RULE\("List-Cons"\)'
    $hasListDoneRule = $parserConsumeText -match 'SPEC_RULE\("List-Done"\)'
    if ((-not $hasListStartRule) -or (-not $hasListConsRule) -or (-not $hasListDoneRule)) {
        throw "Case 'issue514_list_small_step_parse_trace' expected parser_consume.cpp to contain the canonical List-Start/List-Cons/List-Done SPEC_RULE anchors."
    }

    $parserHeaderPath = Join-Path $workspaceRoot "cursive\\include\\02_source\\parser\\parser.h"
    if (-not (Test-Path $parserHeaderPath)) {
        throw "Case 'issue514_list_small_step_parse_trace' missing parser header file: $parserHeaderPath"
    }
    $parserHeaderText = Get-Content -Path $parserHeaderPath -Raw
    $hasListScanFactory = $parserHeaderText -match 'inline ListState<Elem>\s+MakeListScanState\(Parser parser,\s*std::vector<Elem>\s+elems\)'
    $hasListConsScanTransition = $parserHeaderText -match 'if\s*\(state\.tag\s*!=\s*ListStateTag::Scan\)\s*\{\s*return state;\s*\}\s*RecordListCons\(\);\s*ParseElemResult<Elem>\s+parsed\s*=\s*parse_elem\(state\.parser\);\s*std::vector<Elem>\s+elems\s*=\s*std::move\(state\.elems\);\s*elems\.push_back\(std::move\(parsed\.elem\)\);\s*return\s+MakeListScanState\(parsed\.parser,\s*std::move\(elems\)\);'
    if ((-not $hasListScanFactory) -or (-not $hasListConsScanTransition)) {
        throw "Case 'issue514_list_small_step_parse_trace' expected parser.h to model List-Cons as an explicit ListScan(P, xs) -> ListScan(P', xs ++ [x]) transition."
    }

    $callPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\expr\\call.cpp"
    if (-not (Test-Path $callPath)) {
        throw "Case 'issue514_list_small_step_parse_trace' missing call parser file: $callPath"
    }
    $callText = Get-Content -Path $callPath -Raw
    $hasCallListStart = $callText -match 'ListState<Arg>\s+state\s*=\s*ListStart<Arg>\(parser\);'
    $hasCallListCons = $callText -match 'state\s*=\s*ListCons\([^,]+,\s*ParseArg\);'
    $hasCallListDone = $callText -match 'ListDone\(state,\s*end_set\)'
    if ((-not $hasCallListStart) -or (-not $hasCallListCons) -or (-not $hasCallListDone)) {
        throw "Case 'issue514_list_small_step_parse_trace' expected call.cpp to route argument list parsing through ListStart/ListCons/ListDone."
    }

    $arrayPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\expr\\array_literal.cpp"
    if (-not (Test-Path $arrayPath)) {
        throw "Case 'issue514_list_small_step_parse_trace' missing array literal parser file: $arrayPath"
    }
    $arrayText = Get-Content -Path $arrayPath -Raw
    $hasArrayListStart = $arrayText -match 'ListState<ArraySegment>\s+state\s*=\s*ListStart<ArraySegment>\(parser\);'
    $hasArrayListCons = $arrayText -match 'state\s*=\s*ListCons\([^,]+,\s*ParseArraySegment\);'
    $hasArrayListDone = $arrayText -match 'ListDone\(state,\s*end_set\)'
    if ((-not $hasArrayListStart) -or (-not $hasArrayListCons) -or (-not $hasArrayListDone)) {
        throw "Case 'issue514_list_small_step_parse_trace' expected array_literal.cpp to route array segment lists through ListStart/ListCons/ListDone."
    }

    $tuplePath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\expr\\tuple_literal.cpp"
    if (-not (Test-Path $tuplePath)) {
        throw "Case 'issue514_list_small_step_parse_trace' missing tuple literal parser file: $tuplePath"
    }
    $tupleText = Get-Content -Path $tuplePath -Raw
    $hasTupleSeed = $tupleText -match 'ParseExprListTail\(ListSeed\(second\.parser,\s*second\.elem\)\)'
    $hasTupleListDone = $tupleText -match 'ListDone\(state,\s*end_set\)'
    $hasTupleTailCommaRule = $tupleText -match 'SPEC_RULE\("Parse-ExprListTail-Comma"\)'
    $hasTupleTailEndRule = $tupleText -match 'SPEC_RULE\("Parse-ExprListTail-End"\)'
    $hasTupleTailTrailingRule = $tupleText -match 'SPEC_RULE\("Parse-ExprListTail-TrailingComma"\)'
    if ((-not $hasTupleSeed) -or (-not $hasTupleListDone) -or
        (-not $hasTupleTailCommaRule) -or (-not $hasTupleTailEndRule) -or
        (-not $hasTupleTailTrailingRule)) {
        throw "Case 'issue514_list_small_step_parse_trace' expected tuple_literal.cpp to seed expression-list tails canonically and emit Parse-ExprListTail-* rule anchors."
    }

    Write-Host "[compiler-static] issue514_list_small_step_parse_trace: exit=$($result.ExitCode) list_start=$listStartCount list_cons=$listConsCount list_done=$listDoneCount arg_empty=$argListEmptyCount arg_cons=$argListConsCount expr_tail_comma=$exprListCommaCount expr_tail_end=$exprListEndCount array_segment_comma=$arraySegmentCommaCount array_segment_single=$arraySegmentSingleCount"
}

function Invoke-Issue514TrailingCommaEndSetConformanceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue514_trailing_comma_end_set_conformance" `
        -Source (New-Issue514TrailingCommaEndSetConformanceSource) `
        -ConformanceFileName "issue514_trailing_comma_end_set_conformance.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue514_trailing_comma_end_set_conformance' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue514_trailing_comma_end_set_conformance' expected zero compile-time errors but observed $errorCount."
    }

    $parserHeaderPath = Join-Path $workspaceRoot "cursive\\include\\02_source\\parser\\parser.h"
    if (-not (Test-Path $parserHeaderPath)) {
        throw "Case 'issue514_trailing_comma_end_set_conformance' missing parser header file: $parserHeaderPath"
    }
    $parserHeaderText = Get-Content -Path $parserHeaderPath -Raw
    $hasEndSetToken = $parserHeaderText -match 'struct EndSetToken\s*\{\s*TokenKind kind = TokenKind::Unknown;\s*std::string_view lexeme;\s*\};'
    $hasTrailingCommaSignature = $parserHeaderText -match 'bool\s+TrailingComma\(const Parser& parser,\s*std::span<const EndSetToken>\s+end_set\);'
    $hasTrailingAllowedSignature = $parserHeaderText -match 'bool\s+TrailingCommaAllowed\(const Parser& parser,\s*std::span<const EndSetToken>\s+end_set\);'
    if ((-not $hasEndSetToken) -or (-not $hasTrailingCommaSignature) -or (-not $hasTrailingAllowedSignature)) {
        throw "Case 'issue514_trailing_comma_end_set_conformance' expected parser.h to model TrailingComma and TrailingCommaAllowed over explicit EndSetToken inputs."
    }

    $parserConsumePath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\parser_consume.cpp"
    if (-not (Test-Path $parserConsumePath)) {
        throw "Case 'issue514_trailing_comma_end_set_conformance' missing parser consume implementation file: $parserConsumePath"
    }
    $parserConsumeText = Get-Content -Path $parserConsumePath -Raw
    $hasTokenMatchOverload = $parserConsumeText -match 'bool\s+TokenMatches\(const Token& tok,\s*const EndSetToken& match\)'
    $hasTrailingCommaImpl = $parserConsumeText -match 'bool\s+TrailingComma\(const Parser& parser,\s*std::span<const EndSetToken>\s+end_set\)'
    $hasTrailingAllowedImpl = $parserConsumeText -match 'bool\s+TrailingCommaAllowed\(const Parser& parser,\s*std::span<const EndSetToken>\s+end_set\)'
    if ((-not $hasTokenMatchOverload) -or (-not $hasTrailingCommaImpl) -or (-not $hasTrailingAllowedImpl)) {
        throw "Case 'issue514_trailing_comma_end_set_conformance' expected parser_consume.cpp to implement explicit EndSetToken-aware TrailingComma and TrailingCommaAllowed helpers."
    }

    $callPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\expr\\call.cpp"
    if (-not (Test-Path $callPath)) {
        throw "Case 'issue514_trailing_comma_end_set_conformance' missing call parser file: $callPath"
    }
    $callText = Get-Content -Path $callPath -Raw
    $hasCallEndSet = $callText -match 'std::array<EndSetToken,\s*1>\s+end_set\s*=\s*\{EndPunct\("\)"\)\};'
    if (-not $hasCallEndSet) {
        throw "Case 'issue514_trailing_comma_end_set_conformance' expected call.cpp to build trailing-comma end sets with EndSetToken entries."
    }

    Write-Host "[compiler-static] issue514_trailing_comma_end_set_conformance: exit=$($result.ExitCode) errors=$errorCount endset_token=$hasEndSetToken trailing_sig=$hasTrailingCommaSignature trailing_allowed_sig=$hasTrailingAllowedSignature trailing_impl=$hasTrailingCommaImpl trailing_allowed_impl=$hasTrailingAllowedImpl call_endset=$hasCallEndSet"
}

function Invoke-Issue514TrailingCommaErrConformanceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue514_trailing_comma_err_conformance" `
        -Source (New-Issue514TrailingCommaErrSingleLineCallSource) `
        -ConformanceFileName "issue514_trailing_comma_err_conformance.log"

    if ($result.ExitCode -ne 1) {
        throw "Case 'issue514_trailing_comma_err_conformance' expected exit 1 but got $($result.ExitCode)."
    }

    $errorDiagnostics = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    })
    $errorCodes = @($errorDiagnostics | ForEach-Object { $_.code })
    if ($errorCodes -notcontains "E-SRC-0521") {
        throw "Case 'issue514_trailing_comma_err_conformance' expected E-SRC-0521 in diagnostics."
    }

    $commaSpanCount = @($errorDiagnostics | Where-Object {
        $_.code -eq "E-SRC-0521" -and
        $null -ne $_.span -and
        [int]$_.span.start_line -eq 7 -and
        [int]$_.span.start_col -eq 29
    }).Count
    if ($commaSpanCount -lt 1) {
        throw "Case 'issue514_trailing_comma_err_conformance' expected E-SRC-0521 to point at the trailing comma token."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $errRuleCount = @($logLines | Where-Object {
        $_ -like "*`tTrailing-Comma-Err`t*"
    }).Count
    $argTailTrailingCount = @($logLines | Where-Object {
        $_ -like "*`tParse-ArgTail-TrailingComma`t*"
    }).Count
    if ($errRuleCount -lt 1) {
        throw "Case 'issue514_trailing_comma_err_conformance' expected Trailing-Comma-Err in conformance trace."
    }
    if ($argTailTrailingCount -ne 0) {
        throw "Case 'issue514_trailing_comma_err_conformance' must not emit Parse-ArgTail-TrailingComma for an invalid single-line trailing comma."
    }

    $parserConsumePath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\parser_consume.cpp"
    if (-not (Test-Path $parserConsumePath)) {
        throw "Case 'issue514_trailing_comma_err_conformance' missing parser consume implementation file: $parserConsumePath"
    }
    $parserConsumeText = Get-Content -Path $parserConsumePath -Raw
    $hasLocalEmitState = $parserConsumeText -match 'struct\s+InvalidTrailingCommaEmitState'
    $hasLocalEmitPredicate = $parserConsumeText -match 'InvalidTrailingCommaForEmit\(\s*const Parser& parser,\s*std::span<const EndSetToken>\s+end_set\s*\)'
    $hasEmitImpl = $parserConsumeText -match 'bool\s+EmitTrailingCommaErr\(Parser& parser,\s*std::span<const EndSetToken>\s+end_set\)'
    $hasEmitLocalGuard = $parserConsumeText -match 'const std::optional<InvalidTrailingCommaEmitState>\s+invalid\s*=\s*InvalidTrailingCommaForEmit\(parser,\s*end_set\);'
    $hasEmitCommaSpan = $parserConsumeText -match 'MakeDiagnosticById\("E-SRC-0521",\s*invalid->comma->span\)'
    if ((-not $hasLocalEmitState) -or (-not $hasLocalEmitPredicate) -or
        (-not $hasEmitImpl) -or (-not $hasEmitLocalGuard) -or
        (-not $hasEmitCommaSpan)) {
        throw "Case 'issue514_trailing_comma_err_conformance' expected parser_consume.cpp to evaluate Trailing-Comma-Err locally and emit the comma-token span."
    }

    Write-Host "[compiler-static] issue514_trailing_comma_err_conformance: exit=$($result.ExitCode) error_codes=$($errorCodes -join ',') comma_span=$commaSpanCount trailing_comma_err=$errRuleCount parse_arg_tail_trailing=$argTailTrailingCount emit_state=$hasLocalEmitState emit_guard=$hasEmitLocalGuard emit_span=$hasEmitCommaSpan"
}

function Invoke-Issue514TupleExprSingletonCommaRejectedCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue514_tuple_expr_singleton_comma_rejected" `
        -Source (New-Issue514TupleExprSingletonCommaRejectedSource) `
        -ConformanceFileName "issue514_tuple_expr_singleton_comma_rejected.log"

    if ($result.ExitCode -ne 1) {
        throw "Case 'issue514_tuple_expr_singleton_comma_rejected' expected exit 1 but got $($result.ExitCode)."
    }

    $errorCodes = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    } | ForEach-Object { $_.code })
    if ($errorCodes -notcontains "E-SRC-0520") {
        throw "Case 'issue514_tuple_expr_singleton_comma_rejected' expected E-SRC-0520 in diagnostics."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $parenExprCount = @($logLines | Where-Object {
        $_ -like "*`tParse-Paren-Expr`t*"
    }).Count
    $tupleExprCount = @($logLines | Where-Object {
        $_ -like "*`tParse-Tuple-Expr`t*"
    }).Count
    $tupleManyCount = @($logLines | Where-Object {
        $_ -like "*`tParse-TupleExprElems-Many`t*"
    }).Count
    $tupleSingleCount = @($logLines | Where-Object {
        $_ -like "*`tParse-TupleExprElems-Single`t*"
    }).Count
    if ($parenExprCount -lt 1) {
        throw "Case 'issue514_tuple_expr_singleton_comma_rejected' expected Parse-Paren-Expr in conformance trace."
    }
    if ($tupleExprCount -ne 0 -or $tupleManyCount -ne 0 -or $tupleSingleCount -ne 0) {
        throw "Case 'issue514_tuple_expr_singleton_comma_rejected' must not classify '(e,)' as a tuple expression in conformance trace."
    }

    Write-Host "[compiler-static] issue514_tuple_expr_singleton_comma_rejected: exit=$($result.ExitCode) error_codes=$($errorCodes -join ',') parse_paren_expr=$parenExprCount parse_tuple_expr=$tupleExprCount parse_tuple_many=$tupleManyCount parse_tuple_single=$tupleSingleCount"
}

function Invoke-ConsumeStateSurfaceConformanceCase {
    $specText = Get-Content -Path $canonicalSpecPath -Raw
    if ($specText -notmatch 'ConsumeState\s*=\s*\{Consume\(P,\s*k\),\s*ConsumeDone\(P\)\}') {
        throw "Case 'consume_state_surface_conformance' missing canonical ConsumeState rule in the language spec."
    }

    $headerPath = Join-Path $workspaceRoot "cursive\\include\\02_source\\parser\\parser.h"
    $implPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\parser\\parser_consume.cpp"
    foreach ($path in @($headerPath, $implPath)) {
        if (-not (Test-Path $path)) {
            throw "Case 'consume_state_surface_conformance' missing compiler source file: $path"
        }
    }

    $headerText = Get-Content -Path $headerPath -Raw
    $implText = Get-Content -Path $implPath -Raw

    $headerPatterns = @(
        'struct\s+ConsumePendingState\s*\{\s*Parser\s+parser;\s*TokenKindMatch\s+expected;\s*\}',
        'struct\s+ConsumeDoneState\s*\{\s*Parser\s+parser;\s*\}',
        'using\s+ConsumeState\s*=\s*std::variant<\s*ConsumePendingState\s*,\s*ConsumeDoneState\s*>;',
        'inline\s+ConsumeState\s+Consume\s*\(\s*Parser\s+parser,\s*TokenKindMatch\s+expected\s*\)',
        'inline\s+ConsumeDoneState\s+ConsumeDone\s*\(\s*Parser\s+parser\s*\)',
        'std::optional<ConsumeDoneState>\s+TryAdvanceConsume\s*\(\s*const\s+ConsumePendingState&\s+state\s*\);'
    )
    foreach ($pattern in $headerPatterns) {
        if ($headerText -notmatch $pattern) {
            throw "Case 'consume_state_surface_conformance' missing expected parser.h pattern '$pattern'."
        }
    }

    $implPatterns = @(
        'std::optional<ConsumeDoneState>\s+TryAdvanceConsume\s*\(\s*const\s+ConsumePendingState&\s+state\s*\)',
        'ConsumeState\s+state\s*=\s*Consume\s*\(\s*parser\s*,\s*expected\s*\);',
        'std::optional<ConsumeDoneState>\s+done\s*=\s*TryAdvanceConsume\s*\(\s*\*pending\s*\);',
        'return\s+ConsumeByMatch\s*\(\s*parser\s*,\s*MatchKind\(kind\)\s*,\s*"Tok-Consume-Kind"\s*\)\s*;',
        'return\s+ConsumeByMatch\s*\(\s*parser\s*,\s*MatchKeyword\(keyword\)\s*,\s*"Tok-Consume-Keyword"\s*\)\s*;',
        'return\s+ConsumeByMatch\s*\(\s*parser\s*,\s*MatchOperator\(op\)\s*,\s*"Tok-Consume-Operator"\s*\)\s*;',
        'return\s+ConsumeByMatch\s*\(\s*parser\s*,\s*MatchPunct\(punct\)\s*,\s*"Tok-Consume-Punct"\s*\)\s*;'
    )
    foreach ($pattern in $implPatterns) {
        if ($implText -notmatch $pattern) {
            throw "Case 'consume_state_surface_conformance' missing expected parser_consume.cpp pattern '$pattern'."
        }
    }

    Write-Host "[compiler-static] consume_state_surface_conformance: header_patterns=$($headerPatterns.Count) impl_patterns=$($implPatterns.Count)"
}

function Invoke-Issue510EnumDiscriminantDefaultsConformanceCase {
    $failures = New-Object System.Collections.Generic.List[string]

    $specPath = $canonicalSpecPath
    if (-not (Test-Path $specPath)) {
        $failures.Add("Case 'issue510_enum_discriminant_defaults_conformance' missing spec file: $specPath") | Out-Null
    } else {
        $specText = Get-Content -Path $specPath -Raw
        if ($specText -notmatch 'DiscValue\(tok\)\s*=\s*IntValue\(tok\)') {
            $failures.Add("Case 'issue510_enum_discriminant_defaults_conformance' missing DiscValue(tok) = IntValue(tok) rule in canonical language spec.") | Out-Null
        }
        if ($specText -notmatch 's = "0x" \+\+ h' -or
            $specText -notmatch 's = "0o" \+\+ o' -or
            $specText -notmatch 's = "0b" \+\+ b') {
            $failures.Add("Case 'issue510_enum_discriminant_defaults_conformance' missing IntValueCore base forms (hex/octal/binary) in spec literal semantics.") | Out-Null
        }
    }

    $enumDeclPath = Join-Path $workspaceRoot "cursive\\src\\04_analysis\\typing\\item\\enum_decl.cpp"
    if (-not (Test-Path $enumDeclPath)) {
        $failures.Add("Case 'issue510_enum_discriminant_defaults_conformance' missing enum declaration typing file: $enumDeclPath") | Out-Null
    } else {
        $enumDeclText = Get-Content -Path $enumDeclPath -Raw
        if ($enumDeclText -match 'std::stoll\s*\(') {
            $failures.Add("Case 'issue510_enum_discriminant_defaults_conformance' found std::stoll-based discriminant parsing in enum_decl.cpp; expected canonical EnumDiscriminants path.") | Out-Null
        }
        if ($enumDeclText -notmatch 'EnumDiscriminants\(decl\)') {
            $failures.Add("Case 'issue510_enum_discriminant_defaults_conformance' missing EnumDiscriminants(decl) usage in enum_decl.cpp.") | Out-Null
        }
    }

    $typeDeclsPath = Join-Path $workspaceRoot "cursive\\include\\04_analysis\\typing\\type_decls.h"
    if (-not (Test-Path $typeDeclsPath)) {
        $failures.Add("Case 'issue510_enum_discriminant_defaults_conformance' missing type declaration header: $typeDeclsPath") | Out-Null
    } else {
        $typeDeclsText = Get-Content -Path $typeDeclsPath -Raw
        if ($typeDeclsText -notmatch 'std::uint64_t\s+discriminant\s*=\s*0;') {
            $failures.Add("Case 'issue510_enum_discriminant_defaults_conformance' expected VariantInfo discriminant storage to be std::uint64_t.") | Out-Null
        }
    }

    $zeroablePredPath = Join-Path $workspaceRoot "cursive\\src\\04_analysis\\typing\\type_predicates.cpp"
    if (-not (Test-Path $zeroablePredPath)) {
        $failures.Add("Case 'issue510_enum_discriminant_defaults_conformance' missing shared type-predicate file: $zeroablePredPath") | Out-Null
    } else {
        $zeroablePredText = Get-Content -Path $zeroablePredPath -Raw
        if ($zeroablePredText -notmatch 'EnumDiscriminants\(decl\)') {
            $failures.Add("Case 'issue510_enum_discriminant_defaults_conformance' expected ZeroableType enum path to use EnumDiscriminants(decl).") | Out-Null
        }
        if ($zeroablePredText -match 'ParseIntegralDiscriminant') {
            $failures.Add("Case 'issue510_enum_discriminant_defaults_conformance' found legacy ParseIntegralDiscriminant helper after canonicalization.") | Out-Null
        }
    }

    $successCases = @(
        @{ Id = "issue510_enum_disc_hex_ok"; Source = (New-Issue510EnumDiscHexSource) },
        @{ Id = "issue510_enum_disc_binary_ok"; Source = (New-Issue510EnumDiscBinarySource) },
        @{ Id = "issue510_enum_disc_octal_ok"; Source = (New-Issue510EnumDiscOctalSource) },
        @{ Id = "issue510_enum_disc_underscore_ok"; Source = (New-Issue510EnumDiscUnderscoreSource) },
        @{ Id = "issue510_enum_disc_suffix_ok"; Source = (New-Issue510EnumDiscSuffixSource) },
        @{ Id = "issue510_enum_disc_u64_max_ok"; Source = (New-Issue510EnumDiscU64MaxSource) }
    )

    foreach ($case in $successCases) {
        try {
            Invoke-ExpectedSuccessCase -Id $case.Id -Source $case.Source
        } catch {
            $failures.Add($_.Exception.Message) | Out-Null
        }
    }

    try {
        $overU64 = Invoke-CheckWithConformance `
            -CaseId "issue510_enum_disc_over_u64_invalid" `
            -Source (New-Issue510EnumDiscOverU64Source) `
            -ConformanceFileName "issue510_enum_disc_over_u64_invalid.log"

        if ($overU64.ExitCode -ne 1) {
            $failures.Add("Case 'issue510_enum_disc_over_u64_invalid' expected exit 1 but got $($overU64.ExitCode).") | Out-Null
        } else {
            $diagCount = @($overU64.DiagJson.diagnostics | Where-Object { $_.code -eq "E-TYP-1921" }).Count
            if ($diagCount -lt 1) {
                $failures.Add("Case 'issue510_enum_disc_over_u64_invalid' expected E-TYP-1921 diagnostic.") | Out-Null
            }
            $trace = Get-Content -Path $overU64.ConformancePath
            $ruleCount = @($trace | Where-Object { $_ -like "*`tEnum-Disc-Invalid`t*" }).Count
            if ($ruleCount -lt 1) {
                $failures.Add("Case 'issue510_enum_disc_over_u64_invalid' expected Enum-Disc-Invalid conformance trace row.") | Out-Null
            }
        }
    } catch {
        $failures.Add($_.Exception.Message) | Out-Null
    }

    try {
        $dup = Invoke-CheckWithConformance `
            -CaseId "issue510_enum_disc_duplicate_invalid" `
            -Source (New-Issue510EnumDiscDuplicateSource) `
            -ConformanceFileName "issue510_enum_disc_duplicate_invalid.log"

        if ($dup.ExitCode -ne 1) {
            $failures.Add("Case 'issue510_enum_disc_duplicate_invalid' expected exit 1 but got $($dup.ExitCode).") | Out-Null
        } else {
            $diagCount = @($dup.DiagJson.diagnostics | Where-Object { $_.code -eq "E-TYP-1923" }).Count
            if ($diagCount -lt 1) {
                $failures.Add("Case 'issue510_enum_disc_duplicate_invalid' expected E-TYP-1923 diagnostic.") | Out-Null
            }
            $trace = Get-Content -Path $dup.ConformancePath
            $ruleCount = @($trace | Where-Object { $_ -like "*`tEnum-Disc-Dup`t*" }).Count
            if ($ruleCount -lt 1) {
                $failures.Add("Case 'issue510_enum_disc_duplicate_invalid' expected Enum-Disc-Dup conformance trace row.") | Out-Null
            }
        }
    } catch {
        $failures.Add($_.Exception.Message) | Out-Null
    }

    try {
        $overflowImplicit = Invoke-CheckWithConformance `
            -CaseId "issue510_enum_disc_u64_max_plus_implicit_invalid" `
            -Source (New-Issue510EnumDiscU64MaxPlusImplicitSource) `
            -ConformanceFileName "issue510_enum_disc_u64_max_plus_implicit_invalid.log"

        if ($overflowImplicit.ExitCode -ne 1) {
            $failures.Add("Case 'issue510_enum_disc_u64_max_plus_implicit_invalid' expected exit 1 but got $($overflowImplicit.ExitCode).") | Out-Null
        } else {
            $diagCount = @($overflowImplicit.DiagJson.diagnostics | Where-Object { $_.code -eq "E-TYP-1921" }).Count
            if ($diagCount -lt 1) {
                $failures.Add("Case 'issue510_enum_disc_u64_max_plus_implicit_invalid' expected E-TYP-1921 diagnostic.") | Out-Null
            }
            $trace = Get-Content -Path $overflowImplicit.ConformancePath
            $ruleCount = @($trace | Where-Object { $_ -like "*`tEnum-Disc-Invalid`t*" }).Count
            if ($ruleCount -lt 1) {
                $failures.Add("Case 'issue510_enum_disc_u64_max_plus_implicit_invalid' expected Enum-Disc-Invalid conformance trace row.") | Out-Null
            }
        }
    } catch {
        $failures.Add($_.Exception.Message) | Out-Null
    }

    if ($failures.Count -gt 0) {
        foreach ($failure in $failures) {
            Write-Host "[compiler-static] issue510 failure: $failure"
        }
        throw "Case 'issue510_enum_discriminant_defaults_conformance' found $($failures.Count) conformance violation(s)."
    }

    Write-Host "[compiler-static] issue510_enum_discriminant_defaults_conformance: literal_forms=1 bounds=1 diagnostics=1 trace_rules=1"
}

function Invoke-Issue559EnumEmptyConformanceCase {
    $failures = New-Object System.Collections.Generic.List[string]

    $specPath = $canonicalSpecPath
    if (-not (Test-Path $specPath)) {
        $failures.Add("Case 'issue559_enum_empty_conformance' missing spec file: $specPath") | Out-Null
    } else {
        $specText = Get-Content -Path $specPath -Raw
        if ($specText -notmatch '\*\*\(Enum-Empty-Err\)\*\*') {
            $failures.Add("Case 'issue559_enum_empty_conformance' missing Enum-Empty-Err rule in canonical language spec.") | Out-Null
        }
        if ($specText -notmatch '\|\s*`E-TYP-2001`\s*\|') {
            $failures.Add("Case 'issue559_enum_empty_conformance' missing E-TYP-2001 diagnostic in canonical language spec.") | Out-Null
        }
    }

    $enumDeclPath = Join-Path $workspaceRoot "cursive\\src\\04_analysis\\typing\\item\\enum_decl.cpp"
    if (-not (Test-Path $enumDeclPath)) {
        $failures.Add("Case 'issue559_enum_empty_conformance' missing enum declaration typing file: $enumDeclPath") | Out-Null
    } else {
        $enumDeclText = Get-Content -Path $enumDeclPath -Raw
        $emptyCheckCount = ([regex]::Matches($enumDeclText, 'if\s*\(decl\.variants\.empty\(\)\)')).Count
        $ruleCount = ([regex]::Matches($enumDeclText, 'SPEC_RULE\("Enum-Empty-Err"\)')).Count
        $diagCount = ([regex]::Matches($enumDeclText, 'result\.diag_id\s*=\s*"E-TYP-2001"')).Count
        if ($emptyCheckCount -lt 2 -or $ruleCount -lt 2 -or $diagCount -lt 2) {
            $failures.Add("Case 'issue559_enum_empty_conformance' expected empty-enum guard, Enum-Empty-Err trace, and E-TYP-2001 assignment in both enum typing passes.") | Out-Null
        }
    }

    $diagTablePath = Join-Path $workspaceRoot "cursive\\src\\00_core\\diagnostic_messages_table.inc"
    if (-not (Test-Path $diagTablePath)) {
        $failures.Add("Case 'issue559_enum_empty_conformance' missing diagnostic message table: $diagTablePath") | Out-Null
    } else {
        $diagTableText = Get-Content -Path $diagTablePath -Raw
        if ($diagTableText -notmatch '\{"E-TYP-2001",\s*Severity::Error,\s*"Enum declaration contains no variants"\}') {
            $failures.Add("Case 'issue559_enum_empty_conformance' missing E-TYP-2001 message table entry.") | Out-Null
        }
    }

    try {
        Invoke-ExpectedSuccessCase `
            -Id "issue559_enum_nonempty_accepted" `
            -Source (New-Issue559EnumNonEmptyAcceptedSource)
    } catch {
        $failures.Add($_.Exception.Message) | Out-Null
    }

    try {
        $empty = Invoke-CheckWithConformance `
            -CaseId "issue559_enum_empty_rejected" `
            -Source (New-Issue559EnumEmptyRejectedSource) `
            -ConformanceFileName "issue559_enum_empty_rejected.log"

        if ($empty.ExitCode -ne 1) {
            $failures.Add("Case 'issue559_enum_empty_rejected' expected exit 1 but got $($empty.ExitCode).") | Out-Null
        } else {
            $diagCount = @($empty.DiagJson.diagnostics | Where-Object { $_.code -eq "E-TYP-2001" }).Count
            if ($diagCount -lt 1) {
                $failures.Add("Case 'issue559_enum_empty_rejected' expected E-TYP-2001 diagnostic.") | Out-Null
            }
            $trace = Get-Content -Path $empty.ConformancePath
            $ruleCount = @($trace | Where-Object { $_ -like "*`tEnum-Empty-Err`t*" }).Count
            if ($ruleCount -lt 1) {
                $failures.Add("Case 'issue559_enum_empty_rejected' expected Enum-Empty-Err conformance trace row.") | Out-Null
            }
        }
    } catch {
        $failures.Add($_.Exception.Message) | Out-Null
    }

    if ($failures.Count -gt 0) {
        foreach ($failure in $failures) {
            Write-Host "[compiler-static] issue559 failure: $failure"
        }
        throw "Case 'issue559_enum_empty_conformance' found $($failures.Count) conformance violation(s)."
    }

    Write-Host "[compiler-static] issue559_enum_empty_conformance: nonempty_ok=1 empty_diag=E-TYP-2001 trace_rule=Enum-Empty-Err"
}

function Invoke-Issue511FoundationalClassesAndPipeConformanceCase {
    $failures = New-Object System.Collections.Generic.List[string]

    $resolveModulePath = Join-Path $workspaceRoot "cursive\\src\\04_analysis\\resolve\\resolve_module.cpp"
    if (-not (Test-Path $resolveModulePath)) {
        $failures.Add("Case 'issue511_foundational_classes_and_pipe_conformance' missing resolver module file: $resolveModulePath") | Out-Null
    } else {
        $resolveModuleText = Get-Content -Path $resolveModulePath -Raw
        foreach ($name in @("Eq", "Hasher", "Hash", "FfiSafe")) {
            if ($resolveModuleText -notmatch "path\.emplace_back\(`"$name`"\)") {
                $failures.Add("Case 'issue511_foundational_classes_and_pipe_conformance' missing built-in class injection for '$name' in resolve_module.cpp.") | Out-Null
            }
        }
        if ($resolveModuleText -notmatch 'next_method\.return_type_opt\s*=\s*make_type_union\(\s*make_type_path\(\{"Self",\s*"Item"\}\),\s*make_type_prim\("\(\)"\)\s*\)') {
            $failures.Add("Case 'issue511_foundational_classes_and_pipe_conformance' expected built-in Iterator::next return type `Self::Item | ()` in resolve_module.cpp.") | Out-Null
        }
    }

    $resolveTypesPath = Join-Path $workspaceRoot "cursive\\src\\04_analysis\\resolve\\resolve_types.cpp"
    if (-not (Test-Path $resolveTypesPath)) {
        $failures.Add("Case 'issue511_foundational_classes_and_pipe_conformance' missing resolve_types file: $resolveTypesPath") | Out-Null
    } else {
        $resolveTypesText = Get-Content -Path $resolveTypesPath -Raw
        foreach ($name in @("Eq", "Hasher", "Hash", "FfiSafe")) {
            if ($resolveTypesText -notmatch "IdEq\(path\[0\],\s*`"$name`"\)") {
                $failures.Add("Case 'issue511_foundational_classes_and_pipe_conformance' missing foundational class path recognition for '$name' in resolve_types.cpp.") | Out-Null
            }
        }
    }

    $recordDeclPath = Join-Path $workspaceRoot "cursive\\src\\04_analysis\\typing\\item\\record_decl.cpp"
    if (-not (Test-Path $recordDeclPath)) {
        $failures.Add("Case 'issue511_foundational_classes_and_pipe_conformance' missing record declaration typing file: $recordDeclPath") | Out-Null
    } else {
        $recordDeclText = Get-Content -Path $recordDeclPath -Raw
        if ($recordDeclText -match "iterator_next_legacy") {
            $failures.Add("Case 'issue511_foundational_classes_and_pipe_conformance' found legacy Iterator::next signature bypass in record_decl.cpp.") | Out-Null
        }
    }

    try {
        Invoke-ExpectedSuccessCase `
            -Id "issue511_eq_class_bound_success" `
            -Source (New-Issue511EqClassBoundSuccessSource)
    } catch {
        $failures.Add($_.Exception.Message) | Out-Null
    }

    try {
        Invoke-ExpectedSuccessCase `
            -Id "issue511_bitcopy_class_bound_success" `
            -Source (New-Issue511BitcopyClassBoundSuccessSource)
    } catch {
        $failures.Add($_.Exception.Message) | Out-Null
    }

    try {
        Invoke-ExpectedSuccessCase `
            -Id "issue511_pipe_refinement_loop_invariant_success" `
            -Source (New-Issue511PipeRefinementAndInvariantSyntaxSource)
    } catch {
        $failures.Add($_.Exception.Message) | Out-Null
    }

    try {
        Invoke-ExpectedSuccessCase `
            -Id "issue511_pipe_iterator_loop_invariant_success" `
            -Source (New-Issue511PipeLoopIteratorInvariantSyntaxSource)
    } catch {
        $failures.Add($_.Exception.Message) | Out-Null
    }

    try {
        Invoke-ExpectedSuccessCase `
            -Id "issue511_pipe_infinite_loop_invariant_success" `
            -Source (New-Issue511PipeLoopInfiniteInvariantSyntaxSource)
    } catch {
        $failures.Add($_.Exception.Message) | Out-Null
    }

    try {
        Invoke-ExpectedDiagCodeCase `
            -Id "issue511_clone_signature_mismatch_diag" `
            -Source (New-Issue511CloneSignatureMismatchSource) `
            -ExpectedCodes @("E-TYP-2503")
    } catch {
        $failures.Add($_.Exception.Message) | Out-Null
    }

    try {
        Invoke-ExpectedDiagCodeCase `
            -Id "issue511_step_signature_mismatch_diag" `
            -Source (New-Issue511StepSignatureMismatchSource) `
            -ExpectedCodes @("E-TYP-2503")
    } catch {
        $failures.Add($_.Exception.Message) | Out-Null
    }

    try {
        Invoke-ExpectedDiagCodeCase `
            -Id "issue511_legacy_where_clause_rejected" `
            -Source (New-Issue511LegacyWhereConstraintRejectedSource) `
            -ExpectedCodes @("E-SRC-0520")
    } catch {
        $failures.Add($_.Exception.Message) | Out-Null
    }

    try {
        Invoke-ExpectedDiagCodeCase `
            -Id "issue511_legacy_where_loop_invariant_rejected" `
            -Source (New-Issue511LegacyWhereLoopInvariantRejectedSource) `
            -ExpectedCodes @("E-SRC-0520")
    } catch {
        $failures.Add($_.Exception.Message) | Out-Null
    }

    try {
        Invoke-ExpectedSuccessCase `
            -Id "issue511_user_assoc_projection_success" `
            -Source (New-Issue511UserAssocProjectionSuccessSource)
    } catch {
        $failures.Add($_.Exception.Message) | Out-Null
    }

    try {
        Invoke-ExpectedDiagCodeCase `
            -Id "issue511_user_assoc_projection_mismatch_diag" `
            -Source (New-Issue511UserAssocProjectionMismatchSource) `
            -ExpectedCodes @("E-TYP-2503")
    } catch {
        $failures.Add($_.Exception.Message) | Out-Null
    }

    try {
        Invoke-ExpectedSuccessCase `
            -Id "issue511_builtin_iterator_assoc_projection_success" `
            -Source (New-Issue511BuiltinIteratorAssocProjectionSuccessSource)
    } catch {
        $failures.Add($_.Exception.Message) | Out-Null
    }

    try {
        Invoke-ExpectedDiagCodeCase `
            -Id "issue511_builtin_iterator_assoc_projection_mismatch_diag" `
            -Source (New-Issue511BuiltinIteratorAssocProjectionMismatchSource) `
            -ExpectedCodes @("E-TYP-2503")
    } catch {
        $failures.Add($_.Exception.Message) | Out-Null
    }

    try {
        Invoke-ExpectedDiagCodeCaseWithForbiddenCodes `
            -Id "issue511_builtin_iterator_missing_assoc_binding_diag" `
            -Source (New-Issue511BuiltinIteratorMissingAssocBindingSource) `
            -ExpectedCodes @("E-TYP-2503") `
            -ForbiddenCodes @("E-UNS-0101")
    } catch {
        $failures.Add($_.Exception.Message) | Out-Null
    }

    if ($failures.Count -gt 0) {
        foreach ($failure in $failures) {
            Write-Host "[compiler-static] issue511 failure: $failure"
        }
        throw "Case 'issue511_foundational_classes_and_pipe_conformance' found $($failures.Count) conformance violation(s)."
    }

    Write-Host "[compiler-static] issue511_foundational_classes_and_pipe_conformance: foundational_classes=1 class_bounds=1 signature_enforcement=1 iterator_assoc_substitution=1 pipe_syntax=1 loop_forms=1"
}

function Invoke-Issue512InitializationPlanningConformanceCase {
    $failures = New-Object System.Collections.Generic.List[string]

    $specPath = $canonicalSpecPath
    if (-not (Test-Path $specPath)) {
        $failures.Add("Case 'issue512_initialization_planning_conformance' missing spec file: $specPath") | Out-Null
    } else {
        $specText = Get-Content -Path $specPath -Raw
        $requiredSpecPatterns = @(
            '\*\*\(ModulePrefix-Current\)\*\*',
            '\*\*\(TypeRef-Apply\)\*\*',
            '\*\*\(TypeRef-CallTypeArgs\)\*\*',
            '\*\*\(TypeRefsArgs-Empty\)\*\*',
            '\*\*\(TypeRefsArgs-Cons\)\*\*',
            '\*\*\(TypeRef-Range\)\*\*',
            '\*\*\(TypeRef-RangeInclusive\)\*\*',
            '\*\*\(TypeRef-RangeFrom\)\*\*',
            '\*\*\(TypeRef-RangeTo\)\*\*',
            '\*\*\(TypeRef-RangeToInclusive\)\*\*',
            '\*\*\(TypeRef-RangeFull\)\*\*',
            '\*\*\(TypeRef-Ref-Apply\)\*\*',
            '\*\*\(Topo-Cycle\)\*\*',
            '\|\s*`E-MOD-1401`\s*\|'
        )
        foreach ($pattern in $requiredSpecPatterns) {
            if (-not [System.Text.RegularExpressions.Regex]::IsMatch(
                $specText,
                $pattern,
                [System.Text.RegularExpressions.RegexOptions]::Multiline)) {
                $failures.Add("Case 'issue512_initialization_planning_conformance' missing expected spec rule or diagnostic pattern '$pattern' in canonical language spec.") | Out-Null
            }
        }
    }

    $initPlannerPath = Join-Path $workspaceRoot "cursive\\src\\04_analysis\\memory\\init_planner.cpp"
    if (-not (Test-Path $initPlannerPath)) {
        $failures.Add("Case 'issue512_initialization_planning_conformance' missing init planner file: $initPlannerPath") | Out-Null
    } else {
        $initPlannerText = Get-Content -Path $initPlannerPath -Raw
        $requiredPlannerPatterns = @(
            'SPEC_RULE\("ModulePrefix-Current"\)',
            'SPEC_DEF\("TypeRefsArgs", "5\.12"\)',
            'SPEC_RULE\("TypeRef-Apply"\)',
            'SPEC_RULE\("TypeRef-CallTypeArgs"\)',
            'SPEC_RULE\("TypeRefsArgs-Empty"\)',
            'SPEC_RULE\("TypeRefsArgs-Cons"\)',
            'SPEC_RULE\("TypeRef-Range"\)',
            'SPEC_RULE\("TypeRef-RangeInclusive"\)',
            'SPEC_RULE\("TypeRef-RangeFrom"\)',
            'SPEC_RULE\("TypeRef-RangeTo"\)',
            'SPEC_RULE\("TypeRef-RangeToInclusive"\)',
            'SPEC_RULE\("TypeRef-RangeFull"\)',
            'SPEC_RULE\("TypeRef-Ref-Apply"\)',
            'SPEC_RULE\("Topo-Cycle"\)'
        )
        foreach ($pattern in $requiredPlannerPatterns) {
            if ($initPlannerText -notmatch $pattern) {
                $failures.Add("Case 'issue512_initialization_planning_conformance' missing expected init-planner pattern '$pattern'.") | Out-Null
            }
        }
        if ($initPlannerText -match 'TypeRef-Ref-GenericPath') {
            $failures.Add("Case 'issue512_initialization_planning_conformance' found stale TypeRef-Ref-GenericPath rule in init planner.") | Out-Null
        }
    }

    $registryPath = Join-Path $workspaceRoot "cursive\\src\\00_core\\generated\\static_rule_registry.inc"
    if (-not (Test-Path $registryPath)) {
        $failures.Add("Case 'issue512_initialization_planning_conformance' missing static rule registry: $registryPath") | Out-Null
    } else {
        $registryText = Get-Content -Path $registryPath -Raw
        $requiredRegistryRules = @(
            'ModulePrefix-Current',
            'TypeRef-Apply',
            'TypeRef-CallTypeArgs',
            'TypeRefsArgs-Empty',
            'TypeRefsArgs-Cons',
            'TypeRef-Range',
            'TypeRef-RangeInclusive',
            'TypeRef-RangeFrom',
            'TypeRef-RangeTo',
            'TypeRef-RangeToInclusive',
            'TypeRef-RangeFull',
            'TypeRef-Ref-Apply',
            'Topo-Cycle'
        )
        foreach ($rule in $requiredRegistryRules) {
            if ($registryText -notmatch "\{`"$rule`",\s*`"DeclJudg`",\s*std::nullopt,\s*`"04_analysis/memory/init_planner\.cpp`"\}") {
                $failures.Add("Case 'issue512_initialization_planning_conformance' missing '$rule' in generated static rule registry.") | Out-Null
            }
        }
        if ($registryText -match '\{"TypeRef-Ref-GenericPath"') {
            $failures.Add("Case 'issue512_initialization_planning_conformance' found stale TypeRef-Ref-GenericPath entry in static rule registry.") | Out-Null
        }
    }

    $diagMapPath = Join-Path $workspaceRoot "cursive\\src\\04_analysis\\typing\\item\\typecheck_diag_map.inc"
    if (-not (Test-Path $diagMapPath)) {
        $failures.Add("Case 'issue512_initialization_planning_conformance' missing typecheck diagnostic map: $diagMapPath") | Out-Null
    } else {
        $diagMapText = Get-Content -Path $diagMapPath -Raw
        if ($diagMapText -notmatch '\{"Topo-Cycle",\s*"E-MOD-1401"\}') {
            $failures.Add("Case 'issue512_initialization_planning_conformance' missing Topo-Cycle -> E-MOD-1401 mapping in typecheck diagnostic map.") | Out-Null
        }
    }

    try {
        $currentAssemblyFiles = @{
            "consumer/Other.cursive" = (New-Issue512ModulePrefixCurrentConsumerSource)
            "sub/Support/Other.cursive" = (New-Issue512ModulePrefixCurrentSupportSource)
        }
        $modulePrefixCurrent = Invoke-CheckWithConformance `
            -CaseId "issue512_module_prefix_current" `
            -Source (New-Issue512ModulePrefixCurrentMainSource) `
            -ConformanceFileName "issue512_module_prefix_current.log" `
            -ExtraFiles $currentAssemblyFiles

        if ($modulePrefixCurrent.ExitCode -ne 0) {
            $failures.Add("Case 'issue512_module_prefix_current' expected exit 0 but got $($modulePrefixCurrent.ExitCode).") | Out-Null
        } else {
            $repoErrorCount = @($modulePrefixCurrent.DiagJson.diagnostics | Where-Object {
                $_.severity -eq "error" -or $_.severity -eq "panic"
            }).Count
            if ($repoErrorCount -ne 0) {
                $failures.Add("Case 'issue512_module_prefix_current' expected zero compile-time errors but observed $repoErrorCount.") | Out-Null
            }

            $repoLog = Get-Content -Path $modulePrefixCurrent.ConformancePath
            $resolveModulePathCount = @($repoLog | Where-Object { $_ -like "*`tResolve-ModulePath-Current`t*" }).Count
            $resolveModulePathErrCount = @($repoLog | Where-Object { $_ -like "*`tResolveModulePath-Err`t*" }).Count
            if ($resolveModulePathCount -lt 1 -or $resolveModulePathErrCount -ne 0) {
                $failures.Add("Case 'issue512_module_prefix_current' expected successful module-path resolution for the current-assembly prefix probe.") | Out-Null
            }
        }
    } catch {
        $failures.Add($_.Exception.Message) | Out-Null
    }

    try {
        $applyResult = Invoke-CheckWithConformance `
            -CaseId "issue512_typeref_apply_trace" `
            -Source (New-Issue512TypeRefApplySource) `
            -ConformanceFileName "issue512_typeref_apply_trace.log"

        if ($applyResult.ExitCode -ne 0) {
            $failures.Add("Case 'issue512_typeref_apply_trace' expected exit 0 but got $($applyResult.ExitCode).") | Out-Null
        } else {
            $applyErrors = @($applyResult.DiagJson.diagnostics | Where-Object {
                $_.severity -eq "error" -or $_.severity -eq "panic"
            }).Count
            if ($applyErrors -ne 0) {
                $failures.Add("Case 'issue512_typeref_apply_trace' expected zero compile-time errors but observed $applyErrors.") | Out-Null
            }
            $applyLog = Get-Content -Path $applyResult.ConformancePath
            $applyRuleCount = @($applyLog | Where-Object { $_ -like "*`tTypeRef-Apply`t*" }).Count
            if ($applyRuleCount -lt 1) {
                $failures.Add("Case 'issue512_typeref_apply_trace' expected TypeRef-Apply in conformance trace.") | Out-Null
            }
        }
    } catch {
        $failures.Add($_.Exception.Message) | Out-Null
    }

    try {
        $callResult = Invoke-CheckWithConformance `
            -CaseId "issue512_call_type_args_trace" `
            -Source (New-Issue512CallTypeArgsSource) `
            -ConformanceFileName "issue512_call_type_args_trace.log"

        if ($callResult.ExitCode -ne 0) {
            $failures.Add("Case 'issue512_call_type_args_trace' expected exit 0 but got $($callResult.ExitCode).") | Out-Null
        } else {
            $callErrors = @($callResult.DiagJson.diagnostics | Where-Object {
                $_.severity -eq "error" -or $_.severity -eq "panic"
            }).Count
            if ($callErrors -ne 0) {
                $failures.Add("Case 'issue512_call_type_args_trace' expected zero compile-time errors but observed $callErrors.") | Out-Null
            }
            $callLog = Get-Content -Path $callResult.ConformancePath
            $callTypeArgsCount = @($callLog | Where-Object { $_ -like "*`tTypeRef-CallTypeArgs`t*" }).Count
            $argsConsCount = @($callLog | Where-Object { $_ -like "*`tTypeRefsArgs-Cons`t*" }).Count
            $argsEmptyCount = @($callLog | Where-Object { $_ -like "*`tTypeRefsArgs-Empty`t*" }).Count
            if ($callTypeArgsCount -lt 2) {
                $failures.Add("Case 'issue512_call_type_args_trace' expected TypeRef-CallTypeArgs at least twice (non-empty and empty args).") | Out-Null
            }
            if ($argsConsCount -lt 1) {
                $failures.Add("Case 'issue512_call_type_args_trace' expected TypeRefsArgs-Cons in conformance trace.") | Out-Null
            }
            if ($argsEmptyCount -lt 1) {
                $failures.Add("Case 'issue512_call_type_args_trace' expected TypeRefsArgs-Empty in conformance trace.") | Out-Null
            }
        }
    } catch {
        $failures.Add($_.Exception.Message) | Out-Null
    }

    try {
        $rangeResult = Invoke-CheckWithConformance `
            -CaseId "issue512_range_family_trace" `
            -Source (New-Issue512RangeFamilySource) `
            -ConformanceFileName "issue512_range_family_trace.log"

        if ($rangeResult.ExitCode -ne 0) {
            $failures.Add("Case 'issue512_range_family_trace' expected exit 0 but got $($rangeResult.ExitCode).") | Out-Null
        } else {
            $rangeErrors = @($rangeResult.DiagJson.diagnostics | Where-Object {
                $_.severity -eq "error" -or $_.severity -eq "panic"
            }).Count
            if ($rangeErrors -ne 0) {
                $failures.Add("Case 'issue512_range_family_trace' expected zero compile-time errors but observed $rangeErrors.") | Out-Null
            }
            $rangeLog = Get-Content -Path $rangeResult.ConformancePath
            $rangeRules = @(
                "TypeRef-Range",
                "TypeRef-RangeInclusive",
                "TypeRef-RangeFrom",
                "TypeRef-RangeTo",
                "TypeRef-RangeToInclusive",
                "TypeRef-RangeFull"
            )
            foreach ($rule in $rangeRules) {
                $count = @($rangeLog | Where-Object { $_ -like "*`t$rule`t*" }).Count
                if ($count -lt 1) {
                    $failures.Add("Case 'issue512_range_family_trace' expected $rule in conformance trace.") | Out-Null
                }
            }
        }
    } catch {
        $failures.Add($_.Exception.Message) | Out-Null
    }

    try {
        $cycleResult = Invoke-CheckWithConformance `
            -CaseId "issue512_topo_cycle_diag" `
            -Source (New-Issue512TopoCycleMainSource) `
            -ConformanceFileName "issue512_topo_cycle_diag.log" `
            -ExtraFiles @{
                "a/A.cursive" = (New-Issue512TopoCycleModuleASource)
                "b/B.cursive" = (New-Issue512TopoCycleModuleBSource)
            }

        if ($cycleResult.ExitCode -ne 1) {
            $failures.Add("Case 'issue512_topo_cycle_diag' expected exit 1 but got $($cycleResult.ExitCode).") | Out-Null
        } else {
            $cycleDiagCount = @($cycleResult.DiagJson.diagnostics | Where-Object { $_.code -eq "E-MOD-1401" }).Count
            if ($cycleDiagCount -lt 1) {
                $failures.Add("Case 'issue512_topo_cycle_diag' expected E-MOD-1401 diagnostic.") | Out-Null
            }
            $cycleLog = Get-Content -Path $cycleResult.ConformancePath
            $cycleRuleCount = @($cycleLog | Where-Object { $_ -like "*`tTopo-Cycle`t*" }).Count
            if ($cycleRuleCount -lt 1) {
                $failures.Add("Case 'issue512_topo_cycle_diag' expected Topo-Cycle in conformance trace.") | Out-Null
            }
        }
    } catch {
        $failures.Add($_.Exception.Message) | Out-Null
    }

    if ($failures.Count -gt 0) {
        foreach ($failure in $failures) {
            Write-Host "[compiler-static] issue512 failure: $failure"
        }
        throw "Case 'issue512_initialization_planning_conformance' found $($failures.Count) conformance violation(s)."
    }

    Write-Host "[compiler-static] issue512_initialization_planning_conformance: spec=1 wiring=1 registry=1 module_prefix_probe=1 type_apply=1 call_type_args=1 range_family=1 topo_cycle=1"
}

function Invoke-Issue545ResolveModulePathDirectConformanceCase {
    $specText = Get-Content -Path $canonicalSpecPath -Raw
    if ($specText -notmatch '\*\*\(Resolve-ModulePath-Direct\)\*\*') {
        throw "Case 'issue545_resolve_module_path_direct_conformance' missing Resolve-ModulePath-Direct in canonical language spec."
    }

    $scopesLookupPath = Join-Path $workspaceRoot "cursive\\src\\04_analysis\\resolve\\scopes_lookup.cpp"
    if (-not (Test-Path $scopesLookupPath)) {
        throw "Case 'issue545_resolve_module_path_direct_conformance' missing resolver source file: $scopesLookupPath"
    }
    $scopesLookupText = Get-Content -Path $scopesLookupPath -Raw
    if ($scopesLookupText -notmatch 'SPEC_RULE\("Resolve-ModulePath-Direct"\)') {
        throw "Case 'issue545_resolve_module_path_direct_conformance' missing Resolve-ModulePath-Direct trace in scopes_lookup.cpp."
    }

    $registryPath = Join-Path $workspaceRoot "cursive\\src\\00_core\\generated\\static_rule_registry.inc"
    if (-not (Test-Path $registryPath)) {
        throw "Case 'issue545_resolve_module_path_direct_conformance' missing static rule registry: $registryPath"
    }
    $registryText = Get-Content -Path $registryPath -Raw
    if ($registryText -notmatch '\{"Resolve-ModulePath-Direct",\s*"ResolvePathJudg",\s*std::nullopt,\s*"04_analysis/resolve/scopes_lookup\.cpp"\}') {
        throw "Case 'issue545_resolve_module_path_direct_conformance' missing Resolve-ModulePath-Direct entry in generated static rule registry."
    }

    $result = Invoke-CheckWithConformance `
        -CaseId "issue545_resolve_module_path_direct_conformance" `
        -Source (New-Issue545ResolveModulePathDirectMainSource) `
        -ConformanceFileName "issue545_resolve_module_path_direct_conformance.log" `
        -ExtraFiles @{
            "consumer/Other.cursive" = (New-Issue545ResolveModulePathDirectConsumerSource)
            "sub/Support/Other.cursive" = (New-Issue545ResolveModulePathDirectSupportSource)
        }

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue545_resolve_module_path_direct_conformance' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue545_resolve_module_path_direct_conformance' expected zero compile-time errors, observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $directCount = @($logLines | Where-Object { $_ -like "*`tResolve-ModulePath-Direct`t*" }).Count
    $errCount = @($logLines | Where-Object { $_ -like "*`tResolveModulePath-Err`t*" }).Count
    if ($directCount -lt 1 -or $errCount -ne 0) {
        throw "Case 'issue545_resolve_module_path_direct_conformance' expected successful direct module-path resolution in conformance trace."
    }

    Write-Host "[compiler-static] issue545_resolve_module_path_direct_conformance: exit=$($result.ExitCode) errors=$errorCount direct=$directCount"
}

function Invoke-Issue546ImportPathAndCoverageConformanceCase {
    $specText = Get-Content -Path $canonicalSpecPath -Raw
    $requiredSpecRules = @(
        '\*\*\(Import-Ok-Local\)\*\*',
        '\*\*\(Import-Ok-Covered\)\*\*',
        '\*\*\(Import-Ok-Err\)\*\*',
        '\*\*\(Resolve-Import-Direct\)\*\*',
        '\*\*\(Resolve-Import-Current\)\*\*',
        '\*\*\(Resolve-Import-Err\)\*\*'
    )
    foreach ($pattern in $requiredSpecRules) {
        if ($specText -notmatch $pattern) {
            throw "Case 'issue546_import_path_and_coverage_conformance' missing expected canonical rule pattern '$pattern' in the language spec."
        }
    }

    $collectPath = Join-Path $workspaceRoot "cursive\\src\\04_analysis\\resolve\\collect_toplevel.cpp"
    $modulePathsPath = Join-Path $workspaceRoot "cursive\\src\\02_source\\module_paths.cpp"
    if (-not (Test-Path $collectPath)) {
        throw "Case 'issue546_import_path_and_coverage_conformance' missing collect_toplevel.cpp: $collectPath"
    }
    if (-not (Test-Path $modulePathsPath)) {
        throw "Case 'issue546_import_path_and_coverage_conformance' missing module_paths.cpp: $modulePathsPath"
    }
    $collectText = Get-Content -Path $collectPath -Raw
    $modulePathsText = Get-Content -Path $modulePathsPath -Raw
    $requiredAnchors = @(
        @{
            Path = "collect_toplevel.cpp"
            Text = $collectText
            Pattern = 'SPEC_RULE\("Import-Ok-Local"\)'
        },
        @{
            Path = "collect_toplevel.cpp"
            Text = $collectText
            Pattern = 'SPEC_RULE\("Import-Ok-Covered"\)'
        },
        @{
            Path = "collect_toplevel.cpp"
            Text = $collectText
            Pattern = 'SPEC_RULE\("Import-Ok-Err"\)'
        },
        @{
            Path = "module_paths.cpp"
            Text = $modulePathsText
            Pattern = 'SPEC_RULE\("Resolve-Import-Direct"\)'
        },
        @{
            Path = "module_paths.cpp"
            Text = $modulePathsText
            Pattern = 'SPEC_RULE\("Resolve-Import-Current"\)'
        },
        @{
            Path = "module_paths.cpp"
            Text = $modulePathsText
            Pattern = 'SPEC_RULE\("Resolve-Import-Err"\)'
        }
    )
    foreach ($entry in $requiredAnchors) {
        if ($entry.Text -notmatch $entry.Pattern) {
            throw "Case 'issue546_import_path_and_coverage_conformance' missing expected trace anchor '$($entry.Pattern)' in $($entry.Path)."
        }
    }

    $registryPath = Join-Path $workspaceRoot "cursive\\src\\00_core\\generated\\static_rule_registry.inc"
    if (-not (Test-Path $registryPath)) {
        throw "Case 'issue546_import_path_and_coverage_conformance' missing static rule registry: $registryPath"
    }
    $registryText = Get-Content -Path $registryPath -Raw
    $requiredRegistryPatterns = @(
        '\{"Resolve-Import-Direct",\s*"ParseJudgment"',
        '\{"Resolve-Import-Current",\s*"ParseJudgment"',
        '\{"Resolve-Import-Err",\s*"ParseJudgment"',
        '\{"Import-Ok-Local",\s*"ResolvePathJudg"',
        '\{"Import-Ok-Covered",\s*"ResolvePathJudg"',
        '\{"Import-Ok-Err",\s*"ResolvePathJudg"'
    )
    foreach ($pattern in $requiredRegistryPatterns) {
        if ($registryText -notmatch $pattern) {
            throw "Case 'issue546_import_path_and_coverage_conformance' missing registry entry '$pattern'."
        }
    }

    $lookupPath = Join-Path $workspaceRoot "cursive\\src\\04_analysis\\resolve\\scopes_lookup.cpp"
    if (-not (Test-Path $lookupPath)) {
        throw "Case 'issue546_import_path_and_coverage_conformance' missing scopes_lookup.cpp: $lookupPath"
    }
    $lookupText = Get-Content -Path $lookupPath -Raw
    $requiredLookupPatterns = @(
        'source::ModuleNames VisibleModuleNamesOf\(',
        'source::ResolveImportModulePath\(\s*CurrentModule\(ctx\),\s*all_module_names,\s*import_decl->path\s*\)'
    )
    foreach ($pattern in $requiredLookupPatterns) {
        if ($lookupText -notmatch $pattern) {
            throw "Case 'issue546_import_path_and_coverage_conformance' missing expected visibility pattern '$pattern' in scopes_lookup.cpp."
        }
    }

    $driverPath = Join-Path $workspaceRoot "cursive\\src\\06_driver\\main.cpp"
    if (-not (Test-Path $driverPath)) {
        throw "Case 'issue546_import_path_and_coverage_conformance' missing main.cpp: $driverPath"
    }
    $driverText = Get-Content -Path $driverPath -Raw
    foreach ($pattern in @(
        'ResolveUsingAssemblyName\(',
        'std::get_if<ast::UsingDecl>\(&item\)'
    )) {
        if ($driverText -notmatch $pattern) {
            throw "Case 'issue546_import_path_and_coverage_conformance' missing expected parse-closure pattern '$pattern' in main.cpp."
        }
    }

    $currentLocal = Invoke-CheckWithConformance `
        -CaseId "issue546_resolve_import_current_local" `
        -Source (New-Issue546ResolveImportCurrentLocalMainSource) `
        -ConformanceFileName "issue546_resolve_import_current_local.log" `
        -ExtraFiles @{
            "consumer/Other.cursive" = (New-Issue546ResolveImportCurrentLocalConsumerSource)
            "sub/Support/Other.cursive" = (New-Issue546ResolveImportCurrentLocalSupportSource)
        }

    if ($currentLocal.ExitCode -ne 0) {
        throw "Case 'issue546_resolve_import_current_local' expected exit 0 but got $($currentLocal.ExitCode)."
    }
    $currentLocalErrors = @($currentLocal.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($currentLocalErrors -ne 0) {
        throw "Case 'issue546_resolve_import_current_local' expected zero compile-time errors but observed $currentLocalErrors."
    }
    $currentLocalLog = Get-Content -Path $currentLocal.ConformancePath
    $currentCount = @($currentLocalLog | Where-Object { $_ -like "*`tResolve-Import-Current`t*" }).Count
    $localOkCount = @($currentLocalLog | Where-Object { $_ -like "*`tImport-Ok-Local`t*" }).Count
    if ($currentCount -lt 1 -or $localOkCount -lt 1) {
        throw "Case 'issue546_resolve_import_current_local' expected Resolve-Import-Current and Import-Ok-Local in the conformance trace."
    }

    $multiAssemblyManifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""executable""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "",
        "[[assembly]]",
        "name = ""dep""",
        "kind = ""dependency""",
        "root = ""dep""",
        "out_dir = ""build/dep"""
    )

    $covered = Invoke-CheckWithConformance `
        -CaseId "issue546_import_covered" `
        -Source (New-Issue546ImportCoveredMainSource) `
        -Manifest $multiAssemblyManifest `
        -ConformanceFileName "issue546_import_covered.log" `
        -ExtraFiles @{
            "dep/Support/Other.cursive" = (New-Issue546DepSupportSource)
        }

    if ($covered.ExitCode -ne 0) {
        throw "Case 'issue546_import_covered' expected exit 0 but got $($covered.ExitCode)."
    }
    $coveredErrors = @($covered.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($coveredErrors -ne 0) {
        throw "Case 'issue546_import_covered' expected zero compile-time errors but observed $coveredErrors."
    }
    $coveredLog = Get-Content -Path $covered.ConformancePath
    $directCount = @($coveredLog | Where-Object { $_ -like "*`tResolve-Import-Direct`t*" }).Count
    $coveredOkCount = @($coveredLog | Where-Object { $_ -like "*`tImport-Ok-Covered`t*" }).Count
    if ($directCount -lt 1 -or $coveredOkCount -lt 1) {
        throw "Case 'issue546_import_covered' expected Resolve-Import-Direct and Import-Ok-Covered in the conformance trace."
    }

    $missingImport = Invoke-CheckWithConformance `
        -CaseId "issue546_import_missing" `
        -Source (New-Issue546ImportMissingMainSource) `
        -Manifest $multiAssemblyManifest `
        -ConformanceFileName "issue546_import_missing.log" `
        -ExtraFiles @{
            "dep/Support/Other.cursive" = (New-Issue546DepSupportSource)
        }

    if ($missingImport.ExitCode -ne 1) {
        throw "Case 'issue546_import_missing' expected exit 1 but got $($missingImport.ExitCode)."
    }
    $missingImportDiag = @($missingImport.DiagJson.diagnostics | Where-Object {
        $_.code -eq "E-MOD-1201"
    }).Count
    if ($missingImportDiag -lt 1) {
        throw "Case 'issue546_import_missing' expected E-MOD-1201."
    }
    $missingImportWrongDiag = @($missingImport.DiagJson.diagnostics | Where-Object {
        $_.code -eq "E-MOD-1204"
    }).Count
    if ($missingImportWrongDiag -gt 0) {
        throw "Case 'issue546_import_missing' must not regress to E-MOD-1204."
    }
    $missingImportLog = Get-Content -Path $missingImport.ConformancePath
    $missingImportCount = @($missingImportLog | Where-Object { $_ -like "*`tImport-Ok-Err`t*" }).Count
    if ($missingImportCount -lt 1) {
        throw "Case 'issue546_import_missing' expected Import-Ok-Err in the conformance trace."
    }

    $resolveErr = Invoke-CheckWithConformance `
        -CaseId "issue546_resolve_import_err" `
        -Source (New-Issue546ResolveImportErrMainSource) `
        -ConformanceFileName "issue546_resolve_import_err.log"

    if ($resolveErr.ExitCode -ne 1) {
        throw "Case 'issue546_resolve_import_err' expected exit 1 but got $($resolveErr.ExitCode)."
    }
    $resolveErrDiag = @($resolveErr.DiagJson.diagnostics | Where-Object {
        $_.code -eq "E-MOD-1202"
    }).Count
    if ($resolveErrDiag -lt 1) {
        throw "Case 'issue546_resolve_import_err' expected E-MOD-1202."
    }
    $resolveErrAssemblyGraphDiag = @($resolveErr.DiagJson.diagnostics | Where-Object {
        $_.code -eq "E-PRJ-0209"
    }).Count
    if ($resolveErrAssemblyGraphDiag -gt 0) {
        throw "Case 'issue546_resolve_import_err' must not surface E-PRJ-0209 for an unresolved import."
    }
    $resolveErrLog = Get-Content -Path $resolveErr.ConformancePath
    $resolveErrCount = @($resolveErrLog | Where-Object { $_ -like "*`tResolve-Import-Err`t*" }).Count
    if ($resolveErrCount -lt 1) {
        throw "Case 'issue546_resolve_import_err' expected Resolve-Import-Err in the conformance trace."
    }

    $transitiveManifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""executable""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "",
        "[[assembly]]",
        "name = ""liba""",
        "kind = ""library""",
        "root = ""liba""",
        "out_dir = ""build/liba""",
        "",
        "[[assembly]]",
        "name = ""libb""",
        "kind = ""library""",
        "root = ""libb""",
        "out_dir = ""build/libb"""
    )

    $transitive = Invoke-CheckWithConformance `
        -CaseId "issue546_transitive_visibility_rejected" `
        -Source (New-Issue546TransitiveVisibilityMainSource) `
        -Manifest $transitiveManifest `
        -ConformanceFileName "issue546_transitive_visibility_rejected.log" `
        -ExtraArgs @("--assembly", "probe") `
        -ExtraFiles @{
            "liba/LibraryA.cursive" = (New-Issue546TransitiveVisibilityLibASource)
            "libb/LibraryB.cursive" = (New-Issue546TransitiveVisibilityLibBSource)
        }

    if ($transitive.ExitCode -ne 1) {
        throw "Case 'issue546_transitive_visibility_rejected' expected exit 1 but got $($transitive.ExitCode)."
    }
    $transitiveDiag = @($transitive.DiagJson.diagnostics | Where-Object {
        $_.code -eq "E-MOD-1301"
    }).Count
    if ($transitiveDiag -lt 1) {
        throw "Case 'issue546_transitive_visibility_rejected' expected E-MOD-1301."
    }
    $transitiveLog = Get-Content -Path $transitive.ConformancePath
    $transitiveResolveErr = @($transitiveLog | Where-Object { $_ -like "*`tResolveModulePath-Err`t*" }).Count
    if ($transitiveResolveErr -lt 1) {
        throw "Case 'issue546_transitive_visibility_rejected' expected ResolveModulePath-Err in the conformance trace."
    }

    Write-Host "[compiler-static] issue546_import_path_and_coverage_conformance: current=$currentCount local_ok=$localOkCount direct=$directCount covered_ok=$coveredOkCount missing_import=$missingImportDiag resolve_err_diag=$resolveErrDiag transitive_diag=$transitiveDiag"
}

function Invoke-Issue515SystemGetEnvConformanceCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue515_system_get_env_conformance" `
        -Source (New-Issue515SystemGetEnvSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue515_system_get_env_conformance.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue515_system_get_env_conformance' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue515_system_get_env_conformance' expected zero diagnostics, observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $builtinCount = @($logLines | Where-Object { $_ -like "*`tBuiltinSym-System-GetEnv`t*" }).Count
    if ($builtinCount -lt 1) {
        throw "Case 'issue515_system_get_env_conformance' expected BuiltinSym-System-GetEnv in conformance trace."
    }

    Write-Host "[compiler-static] issue515_system_get_env_conformance: exit=$($result.ExitCode) errors=$errorCount builtin_system_get_env=$builtinCount"
}

function Invoke-Issue516SystemExitConformanceCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue516_system_exit_conformance" `
        -Source (New-Issue516SystemExitSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue516_system_exit_conformance.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue516_system_exit_conformance' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue516_system_exit_conformance' expected zero diagnostics, observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $builtinCount = @($logLines | Where-Object { $_ -like "*`tBuiltinSym-System-Exit`t*" }).Count
    if ($builtinCount -lt 1) {
        throw "Case 'issue516_system_exit_conformance' expected BuiltinSym-System-Exit in conformance trace."
    }

    Write-Host "[compiler-static] issue516_system_exit_conformance: exit=$($result.ExitCode) errors=$errorCount builtin_system_exit=$builtinCount"
}

function Invoke-Issue517SystemRunConformanceCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue517_system_run_conformance" `
        -Source (New-Issue517SystemRunSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue517_system_run_conformance.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue517_system_run_conformance' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue517_system_run_conformance' expected zero diagnostics, observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $builtinCount = @($logLines | Where-Object { $_ -like "*`tBuiltinSym-System-Run`t*" }).Count
    if ($builtinCount -lt 1) {
        throw "Case 'issue517_system_run_conformance' expected BuiltinSym-System-Run in conformance trace."
    }

    Write-Host "[compiler-static] issue517_system_run_conformance: exit=$($result.ExitCode) errors=$errorCount builtin_system_run=$builtinCount"
}

function Invoke-Issue518FileSystemOpenReadConformanceCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue518_file_system_open_read_conformance" `
        -Source (New-Issue518FileSystemOpenReadSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue518_file_system_open_read_conformance.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue518_file_system_open_read_conformance' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue518_file_system_open_read_conformance' expected zero diagnostics, observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $builtinCount = @($logLines | Where-Object { $_ -like "*`tBuiltinSym-FileSystem-OpenRead`t*" }).Count
    if ($builtinCount -lt 1) {
        throw "Case 'issue518_file_system_open_read_conformance' expected BuiltinSym-FileSystem-OpenRead in conformance trace."
    }

    Write-Host "[compiler-static] issue518_file_system_open_read_conformance: exit=$($result.ExitCode) errors=$errorCount builtin_file_system_open_read=$builtinCount"
}

function Invoke-Issue519FileSystemOpenWriteConformanceCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue519_file_system_open_write_conformance" `
        -Source (New-Issue519FileSystemOpenWriteSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue519_file_system_open_write_conformance.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue519_file_system_open_write_conformance' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue519_file_system_open_write_conformance' expected zero diagnostics, observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $builtinCount = @($logLines | Where-Object { $_ -like "*`tBuiltinSym-FileSystem-OpenWrite`t*" }).Count
    if ($builtinCount -lt 1) {
        throw "Case 'issue519_file_system_open_write_conformance' expected BuiltinSym-FileSystem-OpenWrite in conformance trace."
    }

    Write-Host "[compiler-static] issue519_file_system_open_write_conformance: exit=$($result.ExitCode) errors=$errorCount builtin_file_system_open_write=$builtinCount"
}

function Invoke-Issue521FileSystemCreateWriteConformanceCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue521_file_system_create_write_conformance" `
        -Source (New-Issue521FileSystemCreateWriteSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue521_file_system_create_write_conformance.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue521_file_system_create_write_conformance' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue521_file_system_create_write_conformance' expected zero diagnostics, observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $builtinCount = @($logLines | Where-Object { $_ -like "*`tBuiltinSym-FileSystem-CreateWrite`t*" }).Count
    if ($builtinCount -lt 1) {
        throw "Case 'issue521_file_system_create_write_conformance' expected BuiltinSym-FileSystem-CreateWrite in conformance trace."
    }

    Write-Host "[compiler-static] issue521_file_system_create_write_conformance: exit=$($result.ExitCode) errors=$errorCount builtin_file_system_create_write=$builtinCount"
}

function Invoke-Issue522FileSystemReadFileConformanceCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue522_file_system_read_file_conformance" `
        -Source (New-Issue522FileSystemReadFileSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue522_file_system_read_file_conformance.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue522_file_system_read_file_conformance' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue522_file_system_read_file_conformance' expected zero diagnostics, observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $builtinCount = @($logLines | Where-Object { $_ -like "*`tBuiltinSym-FileSystem-ReadFile`t*" }).Count
    if ($builtinCount -lt 1) {
        throw "Case 'issue522_file_system_read_file_conformance' expected BuiltinSym-FileSystem-ReadFile in conformance trace."
    }

    Write-Host "[compiler-static] issue522_file_system_read_file_conformance: exit=$($result.ExitCode) errors=$errorCount builtin_file_system_read_file=$builtinCount"
}

function Invoke-Issue523FileSystemReadBytesConformanceCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue523_file_system_read_bytes_conformance" `
        -Source (New-Issue523FileSystemReadBytesSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue523_file_system_read_bytes_conformance.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue523_file_system_read_bytes_conformance' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue523_file_system_read_bytes_conformance' expected zero diagnostics, observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $builtinCount = @($logLines | Where-Object { $_ -like "*`tBuiltinSym-FileSystem-ReadBytes`t*" }).Count
    if ($builtinCount -lt 1) {
        throw "Case 'issue523_file_system_read_bytes_conformance' expected BuiltinSym-FileSystem-ReadBytes in conformance trace."
    }

    Write-Host "[compiler-static] issue523_file_system_read_bytes_conformance: exit=$($result.ExitCode) errors=$errorCount builtin_file_system_read_bytes=$builtinCount"
}

function Invoke-Issue524FileSystemWriteFileConformanceCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue524_file_system_write_file_conformance" `
        -Source (New-Issue524FileSystemWriteFileSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue524_file_system_write_file_conformance.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue524_file_system_write_file_conformance' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue524_file_system_write_file_conformance' expected zero diagnostics, observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $builtinCount = @($logLines | Where-Object { $_ -like "*`tBuiltinSym-FileSystem-WriteFile`t*" }).Count
    if ($builtinCount -lt 1) {
        throw "Case 'issue524_file_system_write_file_conformance' expected BuiltinSym-FileSystem-WriteFile in conformance trace."
    }

    Write-Host "[compiler-static] issue524_file_system_write_file_conformance: exit=$($result.ExitCode) errors=$errorCount builtin_file_system_write_file=$builtinCount"
}

function Invoke-Issue525FileSystemWriteStdoutConformanceCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue525_file_system_write_stdout_conformance" `
        -Source (New-Issue525FileSystemWriteStdoutSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue525_file_system_write_stdout_conformance.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue525_file_system_write_stdout_conformance' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue525_file_system_write_stdout_conformance' expected zero diagnostics, observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $builtinCount = @($logLines | Where-Object { $_ -like "*`tBuiltinSym-FileSystem-WriteStdout`t*" }).Count
    if ($builtinCount -lt 1) {
        throw "Case 'issue525_file_system_write_stdout_conformance' expected BuiltinSym-FileSystem-WriteStdout in conformance trace."
    }

    Write-Host "[compiler-static] issue525_file_system_write_stdout_conformance: exit=$($result.ExitCode) errors=$errorCount builtin_file_system_write_stdout=$builtinCount"
}

function Invoke-Issue526FileSystemWriteStderrConformanceCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue526_file_system_write_stderr_conformance" `
        -Source (New-Issue526FileSystemWriteStderrSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue526_file_system_write_stderr_conformance.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue526_file_system_write_stderr_conformance' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue526_file_system_write_stderr_conformance' expected zero diagnostics, observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $builtinCount = @($logLines | Where-Object { $_ -like "*`tBuiltinSym-FileSystem-WriteStderr`t*" }).Count
    if ($builtinCount -lt 1) {
        throw "Case 'issue526_file_system_write_stderr_conformance' expected BuiltinSym-FileSystem-WriteStderr in conformance trace."
    }

    Write-Host "[compiler-static] issue526_file_system_write_stderr_conformance: exit=$($result.ExitCode) errors=$errorCount builtin_file_system_write_stderr=$builtinCount"
}

function Invoke-Issue527FileSystemExistsConformanceCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue527_file_system_exists_conformance" `
        -Source (New-Issue527FileSystemExistsSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue527_file_system_exists_conformance.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue527_file_system_exists_conformance' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue527_file_system_exists_conformance' expected zero diagnostics, observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $builtinCount = @($logLines | Where-Object { $_ -like "*`tBuiltinSym-FileSystem-Exists`t*" }).Count
    if ($builtinCount -lt 1) {
        throw "Case 'issue527_file_system_exists_conformance' expected BuiltinSym-FileSystem-Exists in conformance trace."
    }

    Write-Host "[compiler-static] issue527_file_system_exists_conformance: exit=$($result.ExitCode) errors=$errorCount builtin_file_system_exists=$builtinCount"
}

function Invoke-Issue528FileSystemRemoveConformanceCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue528_file_system_remove_conformance" `
        -Source (New-Issue528FileSystemRemoveSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue528_file_system_remove_conformance.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue528_file_system_remove_conformance' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue528_file_system_remove_conformance' expected zero diagnostics, observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $builtinCount = @($logLines | Where-Object { $_ -like "*`tBuiltinSym-FileSystem-Remove`t*" }).Count
    if ($builtinCount -lt 1) {
        throw "Case 'issue528_file_system_remove_conformance' expected BuiltinSym-FileSystem-Remove in conformance trace."
    }

    Write-Host "[compiler-static] issue528_file_system_remove_conformance: exit=$($result.ExitCode) errors=$errorCount builtin_file_system_remove=$builtinCount"
}

function Invoke-Issue529FileSystemOpenDirConformanceCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue529_file_system_open_dir_conformance" `
        -Source (New-Issue529FileSystemOpenDirSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue529_file_system_open_dir_conformance.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue529_file_system_open_dir_conformance' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue529_file_system_open_dir_conformance' expected zero diagnostics, observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $builtinCount = @($logLines | Where-Object { $_ -like "*`tBuiltinSym-FileSystem-OpenDir`t*" }).Count
    if ($builtinCount -lt 1) {
        throw "Case 'issue529_file_system_open_dir_conformance' expected BuiltinSym-FileSystem-OpenDir in conformance trace."
    }

    Write-Host "[compiler-static] issue529_file_system_open_dir_conformance: exit=$($result.ExitCode) errors=$errorCount builtin_file_system_open_dir=$builtinCount"
}

function Invoke-Issue530FileSystemCreateDirConformanceCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue530_file_system_create_dir_conformance" `
        -Source (New-Issue530FileSystemCreateDirSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue530_file_system_create_dir_conformance.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue530_file_system_create_dir_conformance' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue530_file_system_create_dir_conformance' expected zero diagnostics, observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $builtinCount = @($logLines | Where-Object { $_ -like "*`tBuiltinSym-FileSystem-CreateDir`t*" }).Count
    if ($builtinCount -lt 1) {
        throw "Case 'issue530_file_system_create_dir_conformance' expected BuiltinSym-FileSystem-CreateDir in conformance trace."
    }

    Write-Host "[compiler-static] issue530_file_system_create_dir_conformance: exit=$($result.ExitCode) errors=$errorCount builtin_file_system_create_dir=$builtinCount"
}

function Invoke-Issue531FileSystemEnsureDirConformanceCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue531_file_system_ensure_dir_conformance" `
        -Source (New-Issue531FileSystemEnsureDirSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue531_file_system_ensure_dir_conformance.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue531_file_system_ensure_dir_conformance' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue531_file_system_ensure_dir_conformance' expected zero diagnostics, observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $builtinCount = @($logLines | Where-Object { $_ -like "*`tBuiltinSym-FileSystem-EnsureDir`t*" }).Count
    if ($builtinCount -lt 1) {
        throw "Case 'issue531_file_system_ensure_dir_conformance' expected BuiltinSym-FileSystem-EnsureDir in conformance trace."
    }

    Write-Host "[compiler-static] issue531_file_system_ensure_dir_conformance: exit=$($result.ExitCode) errors=$errorCount builtin_file_system_ensure_dir=$builtinCount"
}

function Invoke-Issue532FileSystemKindConformanceCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue532_file_system_kind_conformance" `
        -Source (New-Issue532FileSystemKindSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue532_file_system_kind_conformance.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue532_file_system_kind_conformance' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue532_file_system_kind_conformance' expected zero diagnostics, observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $builtinCount = @($logLines | Where-Object { $_ -like "*`tBuiltinSym-FileSystem-Kind`t*" }).Count
    if ($builtinCount -lt 1) {
        throw "Case 'issue532_file_system_kind_conformance' expected BuiltinSym-FileSystem-Kind in conformance trace."
    }

    Write-Host "[compiler-static] issue532_file_system_kind_conformance: exit=$($result.ExitCode) errors=$errorCount builtin_file_system_kind=$builtinCount"
}

function Invoke-Issue533FileSystemRestrictConformanceCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue533_file_system_restrict_conformance" `
        -Source (New-Issue533FileSystemRestrictSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue533_file_system_restrict_conformance.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue533_file_system_restrict_conformance' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue533_file_system_restrict_conformance' expected zero diagnostics, observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $builtinCount = @($logLines | Where-Object { $_ -like "*`tBuiltinSym-FileSystem-Restrict`t*" }).Count
    if ($builtinCount -lt 2) {
        throw "Case 'issue533_file_system_restrict_conformance' expected BuiltinSym-FileSystem-Restrict at least twice in conformance trace."
    }

    Write-Host "[compiler-static] issue533_file_system_restrict_conformance: exit=$($result.ExitCode) errors=$errorCount builtin_file_system_restrict=$builtinCount"
}

function Invoke-Issue534FileModalReadConformanceCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue534_file_modal_read_conformance" `
        -Source (New-Issue534FileModalReadConformanceSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue534_file_modal_read_conformance.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue534_file_modal_read_conformance' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue534_file_modal_read_conformance' expected zero diagnostics, observed $errorCount."
    }

    $llText = Get-EmittedLlvmIrText -CaseId "issue534_file_modal_read_conformance" -CaseRoot $result.CaseRoot
    foreach ($requiredSymbol in @(
        "File_x3a_x3aRead_x3a_x3aread_x5fall",
        "File_x3a_x3aRead_x3a_x3aread_x5fall_x5fbytes",
        "File_x3a_x3aRead_x3a_x3aclose"
    )) {
        if (-not $llText.Contains($requiredSymbol)) {
            throw "Case 'issue534_file_modal_read_conformance' missing runtime symbol '$requiredSymbol' in emitted LLVM IR."
        }
    }

    Write-Host "[compiler-static] issue534_file_modal_read_conformance: exit=$($result.ExitCode) errors=$errorCount"
}

function Invoke-Issue535FileModalWriteConformanceCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue535_file_modal_write_conformance" `
        -Source (New-Issue535FileModalWriteConformanceSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue535_file_modal_write_conformance.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue535_file_modal_write_conformance' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue535_file_modal_write_conformance' expected zero diagnostics, observed $errorCount."
    }

    $llText = Get-EmittedLlvmIrText -CaseId "issue535_file_modal_write_conformance" -CaseRoot $result.CaseRoot
    foreach ($requiredSymbol in @(
        "File_x3a_x3aWrite_x3a_x3awrite",
        "File_x3a_x3aWrite_x3a_x3aflush",
        "File_x3a_x3aWrite_x3a_x3aclose"
    )) {
        if (-not $llText.Contains($requiredSymbol)) {
            throw "Case 'issue535_file_modal_write_conformance' missing runtime symbol '$requiredSymbol' in emitted LLVM IR."
        }
    }

    Write-Host "[compiler-static] issue535_file_modal_write_conformance: exit=$($result.ExitCode) errors=$errorCount"
}

function Invoke-Issue536FileModalAppendConformanceCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue536_file_modal_append_conformance" `
        -Source (New-Issue536FileModalAppendConformanceSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue536_file_modal_append_conformance.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue536_file_modal_append_conformance' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue536_file_modal_append_conformance' expected zero diagnostics, observed $errorCount."
    }

    $llText = Get-EmittedLlvmIrText -CaseId "issue536_file_modal_append_conformance" -CaseRoot $result.CaseRoot
    foreach ($requiredSymbol in @(
        "File_x3a_x3aAppend_x3a_x3awrite",
        "File_x3a_x3aAppend_x3a_x3aflush",
        "File_x3a_x3aAppend_x3a_x3aclose"
    )) {
        if (-not $llText.Contains($requiredSymbol)) {
            throw "Case 'issue536_file_modal_append_conformance' missing runtime symbol '$requiredSymbol' in emitted LLVM IR."
        }
    }

    Write-Host "[compiler-static] issue536_file_modal_append_conformance: exit=$($result.ExitCode) errors=$errorCount"
}

function Invoke-Issue537DirIterPrimitiveConformanceCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue537_dir_iter_primitive_conformance" `
        -Source (New-Issue537DirIterPrimitiveConformanceSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue537_dir_iter_primitive_conformance.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue537_dir_iter_primitive_conformance' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue537_dir_iter_primitive_conformance' expected zero diagnostics, observed $errorCount."
    }

    $llText = Get-EmittedLlvmIrText -CaseId "issue537_dir_iter_primitive_conformance" -CaseRoot $result.CaseRoot
    foreach ($requiredSymbol in @(
        "DirIter_x3a_x3aOpen_x3a_x3anext",
        "DirIter_x3a_x3aOpen_x3a_x3aclose"
    )) {
        if (-not $llText.Contains($requiredSymbol)) {
            throw "Case 'issue537_dir_iter_primitive_conformance' missing runtime symbol '$requiredSymbol' in emitted LLVM IR."
        }
    }

    Write-Host "[compiler-static] issue537_dir_iter_primitive_conformance: exit=$($result.ExitCode) errors=$errorCount"
}

function Invoke-Issue538NetworkRestrictHostConformanceCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue538_network_restrict_host_conformance" `
        -Source (New-Issue538NetworkRestrictHostConformanceSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue538_network_restrict_host_conformance.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue538_network_restrict_host_conformance' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue538_network_restrict_host_conformance' expected zero diagnostics, observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $builtinCount = @($logLines | Where-Object { $_ -like "*`tBuiltinSym-Network-RestrictHost`t*" }).Count
    if ($builtinCount -lt 3) {
        throw "Case 'issue538_network_restrict_host_conformance' expected BuiltinSym-Network-RestrictHost at least three times in conformance trace."
    }

    Write-Host "[compiler-static] issue538_network_restrict_host_conformance: exit=$($result.ExitCode) errors=$errorCount builtin_network_restrict_host=$builtinCount"
}

function Invoke-Issue520FileSystemOpenAppendConformanceCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $result = Invoke-BuildWithConformance `
        -CaseId "issue520_file_system_open_append_conformance" `
        -Source (New-Issue520FileSystemOpenAppendSource) `
        -Manifest $manifest `
        -ConformanceFileName "issue520_file_system_open_append_conformance.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue520_file_system_open_append_conformance' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue520_file_system_open_append_conformance' expected zero diagnostics, observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $builtinCount = @($logLines | Where-Object { $_ -like "*`tBuiltinSym-FileSystem-OpenAppend`t*" }).Count
    if ($builtinCount -lt 1) {
        throw "Case 'issue520_file_system_open_append_conformance' expected BuiltinSym-FileSystem-OpenAppend in conformance trace."
    }

    Write-Host "[compiler-static] issue520_file_system_open_append_conformance: exit=$($result.ExitCode) errors=$errorCount builtin_file_system_open_append=$builtinCount"
}

function Invoke-Issue539SharedClosureEscapeConformanceCase {
    $failures = New-Object System.Collections.Generic.List[string]

    $regionsPath = Join-Path $workspaceRoot "cursive\\src\\04_analysis\\memory\\regions.cpp"
    if (-not (Test-Path $regionsPath)) {
        $failures.Add("Case 'issue539_shared_closure_escape_conformance' missing provenance analysis file: $regionsPath") | Out-Null
    } else {
        $regionsText = Get-Content -Path $regionsPath -Raw
        foreach ($pattern in @(
            'SPEC_RULE\("P-Closure-NonCapturing"\)',
            'SPEC_RULE\("P-Closure-Capturing"\)',
            'SPEC_RULE\("P-Closure-Escape-Err"\)',
            'IsEscapingClosureExpr',
            'ClosureCaptureCollector'
        )) {
            if (-not [System.Text.RegularExpressions.Regex]::IsMatch(
                $regionsText,
                $pattern,
                [System.Text.RegularExpressions.RegexOptions]::Singleline)) {
                $failures.Add("Case 'issue539_shared_closure_escape_conformance' missing expected closure provenance pattern '$pattern' in regions.cpp.") | Out-Null
            }
        }
    }

    $returnPath = Join-Path $workspaceRoot "cursive\\src\\04_analysis\\typing\\stmt\\return_stmt.cpp"
    if (-not (Test-Path $returnPath)) {
        $failures.Add("Case 'issue539_shared_closure_escape_conformance' missing return-statement typing file: $returnPath") | Out-Null
    } else {
        $returnText = Get-Content -Path $returnPath -Raw
        if ($returnText -match 'SPEC_RULE\("K-Closure-Escape-Lifetime-Err"\)') {
            $failures.Add("Case 'issue539_shared_closure_escape_conformance' found legacy blanket K-Closure-Escape-Lifetime-Err emission in return_stmt.cpp.") | Out-Null
        }
    }

    try {
        $allowedResult = Invoke-CheckWithConformance `
            -CaseId "issue539_shared_closure_escape_allowed" `
            -Source (New-Issue539SharedClosureEscapeAllowedSource) `
            -ConformanceFileName "issue539_shared_closure_escape_allowed.log"

        if ($allowedResult.ExitCode -ne 0) {
            $failures.Add("Case 'issue539_shared_closure_escape_allowed' expected exit 0 but got $($allowedResult.ExitCode).") | Out-Null
        } else {
            $allowedErrors = @($allowedResult.DiagJson.diagnostics | Where-Object {
                $_.severity -eq "error" -or $_.severity -eq "panic"
            }).Count
            if ($allowedErrors -ne 0) {
                $failures.Add("Case 'issue539_shared_closure_escape_allowed' expected zero compile-time errors but observed $allowedErrors.") | Out-Null
            }
            $allowedForbidden = @($allowedResult.DiagJson.diagnostics | Where-Object {
                $_.code -eq "E-CON-0085" -or $_.code -eq "E-CON-0086"
            }).Count
            if ($allowedForbidden -ne 0) {
                $failures.Add("Case 'issue539_shared_closure_escape_allowed' unexpectedly emitted E-CON-0085/E-CON-0086.") | Out-Null
            }
            $allowedLog = Get-Content -Path $allowedResult.ConformancePath
            $captureRuleCount = @($allowedLog | Where-Object { $_ -like "*`tP-Closure-Capturing`t*" }).Count
            if ($captureRuleCount -lt 1) {
                $failures.Add("Case 'issue539_shared_closure_escape_allowed' expected P-Closure-Capturing in conformance trace.") | Out-Null
            }
        }
    } catch {
        $failures.Add($_.Exception.Message) | Out-Null
    }

    try {
        $depsResult = Invoke-CheckWithConformance `
            -CaseId "issue539_shared_closure_missing_deps" `
            -Source (New-Issue539SharedClosureMissingDepsSource) `
            -ConformanceFileName "issue539_shared_closure_missing_deps.log"

        if ($depsResult.ExitCode -ne 1) {
            $failures.Add("Case 'issue539_shared_closure_missing_deps' expected exit 1 but got $($depsResult.ExitCode).") | Out-Null
        } else {
            $depsDiagCount = @($depsResult.DiagJson.diagnostics | Where-Object { $_.code -eq "E-CON-0085" }).Count
            if ($depsDiagCount -lt 1) {
                $failures.Add("Case 'issue539_shared_closure_missing_deps' expected E-CON-0085 diagnostic.") | Out-Null
            }
            $depsLog = Get-Content -Path $depsResult.ConformancePath
            $depsRuleCount = @($depsLog | Where-Object { $_ -like "*`tK-Closure-Missing-SharedDeps-Err`t*" }).Count
            if ($depsRuleCount -lt 1) {
                $failures.Add("Case 'issue539_shared_closure_missing_deps' expected K-Closure-Missing-SharedDeps-Err in conformance trace.") | Out-Null
            }
        }
    } catch {
        $failures.Add($_.Exception.Message) | Out-Null
    }

    try {
        $lifetimeResult = Invoke-CheckWithConformance `
            -CaseId "issue539_shared_closure_lifetime_err" `
            -Source (New-Issue539SharedClosureLifetimeErrSource) `
            -ConformanceFileName "issue539_shared_closure_lifetime_err.log"

        if ($lifetimeResult.ExitCode -ne 1) {
            $failures.Add("Case 'issue539_shared_closure_lifetime_err' expected exit 1 but got $($lifetimeResult.ExitCode).") | Out-Null
        } else {
            $lifetimeDiagCount = @($lifetimeResult.DiagJson.diagnostics | Where-Object { $_.code -eq "E-CON-0086" }).Count
            if ($lifetimeDiagCount -lt 1) {
                $failures.Add("Case 'issue539_shared_closure_lifetime_err' expected E-CON-0086 diagnostic.") | Out-Null
            }
            $lifetimeForbidden = @($lifetimeResult.DiagJson.diagnostics | Where-Object { $_.code -eq "E-CON-0085" }).Count
            if ($lifetimeForbidden -ne 0) {
                $failures.Add("Case 'issue539_shared_closure_lifetime_err' unexpectedly emitted E-CON-0085 when deps were provided.") | Out-Null
            }
            $lifetimeLog = Get-Content -Path $lifetimeResult.ConformancePath
            $lifetimeRuleCount = @($lifetimeLog | Where-Object { $_ -like "*`tP-Closure-Escape-Err`t*" }).Count
            if ($lifetimeRuleCount -lt 1) {
                $failures.Add("Case 'issue539_shared_closure_lifetime_err' expected P-Closure-Escape-Err in conformance trace.") | Out-Null
            }
        }
    } catch {
        $failures.Add($_.Exception.Message) | Out-Null
    }

    if ($failures.Count -gt 0) {
        foreach ($failure in $failures) {
            Write-Host "[compiler-static] issue539 failure: $failure"
        }
        throw "Case 'issue539_shared_closure_escape_conformance' found $($failures.Count) conformance violation(s)."
    }

    Write-Host "[compiler-static] issue539_shared_closure_escape_conformance: allowed_escape=1 deps_gate=1 lifetime_gate=1 provenance_trace=1"
}

function Invoke-Issue540InvariantResolutionConformanceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue540_invariant_resolution_conformance" `
        -Source (New-Issue540InvariantResolutionSource) `
        -ConformanceFileName "issue540_invariant_resolution_conformance.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue540_invariant_resolution_conformance' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue540_invariant_resolution_conformance' expected zero compile-time errors, observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $resolveInvariantCount = @($logLines | Where-Object { $_ -like "*`tResolveInvariantOpt-Yes`t*" }).Count
    if ($resolveInvariantCount -lt 2) {
        throw "Case 'issue540_invariant_resolution_conformance' expected ResolveInvariantOpt-Yes for both type and loop invariants."
    }

    $registryPath = Join-Path $workspaceRoot "cursive\\src\\00_core\\generated\\static_rule_registry.inc"
    if (-not (Test-Path $registryPath)) {
        throw "Case 'issue540_invariant_resolution_conformance' missing static rule registry: $registryPath"
    }

    $registryText = Get-Content -Path $registryPath -Raw
    if ($registryText -notmatch '\{"ResolveInvariantOpt-Yes",') {
        throw "Case 'issue540_invariant_resolution_conformance' expected ResolveInvariantOpt-Yes in generated static rule registry."
    }
    if ($registryText -match '\{"ResolveTypeInvariantOpt-Some",') {
        throw "Case 'issue540_invariant_resolution_conformance' found stale ResolveTypeInvariantOpt-Some in generated static rule registry."
    }
    if ($registryText -match '\{"ResolveTypeInvariantOpt-None",') {
        throw "Case 'issue540_invariant_resolution_conformance' found stale ResolveTypeInvariantOpt-None in generated static rule registry."
    }

    Write-Host "[compiler-static] issue540_invariant_resolution_conformance: exit=$($result.ExitCode) errors=$errorCount resolve_invariant=$resolveInvariantCount registry=1"
}

function Invoke-Issue542AsyncAliasSubtypingConformanceCase {
    $result = Invoke-CheckWithConformance `
        -CaseId "issue542_async_alias_subtyping_conformance" `
        -Source (New-Issue542AsyncAliasSubtypingConformanceSource) `
        -ConformanceFileName "issue542_async_alias_subtyping_conformance.log"

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue542_async_alias_subtyping_conformance' expected exit 0 but got $($result.ExitCode)."
    }

    $errorCount = @($result.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($errorCount -ne 0) {
        throw "Case 'issue542_async_alias_subtyping_conformance' expected zero compile-time errors, observed $errorCount."
    }

    $logLines = Get-Content -Path $result.ConformancePath
    $asyncSigExtractCount = @($logLines | Where-Object { $_ -like "*`tAsyncSig-Extract`t*" }).Count
    if ($asyncSigExtractCount -lt 2) {
        throw "Case 'issue542_async_alias_subtyping_conformance' expected AsyncSig-Extract in conformance trace for alias normalization."
    }

    $subsumptionCount = @($logLines | Where-Object { $_ -like "*`tChk-Subsumption`t*" }).Count
    if ($subsumptionCount -lt 1) {
        throw "Case 'issue542_async_alias_subtyping_conformance' expected Chk-Subsumption in conformance trace for async widening."
    }

    Write-Host "[compiler-static] issue542_async_alias_subtyping_conformance: exit=$($result.ExitCode) errors=$errorCount async_sig_extract=$asyncSigExtractCount subsumption=$subsumptionCount"
}

function Invoke-Issue543RefinementUnificationConformanceCase {
    $success = Invoke-CheckWithConformance `
        -CaseId "issue543_refinement_alias_interop" `
        -Source (New-Issue543RefinementAliasInteropSource) `
        -ConformanceFileName "issue543_refinement_alias_interop.log"

    if ($success.ExitCode -ne 0) {
        throw "Case 'issue543_refinement_alias_interop' expected exit 0 but got $($success.ExitCode)."
    }

    $successErrors = @($success.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($successErrors -ne 0) {
        throw "Case 'issue543_refinement_alias_interop' expected zero compile-time errors, observed $successErrors."
    }

    $successLogLines = Get-Content -Path $success.ConformancePath
    $refineEquivCount = @($successLogLines | Where-Object { $_ -like "*`tT-Equiv-Refine`t*" }).Count
    $refineElimCount = @($successLogLines | Where-Object { $_ -like "*`tSub-Refine-Elim`t*" }).Count
    $subsumptionCount = @($successLogLines | Where-Object { $_ -like "*`tChk-Subsumption`t*" }).Count
    if ($refineEquivCount -lt 1 -or $refineElimCount -lt 1 -or $subsumptionCount -lt 2) {
        throw "Case 'issue543_refinement_alias_interop' expected T-Equiv-Refine, Sub-Refine-Elim, and repeated Chk-Subsumption in conformance trace."
    }

    Invoke-ExpectedDiagCodeCase `
        -Id "issue543_refinement_narrowing_rejected" `
        -Source (New-Issue543RefinementNarrowingRejectSource) `
        -ExpectedCodes @("E-SEM-2533")

    $presentChecks = @(
        @{ Path = "cursive\\src\\04_analysis\\typing\\type_infer.cpp"; Pattern = 'static\s+bool\s+RefinePredicatesEqual\s*\(' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\type_infer.cpp"; Pattern = 'SPEC_RULE\("Unify-Refine"\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\type_infer.cpp"; Pattern = 'SPEC_RULE\("Unify-Refine-Pred-Fail"\)' },
        @{ Path = "cursive\\src\\04_analysis\\typing\\type_infer.cpp"; Pattern = 'ExprStructEqual\(lhs,\s*rhs\)' }
    )

    foreach ($check in $presentChecks) {
        $fullPath = Join-Path $workspaceRoot $check.Path
        if (-not (Test-Path $fullPath)) {
            throw "Case 'issue543_refinement_unification_conformance' missing file: $fullPath"
        }
        $text = Get-Content -Path $fullPath -Raw
        if ($text -notmatch $check.Pattern) {
            throw "Case 'issue543_refinement_unification_conformance' missing expected pattern '$($check.Pattern)' in $fullPath."
        }
    }

    Write-Host "[compiler-static] issue543_refinement_unification_conformance: success_errors=$successErrors t_equiv_refine=$refineEquivCount sub_refine_elim=$refineElimCount chk_subsumption=$subsumptionCount wiring_checks=$($presentChecks.Count)"
}

function Invoke-Issue552LinkKindManifestConformanceCase {
    $badValueManifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""library""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "link_kind = ""bogus"""
    )

    $badValue = Invoke-CheckWithConformance `
        -CaseId "issue552_link_kind_invalid_value" `
        -Source (New-MinimalMainSource) `
        -Manifest $badValueManifest `
        -AllowMissingConformance `
        -ConformanceFileName "issue552_link_kind_invalid_value.log"

    if ($badValue.ExitCode -ne 1) {
        throw "Case 'issue552_link_kind_invalid_value' expected exit 1 but got $($badValue.ExitCode)."
    }
    $badValueDiag = @($badValue.DiagJson.diagnostics | Where-Object { $_.code -eq "E-PRJ-0207" }).Count
    if ($badValueDiag -lt 1) {
        throw "Case 'issue552_link_kind_invalid_value' expected E-PRJ-0207."
    }

    $wrongUseManifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""executable""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "link_kind = ""static"""
    )

    $wrongUse = Invoke-CheckWithConformance `
        -CaseId "issue552_link_kind_wrong_use" `
        -Source (New-MinimalMainSource) `
        -Manifest $wrongUseManifest `
        -AllowMissingConformance `
        -ConformanceFileName "issue552_link_kind_wrong_use.log"

    if ($wrongUse.ExitCode -ne 1) {
        throw "Case 'issue552_link_kind_wrong_use' expected exit 1 but got $($wrongUse.ExitCode)."
    }
    $wrongUseDiag = @($wrongUse.DiagJson.diagnostics | Where-Object { $_.code -eq "E-PRJ-0208" }).Count
    if ($wrongUseDiag -lt 1) {
        throw "Case 'issue552_link_kind_wrong_use' expected E-PRJ-0208."
    }

    Write-Host "[compiler-static] issue552_link_kind_manifest: invalid_value=$badValueDiag wrong_use=$wrongUseDiag"
}

function Invoke-Issue552ManifestNameProjectionConformanceCase {
    $manifest = @(
        "[[assembly]]",
        "name = ""schema_name_probe""",
        "kind = ""executable""",
        "root = "".""",
        "out_dir = ""build/schema_name_probe"""
    )

    $result = Invoke-StdoutModeWithConformance `
        -CaseId "issue552_manifest_name_projection" `
        -Source (New-MinimalMainSource) `
        -Manifest $manifest `
        -AllowMissingConformance `
        -ConformanceFileName "issue552_manifest_name_projection.log" `
        -ExtraArgs @("--assembly", "schema_name_probe", "--dump")

    if ($result.ExitCode -ne 0) {
        throw "Case 'issue552_manifest_name_projection' expected exit 0 but got $($result.ExitCode)."
    }
    if ($result.StdoutText -notmatch 'assembly_name:\s+schema_name_probe') {
        throw "Case 'issue552_manifest_name_projection' expected dump output to reflect the manifest name field."
    }
    if ($result.StdoutText -notmatch 'assemblies:\s+\[(?=.*schema_name_probe)') {
        throw "Case 'issue552_manifest_name_projection' expected dump output to enumerate the manifest-defined assembly name."
    }

    Write-Host "[compiler-static] issue552_manifest_name_projection: assembly_name=1 assemblies=1"
}

function Invoke-Issue552AssemblyGraphConformanceCase {
    $importExecManifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""executable""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "",
        "[[assembly]]",
        "name = ""tool""",
        "kind = ""executable""",
        "root = ""tool""",
        "out_dir = ""build/tool"""
    )

    $importExec = Invoke-CheckWithConformance `
        -CaseId "issue552_import_executable_rejected" `
        -Source @"
import tool

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@ `
        -Manifest $importExecManifest `
        -ExtraArgs @("--assembly", "probe") `
        -AllowMissingConformance `
        -ConformanceFileName "issue552_import_executable_rejected.log" `
        -ExtraFiles @{
            "tool/Main.cursive" = @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
        }

    if ($importExec.ExitCode -ne 1) {
        throw "Case 'issue552_import_executable_rejected' expected exit 1 but got $($importExec.ExitCode)."
    }
    $importExecDiag = @($importExec.DiagJson.diagnostics | Where-Object { $_.code -eq "E-PRJ-0209" }).Count
    if ($importExecDiag -lt 1) {
        throw "Case 'issue552_import_executable_rejected' expected E-PRJ-0209."
    }

    $cycleManifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""executable""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "",
        "[[assembly]]",
        "name = ""liba""",
        "kind = ""library""",
        "root = ""liba""",
        "out_dir = ""build/liba""",
        "",
        "[[assembly]]",
        "name = ""libb""",
        "kind = ""library""",
        "root = ""libb""",
        "out_dir = ""build/libb"""
    )

    $cycle = Invoke-CheckWithConformance `
        -CaseId "issue552_library_cycle_rejected" `
        -Source @"
import liba

public procedure main(move ctx: Context) -> i32 {
    return liba::run(ctx)
}
"@ `
        -Manifest $cycleManifest `
        -ExtraArgs @("--assembly", "probe") `
        -AllowMissingConformance `
        -ConformanceFileName "issue552_library_cycle_rejected.log" `
        -ExtraFiles @{
            "liba/A.cursive" = @"
import libb

public procedure run(ctx: Context) -> i32 {
    let _ = ctx
    return libb::step()
}
"@;
            "libb/B.cursive" = @"
import liba

public procedure step() -> i32 {
    return 0
}
"@
        }

    if ($cycle.ExitCode -ne 1) {
        throw "Case 'issue552_library_cycle_rejected' expected exit 1 but got $($cycle.ExitCode)."
    }
    $cycleDiag = @($cycle.DiagJson.diagnostics | Where-Object { $_.code -eq "E-PRJ-0209" }).Count
    if ($cycleDiag -lt 1) {
        throw "Case 'issue552_library_cycle_rejected' expected E-PRJ-0209."
    }

    $unrelatedImportExecManifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""executable""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "",
        "[[assembly]]",
        "name = ""sidebad""",
        "kind = ""library""",
        "root = ""sidebad""",
        "out_dir = ""build/sidebad""",
        "",
        "[[assembly]]",
        "name = ""sidetool""",
        "kind = ""executable""",
        "root = ""sidetool""",
        "out_dir = ""build/sidetool"""
    )

    $unrelatedImportExec = Invoke-CheckWithConformance `
        -CaseId "issue552_unrelated_import_executable_rejected" `
        -Source @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@ `
        -Manifest $unrelatedImportExecManifest `
        -ExtraArgs @("--assembly", "probe") `
        -AllowMissingConformance `
        -ConformanceFileName "issue552_unrelated_import_executable_rejected.log" `
        -ExtraFiles @{
            "sidebad/Library.cursive" = @"
import sidetool

public procedure helper() -> i32 {
    return 0
}
"@;
            "sidetool/Main.cursive" = @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@
        }

    if ($unrelatedImportExec.ExitCode -ne 0) {
        throw "Case 'issue552_unrelated_import_executable_rejected' expected exit 0 but got $($unrelatedImportExec.ExitCode)."
    }
    $unrelatedImportExecDiag = @($unrelatedImportExec.DiagJson.diagnostics | Where-Object {
        $_.code -eq "E-PRJ-0209"
    }).Count
    if ($unrelatedImportExecDiag -ne 0) {
        throw "Case 'issue552_unrelated_import_executable_rejected' must not emit E-PRJ-0209 for an unrelated assembly."
    }

    $unrelatedCycleManifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""executable""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "",
        "[[assembly]]",
        "name = ""cyclea""",
        "kind = ""library""",
        "root = ""cyclea""",
        "out_dir = ""build/cyclea""",
        "",
        "[[assembly]]",
        "name = ""cycleb""",
        "kind = ""library""",
        "root = ""cycleb""",
        "out_dir = ""build/cycleb"""
    )

    $unrelatedCycle = Invoke-CheckWithConformance `
        -CaseId "issue552_unrelated_library_cycle_rejected" `
        -Source @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@ `
        -Manifest $unrelatedCycleManifest `
        -ExtraArgs @("--assembly", "probe") `
        -AllowMissingConformance `
        -ConformanceFileName "issue552_unrelated_library_cycle_rejected.log" `
        -ExtraFiles @{
            "cyclea/A.cursive" = @"
import cycleb

public procedure helper() -> i32 {
    return cycleb::step()
}
"@;
            "cycleb/B.cursive" = @"
import cyclea

public procedure step() -> i32 {
    return 0
}
"@
        }

    if ($unrelatedCycle.ExitCode -ne 0) {
        throw "Case 'issue552_unrelated_library_cycle_rejected' expected exit 0 but got $($unrelatedCycle.ExitCode)."
    }
    $unrelatedCycleDiag = @($unrelatedCycle.DiagJson.diagnostics | Where-Object {
        $_.code -eq "E-PRJ-0209"
    }).Count
    if ($unrelatedCycleDiag -ne 0) {
        throw "Case 'issue552_unrelated_library_cycle_rejected' must not emit E-PRJ-0209 for an unrelated assembly cycle."
    }

    Write-Host "[compiler-static] issue552_assembly_graph: import_exec=$importExecDiag cycle=$cycleDiag unrelated_import_exec=$unrelatedImportExecDiag unrelated_cycle=$unrelatedCycleDiag"
}

function Invoke-Issue552ArtifactPipelineConformanceCase {
    $dependencyManifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""dependency""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $dependency = Invoke-BuildWithConformance `
        -CaseId "issue552_dependency_no_link" `
        -Source @"
public procedure helper() -> i32 {
    return 7
}
"@ `
        -Manifest $dependencyManifest `
        -ConformanceFileName "issue552_dependency_no_link.log"

    if ($dependency.ExitCode -ne 0) {
        throw "Case 'issue552_dependency_no_link' expected exit 0 but got $($dependency.ExitCode)."
    }
    $dependencyErrors = @($dependency.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($dependencyErrors -ne 0) {
        throw "Case 'issue552_dependency_no_link' expected zero errors but observed $dependencyErrors."
    }
    $dependencyObjs = @(Get-ChildItem -Path (Join-Path $dependency.CaseRoot "build/probe/obj") -Filter *.obj -File -ErrorAction SilentlyContinue).Count
    $dependencyIrs = @(Get-ChildItem -Path (Join-Path $dependency.CaseRoot "build/probe/ir") -Filter *.ll -File -ErrorAction SilentlyContinue).Count
    if ($dependencyObjs -lt 1 -or $dependencyIrs -lt 1) {
        throw "Case 'issue552_dependency_no_link' expected emitted objects and LLVM IR."
    }
    foreach ($forbidden in @(
        "build/probe/bin/probe.exe",
        "build/probe/bin/probe.dll",
        "build/probe/lib/probe.lib"
    )) {
        if (Test-Path (Join-Path $dependency.CaseRoot $forbidden)) {
            throw "Case 'issue552_dependency_no_link' unexpectedly produced final artifact: $forbidden"
        }
    }
    $dependencyLog = Get-Content -Path $dependency.ConformancePath
    $dependencyRuleCount = @($dependencyLog | Where-Object { $_ -like "*`tOutput-Pipeline-Dependency`t*" }).Count
    if ($dependencyRuleCount -lt 1) {
        throw "Case 'issue552_dependency_no_link' expected Output-Pipeline-Dependency in the conformance trace."
    }

    $sharedManifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""executable""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "",
        "[[assembly]]",
        "name = ""shareddep""",
        "kind = ""dependency""",
        "root = ""shareddep""",
        "out_dir = ""build/shareddep""",
        "",
        "[[assembly]]",
        "name = ""sharedlib""",
        "kind = ""library""",
        "root = ""sharedlib""",
        "out_dir = ""build/sharedlib"""
    )

    $sharedBuild = Invoke-BuildWithConformance `
        -CaseId "issue552_shared_library_artifacts" `
        -Source @"
import sharedlib

public procedure main(move ctx: Context) -> i32 {
    return sharedlib::run(ctx)
}
"@ `
        -Manifest $sharedManifest `
        -ConformanceFileName "issue552_shared_library_artifacts.log" `
        -ExtraFiles @{
            "shareddep/Helper.cursive" = @"
public procedure contribution() -> i32 {
    return 41
}
"@;
            "sharedlib/Library.cursive" = @"
import shareddep

public let shared_counter: i32 = 41

internal procedure hidden_helper() -> i32 {
    return 9
}

public procedure run(ctx: Context) -> i32 {
    let _ = ctx
    if (shareddep::contribution() != shared_counter) {
        return 1
    }
    return hidden_helper() - 9
}
"@
        }

    if ($sharedBuild.ExitCode -ne 0) {
        throw "Case 'issue552_shared_library_artifacts' expected exit 0 but got $($sharedBuild.ExitCode)."
    }
    $sharedErrors = @($sharedBuild.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($sharedErrors -ne 0) {
        throw "Case 'issue552_shared_library_artifacts' expected zero errors but observed $sharedErrors."
    }
    foreach ($required in @(
        "build/probe/bin/probe.exe",
        "build/sharedlib/bin/sharedlib.dll",
        "build/sharedlib/lib/sharedlib.lib"
    )) {
        if (-not (Test-Path (Join-Path $sharedBuild.CaseRoot $required))) {
            throw "Case 'issue552_shared_library_artifacts' missing required artifact: $required"
        }
    }
    foreach ($forbidden in @(
        "build/shareddep/bin/shareddep.exe",
        "build/shareddep/bin/shareddep.dll",
        "build/shareddep/lib/shareddep.lib"
    )) {
        if (Test-Path (Join-Path $sharedBuild.CaseRoot $forbidden)) {
            throw "Case 'issue552_shared_library_artifacts' unexpectedly produced dependency final artifact: $forbidden"
        }
    }
    $sharedLog = Get-Content -Path $sharedBuild.ConformancePath
    $sharedLinkOk = @($sharedLog | Where-Object { $_ -like "*`tOut-Final-Link-Ok`t*" }).Count
    if ($sharedLinkOk -lt 1) {
        throw "Case 'issue552_shared_library_artifacts' expected Out-Final-Link-Ok in the conformance trace."
    }
    $sharedDllPath = Join-Path $sharedBuild.CaseRoot "build\sharedlib\bin\sharedlib.dll"
    $sharedDllExports = Get-CoffExportNames $sharedDllPath
    if (-not ($sharedDllExports -contains "sharedlib_x3a_x3arun")) {
        throw "Case 'issue552_shared_library_artifacts' expected the library entry procedure export to be present in the DLL export table."
    }
    if (-not ($sharedDllExports -contains "sharedlib_x3a_x3ashared_x5fcounter")) {
        throw "Case 'issue552_shared_library_artifacts' expected the public static data export to be present in the DLL export table."
    }
    if ($sharedDllExports -contains "sharedlib_x3a_x3ahidden_x5fhelper") {
        throw "Case 'issue552_shared_library_artifacts' must not export internal procedures."
    }
    if ($sharedDllExports -contains "__cursive_library_entry") {
        throw "Case 'issue552_shared_library_artifacts' must not export the generated shared-library entrypoint."
    }
    $sharedHelperExports = @($sharedDllExports | Where-Object { $_ -like "__cx_*" })
    if ($sharedHelperExports.Count -gt 0) {
        throw "Case 'issue552_shared_library_artifacts' must not export generated helper symbols: $($sharedHelperExports -join ', ')"
    }
    $sharedRuntimeInitExports = @($sharedDllExports | Where-Object { $_ -match '^cursive_x3a_x3aruntime_x3a_x3a(init|deinit)' })
    if ($sharedRuntimeInitExports.Count -gt 0) {
        throw "Case 'issue552_shared_library_artifacts' must not export generated init/deinit symbols: $($sharedRuntimeInitExports -join ', ')"
    }

    $staticManifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""executable""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "",
        "[[assembly]]",
        "name = ""staticdep""",
        "kind = ""dependency""",
        "root = ""staticdep""",
        "out_dir = ""build/staticdep""",
        "",
        "[[assembly]]",
        "name = ""staticlib""",
        "kind = ""library""",
        "link_kind = ""static""",
        "root = ""staticlib""",
        "out_dir = ""build/staticlib"""
    )

    $staticBuild = Invoke-BuildWithConformance `
        -CaseId "issue552_static_library_artifacts" `
        -Source @"
import staticlib

public procedure main(move ctx: Context) -> i32 {
    return staticlib::run(ctx)
}
"@ `
        -Manifest $staticManifest `
        -ConformanceFileName "issue552_static_library_artifacts.log" `
        -ExtraFiles @{
            "staticdep/Helper.cursive" = @"
public procedure contribution() -> i32 {
    return 64
}
"@;
            "staticlib/Library.cursive" = @"
import staticdep

public procedure run(ctx: Context) -> i32 {
    let _ = ctx
    if (staticdep::contribution() != 64) {
        return 1
    }
    return 0
}
"@
        }

    if ($staticBuild.ExitCode -ne 0) {
        throw "Case 'issue552_static_library_artifacts' expected exit 0 but got $($staticBuild.ExitCode)."
    }
    $staticErrors = @($staticBuild.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($staticErrors -ne 0) {
        throw "Case 'issue552_static_library_artifacts' expected zero errors but observed $staticErrors."
    }
    foreach ($required in @(
        "build/probe/bin/probe.exe",
        "build/staticlib/lib/staticlib.lib"
    )) {
        if (-not (Test-Path (Join-Path $staticBuild.CaseRoot $required))) {
            throw "Case 'issue552_static_library_artifacts' missing required artifact: $required"
        }
    }
    if (Test-Path (Join-Path $staticBuild.CaseRoot "build/staticlib/bin/staticlib.dll")) {
        throw "Case 'issue552_static_library_artifacts' unexpectedly produced a shared-library runtime artifact."
    }
    $staticLog = Get-Content -Path $staticBuild.ConformancePath
    $archiveOk = @($staticLog | Where-Object { $_ -like "*`tOut-Final-Archive-Ok`t*" }).Count
    $linkOk = @($staticLog | Where-Object { $_ -like "*`tOut-Final-Link-Ok`t*" }).Count
    if ($archiveOk -lt 1 -or $linkOk -lt 1) {
        throw "Case 'issue552_static_library_artifacts' expected Out-Final-Archive-Ok for the static library and Out-Final-Link-Ok for the executable."
    }

    $buildOrderManifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""executable""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "",
        "[[assembly]]",
        "name = ""aaa""",
        "kind = ""library""",
        "root = ""aaa""",
        "out_dir = ""build/aaa""",
        "",
        "[[assembly]]",
        "name = ""bbb""",
        "kind = ""library""",
        "root = ""bbb""",
        "out_dir = ""build/bbb""",
        "",
        "[[assembly]]",
        "name = ""ccc""",
        "kind = ""library""",
        "root = ""ccc""",
        "out_dir = ""build/ccc"""
    )

    $buildOrder = Invoke-BuildWithConformance `
        -CaseId "issue552_library_topo_build_order" `
        -Source @"
import aaa
import bbb

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return aaa::run() + bbb::run()
}
"@ `
        -Manifest $buildOrderManifest `
        -ExtraArgs @("--assembly", "probe", "--verbose") `
        -ConformanceFileName "issue552_library_topo_build_order.log" `
        -ExtraFiles @{
            "aaa/Library.cursive" = @"
import ccc

public procedure run() -> i32 {
    return ccc::run()
}
"@;
            "bbb/Library.cursive" = @"
public procedure run() -> i32 {
    return 2
}
"@;
            "ccc/Library.cursive" = @"
public procedure run() -> i32 {
    return 1
}
"@
        }

    if ($buildOrder.ExitCode -ne 0) {
        throw "Case 'issue552_library_topo_build_order' expected exit 0 but got $($buildOrder.ExitCode)."
    }
    $buildOrderErrors = @($buildOrder.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($buildOrderErrors -ne 0) {
        throw "Case 'issue552_library_topo_build_order' expected zero errors but observed $buildOrderErrors."
    }
    $buildProgress = Get-BuildProgressAssemblies $buildOrder.StderrPath
    $bbbIndex = [array]::IndexOf($buildProgress, "bbb")
    $cccIndex = [array]::IndexOf($buildProgress, "ccc")
    $aaaIndex = [array]::IndexOf($buildProgress, "aaa")
    $probeOccurrences = @($buildProgress | Where-Object { $_ -eq "probe" }).Count
    $lastProbeIndex = [array]::LastIndexOf($buildProgress, "probe")
    if ($bbbIndex -lt 0 -or $cccIndex -lt 0 -or $aaaIndex -lt 0) {
        throw "Case 'issue552_library_topo_build_order' missing expected build progress entries in stderr: $($buildProgress -join ', ')"
    }
    if (-not ($bbbIndex -lt $cccIndex -and $cccIndex -lt $aaaIndex)) {
        throw "Case 'issue552_library_topo_build_order' expected ready-lex-least order bbb, ccc, aaa but observed: $($buildProgress -join ', ')"
    }
    if ($probeOccurrences -lt 2 -or $lastProbeIndex -le $aaaIndex) {
        throw "Case 'issue552_library_topo_build_order' expected the root assembly to build after its libraries. Observed stderr order: $($buildProgress -join ', ')"
    }

    Write-Host "[compiler-static] issue552_artifact_pipeline: dependency_rule=$dependencyRuleCount shared_link_ok=$sharedLinkOk static_archive_ok=$archiveOk static_link_ok=$linkOk topo_order=bbb,ccc,aaa"
}

function Invoke-SingleExeCompilerPackagingConformanceCase {
    $compilerExeDir = Split-Path -Parent $CompilerPath
    $compilerBuildRoot = Split-Path -Parent $compilerExeDir
    $compilerDistRoot = Join-Path $compilerBuildRoot "out"
    foreach ($requiredPath in @(
        "cursive.exe",
        "cursive0_rt.lib",
        "windows\\bin\\icudt72.dll",
        "windows\\bin\\icuuc72.dll",
        "windows\\bin\\icuin72.dll",
        "windows\\lib\\delayimp.lib",
        "windows\\tools\\lld-link.exe",
        "windows\\tools\\llvm-as.exe",
        "windows\\tools\\llvm-lib.exe"
    )) {
        if (-not (Test-Path (Join-Path $compilerDistRoot $requiredPath))) {
            throw "Single-exe packaging case missing packaged compiler distribution artifact: $(Join-Path $compilerDistRoot $requiredPath)"
        }
    }
    $packagedCompilerPath = Join-Path $compilerDistRoot "cursive.exe"

    $missingSidecarsRoot = Join-Path $workRoot "single_exe_missing_sidecars"
    New-Item -ItemType Directory -Path $missingSidecarsRoot -Force | Out-Null
    $missingCompilerPath = Join-Path $missingSidecarsRoot "cursive.exe"
    Copy-Item -Path $packagedCompilerPath -Destination $missingCompilerPath -Force

    $missingLocalAppData = Join-Path $missingSidecarsRoot "LocalAppData"
    $missingTemp = Join-Path $missingSidecarsRoot "Temp"
    New-Item -ItemType Directory -Path $missingLocalAppData -Force | Out-Null
    New-Item -ItemType Directory -Path $missingTemp -Force | Out-Null
    [System.IO.File]::WriteAllLines((Join-Path $missingSidecarsRoot "Cursive.toml"), $manifestLines)
    [System.IO.File]::WriteAllText((Join-Path $missingSidecarsRoot "Main.cursive"), (New-MinimalMainSource))
    $systemRoot = $env:SystemRoot
    if ([string]::IsNullOrWhiteSpace($systemRoot)) {
        $systemRoot = "C:\\Windows"
    }
    $minimalPath = @(
        (Join-Path $systemRoot "System32"),
        $systemRoot
    ) -join ";"
    $missingEnv = @{
        LOCALAPPDATA = $missingLocalAppData;
        TEMP = $missingTemp;
        TMP = $missingTemp;
        PATH = $minimalPath
    }
    $missingStdoutPath = Join-Path $missingSidecarsRoot "stdout.txt"
    $missingStderrPath = Join-Path $missingSidecarsRoot "stderr.txt"
    $savedMissingEnv = @{}
    $savedMissingErrorActionPreference = $ErrorActionPreference
    Push-Location $missingSidecarsRoot
    try {
        $ErrorActionPreference = "Continue"
        foreach ($name in $missingEnv.Keys) {
            $envName = [string]$name
            $existing = Get-Item -Path ("Env:" + $envName) -ErrorAction SilentlyContinue
            if ($null -ne $existing) {
                $savedMissingEnv[$envName] = $existing.Value
            } else {
                $savedMissingEnv[$envName] = $null
            }
            Set-Item -Path ("Env:" + $envName) -Value ([string]$missingEnv[$name])
        }
        & $missingCompilerPath build Main.cursive 1> $missingStdoutPath 2> $missingStderrPath
        $missingExitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $savedMissingErrorActionPreference
        foreach ($name in $missingEnv.Keys) {
            $envName = [string]$name
            if ($savedMissingEnv.ContainsKey($envName) -and $null -ne $savedMissingEnv[$envName]) {
                Set-Item -Path ("Env:" + $envName) -Value ([string]$savedMissingEnv[$envName])
            } else {
                Remove-Item -Path ("Env:" + $envName) -ErrorAction SilentlyContinue
            }
        }
        Pop-Location
    }

    if ($missingExitCode -eq 0) {
        throw "Case 'single_exe_missing_sidecars' expected compiler startup to fail without sidecar files."
    }
    $missingStderr = Get-Content -Path $missingStderrPath -Raw
    if ($missingStderr -notmatch "compiler sidecar") {
        throw "Case 'single_exe_missing_sidecars' expected stderr to mention missing compiler sidecar files."
    }

    $standaloneRoot = Join-Path $workRoot "single_exe_distribution"
    New-Item -ItemType Directory -Path $standaloneRoot -Force | Out-Null

    $standaloneCompilerPath = Join-Path $standaloneRoot "cursive.exe"
    Copy-Item -Path $packagedCompilerPath -Destination $standaloneCompilerPath -Force
    Copy-Item -Path (Join-Path $compilerDistRoot "cursive0_rt.lib") `
        -Destination (Join-Path $standaloneRoot "cursive0_rt.lib") -Force
    Copy-Item -Path (Join-Path $compilerDistRoot "windows") `
        -Destination (Join-Path $standaloneRoot "windows") -Recurse -Force

    $isolatedLocalAppData = Join-Path $standaloneRoot "LocalAppData"
    $isolatedTemp = Join-Path $standaloneRoot "Temp"
    New-Item -ItemType Directory -Path $isolatedLocalAppData -Force | Out-Null
    New-Item -ItemType Directory -Path $isolatedTemp -Force | Out-Null
    $standaloneEnv = @{
        LOCALAPPDATA = $isolatedLocalAppData;
        TEMP = $isolatedTemp;
        TMP = $isolatedTemp;
        PATH = $minimalPath
    }

    $bcManifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""executable""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""bc"""
    )

    $bcBuild = Invoke-BuildWithConformance `
        -CaseId "single_exe_bc_pipeline" `
        -Source @"
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
"@ `
        -Manifest $bcManifest `
        -ConformanceFileName "single_exe_bc_pipeline.log" `
        -CompilerPathOverride $standaloneCompilerPath `
        -EnvOverrides $standaloneEnv

    if ($bcBuild.ExitCode -ne 0) {
        throw "Case 'single_exe_bc_pipeline' expected exit 0 but got $($bcBuild.ExitCode)."
    }
    $bcErrors = @($bcBuild.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($bcErrors -ne 0) {
        throw "Case 'single_exe_bc_pipeline' expected zero errors but observed $bcErrors."
    }
    foreach ($required in @(
        "build/probe/bin/probe.exe",
        "build/probe/ir/probe.bc"
    )) {
        if (-not (Test-Path (Join-Path $bcBuild.CaseRoot $required))) {
            throw "Case 'single_exe_bc_pipeline' missing required artifact: $required"
        }
    }

    $sharedManifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""executable""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "",
        "[[assembly]]",
        "name = ""shareddep""",
        "kind = ""dependency""",
        "root = ""shareddep""",
        "out_dir = ""build/shareddep""",
        "",
        "[[assembly]]",
        "name = ""sharedlib""",
        "kind = ""library""",
        "root = ""sharedlib""",
        "out_dir = ""build/sharedlib"""
    )

    $sharedBuild = Invoke-BuildWithConformance `
        -CaseId "single_exe_shared_library_artifacts" `
        -Source @"
import sharedlib

public procedure main(move ctx: Context) -> i32 {
    return sharedlib::run(ctx)
}
"@ `
        -Manifest $sharedManifest `
        -ConformanceFileName "single_exe_shared_library_artifacts.log" `
        -CompilerPathOverride $standaloneCompilerPath `
        -EnvOverrides $standaloneEnv `
        -ExtraFiles @{
            "shareddep/Helper.cursive" = @"
public procedure contribution() -> i32 {
    return 41
}
"@;
            "sharedlib/Library.cursive" = @"
import shareddep

public procedure run(ctx: Context) -> i32 {
    let _ = ctx
    if (shareddep::contribution() != 41) {
        return 1
    }
    return 0
}
"@
        }

    if ($sharedBuild.ExitCode -ne 0) {
        throw "Case 'single_exe_shared_library_artifacts' expected exit 0 but got $($sharedBuild.ExitCode)."
    }
    $sharedErrors = @($sharedBuild.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($sharedErrors -ne 0) {
        throw "Case 'single_exe_shared_library_artifacts' expected zero errors but observed $sharedErrors."
    }
    foreach ($required in @(
        "build/probe/bin/probe.exe",
        "build/sharedlib/bin/sharedlib.dll",
        "build/sharedlib/lib/sharedlib.lib"
    )) {
        if (-not (Test-Path (Join-Path $sharedBuild.CaseRoot $required))) {
            throw "Case 'single_exe_shared_library_artifacts' missing required artifact: $required"
        }
    }

    $staticManifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""executable""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "",
        "[[assembly]]",
        "name = ""staticdep""",
        "kind = ""dependency""",
        "root = ""staticdep""",
        "out_dir = ""build/staticdep""",
        "",
        "[[assembly]]",
        "name = ""staticlib""",
        "kind = ""library""",
        "link_kind = ""static""",
        "root = ""staticlib""",
        "out_dir = ""build/staticlib"""
    )

    $staticBuild = Invoke-BuildWithConformance `
        -CaseId "single_exe_static_library_artifacts" `
        -Source @"
import staticlib

public procedure main(move ctx: Context) -> i32 {
    return staticlib::run(ctx)
}
"@ `
        -Manifest $staticManifest `
        -ConformanceFileName "single_exe_static_library_artifacts.log" `
        -CompilerPathOverride $standaloneCompilerPath `
        -EnvOverrides $standaloneEnv `
        -ExtraFiles @{
            "staticdep/Helper.cursive" = @"
public procedure contribution() -> i32 {
    return 64
}
"@;
            "staticlib/Library.cursive" = @"
import staticdep

public procedure run(ctx: Context) -> i32 {
    let _ = ctx
    if (staticdep::contribution() != 64) {
        return 1
    }
    return 0
}
"@
        }

    if ($staticBuild.ExitCode -ne 0) {
        throw "Case 'single_exe_static_library_artifacts' expected exit 0 but got $($staticBuild.ExitCode)."
    }
    $staticErrors = @($staticBuild.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($staticErrors -ne 0) {
        throw "Case 'single_exe_static_library_artifacts' expected zero errors but observed $staticErrors."
    }
    foreach ($required in @(
        "build/probe/bin/probe.exe",
        "build/staticlib/lib/staticlib.lib"
    )) {
        if (-not (Test-Path (Join-Path $staticBuild.CaseRoot $required))) {
            throw "Case 'single_exe_static_library_artifacts' missing required artifact: $required"
        }
    }

    $bundleRoot = Join-Path $isolatedLocalAppData "Cursive\\toolchain"
    if (Test-Path $bundleRoot) {
        throw "Single-exe packaging case unexpectedly created a persistent compiler bundle under '$bundleRoot'."
    }

    $leftoverBundleEntries = @(Get-ChildItem -Path $isolatedTemp -Force -ErrorAction SilentlyContinue | Where-Object {
        $_.Name -like "cursive-bundle-*"
    })
    if ($leftoverBundleEntries.Count -ne 0) {
        $names = [string]::Join(", ", @($leftoverBundleEntries | ForEach-Object { $_.Name }))
        throw "Single-exe packaging case left temporary compiler bundle entries behind: $names"
    }

    Write-Host "[compiler-static] single_exe_packaging: missing_sidecars=fail bc=ok shared=ok static=ok localappdata_cache=0 temp_residue=0"
}

function Invoke-Issue553HostedLibraryConformanceCase {
    $libraryManifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""library""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $success = Invoke-BuildWithConformance `
        -CaseId "issue553_hosted_export_ir_wiring" `
        -Source @'
record HostedProbeContext {
    fs: $FileSystem
    inline: $ExecutionDomain
}

[[layout(C)]]
record HostedProbeRecord {
    first: i32
    second: i32
    third: i32
}

[[layout(C), ffi_pass_by_value]]
record HostedReadBodyCmd {
    handle: u64
    body: u64
}

[[layout(C)]]
record HostedBodyState {
    motion: i32
    stamp: i32
}

var process_counter: i32 = 0

public procedure run(ctx: Context) -> i32 {
    let _ = ctx
    process_counter = process_counter + 1
    return process_counter
}

[[host_export("C-unwind"), unwind("catch"), mangle("hc_issue553_host_probe")]]
public procedure host_probe(ctx: HostedProbeContext) -> i32 {
    let _ = ctx.fs
    let domain: $ExecutionDomain = ctx.inline
    return parallel domain {
        spawn { 17 }
    }
}

[[host_export("C-unwind"), unwind("catch"), mangle("hc_issue553_host_probe_record")]]
public procedure host_probe_record(ctx: HostedProbeContext) -> HostedProbeRecord {
    let _ = ctx.fs
    return HostedProbeRecord { first: 11, second: 22, third: 33 }
}

[[host_export("C-unwind"), unwind("catch"), mangle("hc_issue553_host_read_body_state")]]
public procedure host_read_body_state(ctx: HostedProbeContext,
                                      move read_cmd: HostedReadBodyCmd,
                                      move out_state: *mut HostedBodyState) -> u32 {
    let _ = ctx.fs
    let _ = ctx.inline
    var out_state_mut: *mut HostedBodyState = out_state
    if (read_cmd.handle == 0u64) {
        return 9u32
    }
    let state: HostedBodyState = HostedBodyState { motion: 2, stamp: 77 }
    unsafe {
        *out_state_mut = move state
    }
    return 0u32
}
'@ `
        -Manifest $libraryManifest `
        -ConformanceFileName "issue553_hosted_export_ir_wiring.log"

    if ($success.ExitCode -ne 0) {
        throw "Case 'issue553_hosted_export_ir_wiring' expected exit 0 but got $($success.ExitCode)."
    }
    $successErrors = @($success.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($successErrors -ne 0) {
        throw "Case 'issue553_hosted_export_ir_wiring' expected zero errors but observed $successErrors."
    }

    foreach ($required in @(
        "build/probe/bin/probe.dll",
        "build/probe/lib/probe.lib",
        "build/probe/ir/probe.ll"
    )) {
        if (-not (Test-Path (Join-Path $success.CaseRoot $required))) {
            throw "Case 'issue553_hosted_export_ir_wiring' missing required artifact: $required"
        }
    }

    $irPath = Join-Path $success.CaseRoot "build\probe\ir\probe.ll"
    $dllPath = Join-Path $success.CaseRoot "build\probe\bin\probe.dll"
    $irText = Get-Content -Path $irPath -Raw
    $abiVersionCount = ([regex]::Matches($irText, "@__cursive_host_abi_version")).Count
    $sessionCreateCount = ([regex]::Matches($irText, "@__cursive_host_session_create")).Count
    $sessionDestroyCount = ([regex]::Matches($irText, "@__cursive_host_session_destroy")).Count
    $runtimeLeaveCount = ([regex]::Matches($irText, "@cursive_host_session_leave\(i64")).Count
    $runtimeEnterRetiredCount = ([regex]::Matches($irText, "@cursive_host_session_enter_retired\(i64")).Count
    $runtimeLeaveRetiredCount = ([regex]::Matches($irText, "@cursive_host_session_leave_retired\(i64")).Count
    $runtimeAbortLiveCount = ([regex]::Matches($irText, "@cursive_host_session_abort_live\(i64")).Count
    $runtimeAbortRetiredCount = ([regex]::Matches($irText, "@cursive_host_session_abort_retired\(i64")).Count
    $ownerTokenCount = ([regex]::Matches($irText, "@__cursive_host_session_owner_token")).Count
    $hostThunkCount = ([regex]::Matches($irText, "@hc_issue553_host_probe\(i64")).Count
    $hostThunkMatch = [regex]::Match(
        $irText,
        "(?ms)define\s+i32\s+@hc_issue553_host_probe\(i64[^)]*\)\s*\{(?<body>.*?)^\}")
    $hostRecordThunkMatch = [regex]::Match(
        $irText,
        "(?ms)define\s+void\s+@hc_issue553_host_probe_record\(ptr[^,]*,\s*i64[^)]*\)\s*\{(?<body>.*?)^\}")
    $hostReadThunkMatch = [regex]::Match(
        $irText,
        "(?ms)define\s+i32\s+@hc_issue553_host_read_body_state\(i64[^,]*,\s*ptr[^,]*,\s*ptr[^)]*\)\s*\{(?<body>.*?)^\}")
    $hostReadThunkSplitAggregate = [regex]::IsMatch(
        $irText,
        "define\s+i32\s+@hc_issue553_host_read_body_state\([^)]*\{\s*i64\s*,\s*i64\s*\}")
    $createBodyMatch = [regex]::Match(
        $irText,
        "(?ms)define\s+i64\s+@__cursive_host_session_create\(\)\s*\{(?<body>.*?)^\}")
    $destroyBodyMatch = [regex]::Match(
        $irText,
        "(?ms)define\s+i32\s+@__cursive_host_session_destroy\(i64[^)]*\)\s*\{(?<body>.*?)^\}")
    $registerDeclMatch = [regex]::IsMatch(
        $irText,
        "declare\s+i64\s+@cursive_host_session_register\(ptr,\s*ptr\)")
    $tryEnterDeclMatch = [regex]::IsMatch(
        $irText,
        "declare\s+i32\s+@cursive_host_session_try_enter\(i64,\s*ptr,\s*ptr\)")
    $leaveDeclMatch = [regex]::IsMatch(
        $irText,
        "declare\s+i32\s+@cursive_host_session_leave\(i64,\s*ptr\)")
    $retireDeclMatch = [regex]::IsMatch(
        $irText,
        "declare\s+i32\s+@cursive_host_session_try_retire\(i64,\s*ptr,\s*ptr\)")
    $abortLiveDeclMatch = [regex]::IsMatch(
        $irText,
        "declare\s+i32\s+@cursive_host_session_abort_live\(i64,\s*ptr,\s*ptr\)")
    $enterRetiredDeclMatch = [regex]::IsMatch(
        $irText,
        "declare\s+i32\s+@cursive_host_session_enter_retired\(i64,\s*ptr,\s*ptr\)")
    $leaveRetiredDeclMatch = [regex]::IsMatch(
        $irText,
        "declare\s+i32\s+@cursive_host_session_leave_retired\(i64,\s*ptr\)")
    $abortRetiredDeclMatch = [regex]::IsMatch(
        $irText,
        "declare\s+i32\s+@cursive_host_session_abort_retired\(i64,\s*ptr,\s*ptr\)")
    $hostBodySymbol = Get-CursiveHostBodySymbol @("probe", "host_probe")
    $ordinaryProcMatch = [regex]::Match(
        $irText,
        "define\s+i32\s+@probe_x3a_x3arun\((?<sig>[^)]*)\)")
    $hostSourceMatch = [regex]::Match(
        $irText,
        "define\s+i32\s+@" + [regex]::Escape($hostBodySymbol) + "\((?<sig>[^)]*)\)")
    if ($abiVersionCount -lt 1 -or $sessionCreateCount -lt 1 -or $sessionDestroyCount -lt 1) {
        throw "Case 'issue553_hosted_export_ir_wiring' expected hosted lifecycle exports in LLVM IR."
    }
    if ($irText -notmatch 'define\s+i32\s+@__cursive_host_session_destroy\(i64') {
        throw "Case 'issue553_hosted_export_ir_wiring' expected __cursive_host_session_destroy to return i32."
    }
    if ($hostThunkCount -lt 1) {
        throw "Case 'issue553_hosted_export_ir_wiring' expected a hosted export thunk with leading session handle."
    }
    if ($runtimeLeaveCount -lt 1) {
        throw "Case 'issue553_hosted_export_ir_wiring' expected hosted thunks and lifecycle exports to call the checked session leave helper."
    }
    if ($runtimeEnterRetiredCount -lt 1 -or $runtimeLeaveRetiredCount -lt 1) {
        throw "Case 'issue553_hosted_export_ir_wiring' expected hosted destroy lowering to use the retired-session helper pair."
    }
    if ($runtimeAbortLiveCount -lt 1 -or $runtimeAbortRetiredCount -lt 1) {
        throw "Case 'issue553_hosted_export_ir_wiring' expected hosted cleanup failure paths to use the abort-session helper pair."
    }
    if ($ownerTokenCount -lt 1) {
        throw "Case 'issue553_hosted_export_ir_wiring' expected an emitted hosted-session owner token global."
    }
    if (-not $registerDeclMatch -or
        -not $tryEnterDeclMatch -or
        -not $leaveDeclMatch -or
        -not $retireDeclMatch -or
        -not $abortLiveDeclMatch -or
        -not $enterRetiredDeclMatch -or
        -not $leaveRetiredDeclMatch -or
        -not $abortRetiredDeclMatch) {
        throw "Case 'issue553_hosted_export_ir_wiring' expected owner-aware hosted-session runtime helper declarations in LLVM IR."
    }
    if (-not $createBodyMatch.Success -or -not $destroyBodyMatch.Success -or -not $hostThunkMatch.Success) {
        throw "Case 'issue553_hosted_export_ir_wiring' expected hosted lifecycle exports and thunks to have analyzable LLVM IR bodies."
    }
    if ($createBodyMatch.Groups["body"].Value -notmatch '@__cursive_host_session_owner_token' -or
        $destroyBodyMatch.Groups["body"].Value -notmatch '@__cursive_host_session_owner_token' -or
        $hostThunkMatch.Groups["body"].Value -notmatch '@__cursive_host_session_owner_token') {
        throw "Case 'issue553_hosted_export_ir_wiring' expected lifecycle exports and hosted thunks to pass the module owner token through the runtime helper seam."
    }
    if ($createBodyMatch.Groups["body"].Value -match '@cursive_x3a_x3aruntime_x3a_x3apanic' -or
        $destroyBodyMatch.Groups["body"].Value -match '@cursive_x3a_x3aruntime_x3a_x3apanic') {
        throw "Case 'issue553_hosted_export_ir_wiring' lifecycle exports must not panic across the foreign boundary."
    }
    if ($hostThunkMatch.Groups["body"].Value -match '@cursive_x3a_x3aruntime_x3a_x3apanic') {
        throw "Case 'issue553_hosted_export_ir_wiring' catch-mode hosted thunk must return ZeroValue(R) on boundary failure, not panic."
    }
    if (-not $hostRecordThunkMatch.Success) {
        throw "Case 'issue553_hosted_export_ir_wiring' expected an aggregate-return hosted thunk with foreign sret ABI shape."
    }
    if (-not $hostReadThunkMatch.Success) {
        throw "Case 'issue553_hosted_export_ir_wiring' expected the hosted aggregate-parameter thunk to lower as (session, ptr, ptr) on Win64."
    }
    if ($hostReadThunkSplitAggregate) {
        throw "Case 'issue553_hosted_export_ir_wiring' unexpectedly split the by-value aggregate parameter into scalar lanes."
    }
    $hostRecordThunkBody = $hostRecordThunkMatch.Groups["body"].Value
    if ($hostRecordThunkBody -notmatch 'store\s+.+,\s+ptr\s+%0') {
        throw "Case 'issue553_hosted_export_ir_wiring' expected the aggregate-return thunk to copy the computed aggregate into the caller-provided sret slot."
    }
    if (-not $ordinaryProcMatch.Success) {
        throw "Case 'issue553_hosted_export_ir_wiring' expected the ordinary procedure to retain a separately emitted source ABI."
    }
    if (-not $hostSourceMatch.Success) {
        throw "Case 'issue553_hosted_export_ir_wiring' expected the hosted export source procedure to remain directly callable from Cursive code."
    }
    if ($ordinaryProcMatch.Groups["sig"].Value.Contains("__c0_host_env")) {
        throw "Case 'issue553_hosted_export_ir_wiring' incorrectly rewrote the ordinary procedure ABI with a hidden hosted-session parameter."
    }
    if ($hostSourceMatch.Groups["sig"].Value.Contains("__c0_host_env")) {
        throw "Case 'issue553_hosted_export_ir_wiring' incorrectly rewrote the hosted export source procedure ABI with a hidden hosted-session parameter."
    }
    $dllEntryPoint = Get-PeAddressOfEntryPoint $dllPath
    if ($dllEntryPoint -eq 0) {
        throw "Case 'issue553_hosted_export_ir_wiring' expected the shared library to carry a non-zero PE entrypoint."
    }
    $dllExports = Get-CoffExportNames $dllPath
    if (-not ($dllExports -contains "hc_issue553_host_probe")) {
        throw "Case 'issue553_hosted_export_ir_wiring' expected the hosted thunk export to be present in the DLL export table."
    }
    if (-not ($dllExports -contains "hc_issue553_host_read_body_state")) {
        throw "Case 'issue553_hosted_export_ir_wiring' expected the hosted aggregate-parameter thunk export to be present in the DLL export table."
    }
    if ($dllExports -contains $hostBodySymbol) {
        throw "Case 'issue553_hosted_export_ir_wiring' must not export the hosted source body symbol; foreign entry must remain thunk-only."
    }
    if ($dllExports -contains "__cursive_library_entry") {
        throw "Case 'issue553_hosted_export_ir_wiring' must not export the generated shared-library entrypoint."
    }
    $helperExports = @($dllExports | Where-Object { $_ -like "__cx_*" })
    if ($helperExports.Count -gt 0) {
        throw "Case 'issue553_hosted_export_ir_wiring' must not export generated helper symbols: $($helperExports -join ', ')"
    }
    $runtimeInitExports = @($dllExports | Where-Object { $_ -match '^cursive_x3a_x3aruntime_x3a_x3a(init|deinit)' })
    if ($runtimeInitExports.Count -gt 0) {
        throw "Case 'issue553_hosted_export_ir_wiring' must not export generated init/deinit symbols: $($runtimeInitExports -join ', ')"
    }
    if ($irText -notmatch '@__cursive_image_panic_record\s*=') {
        throw "Case 'issue553_hosted_export_ir_wiring' expected shared-library image panic storage in LLVM IR."
    }
    if ($irText -match '__c0_panic_record' -or $irText -match '__c0_library_panic') {
        throw "Case 'issue553_hosted_export_ir_wiring' must not materialize shared-library boundary panic state as fresh local records."
    }

    $requiresLibrary = Invoke-BuildWithConformance `
        -CaseId "issue553_host_export_requires_library" `
        -Source @'
record HostedProbeContext {
    fs: $FileSystem
}

[[host_export("C")]]
public procedure bad_export(ctx: HostedProbeContext) -> i32 {
    let _ = ctx
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@ `
        -Manifest $manifestLines `
        -ConformanceFileName "issue553_host_export_requires_library.log"
    Assert-BuildFailureResult -Result $requiresLibrary -CaseId "issue553_host_export_requires_library" -ExpectedCodes @("E-SYS-3357") -ForbiddenCodes @("E-PRJ-0210")

    $nonLibraryImportManifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""executable""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "",
        "[[assembly]]",
        "name = ""linkedlib""",
        "kind = ""library""",
        "root = ""linkedlib""",
        "out_dir = ""build/linkedlib"""
    )

    $nonLibraryImport = Invoke-BuildWithConformance `
        -CaseId "issue553_nonlibrary_host_export_library_import_allowed" `
        -Source @'
import linkedlib

record HostedProbeContext {
    fs: $FileSystem
}

[[host_export("C")]]
public procedure bad_export(ctx: HostedProbeContext) -> i32 {
    let _ = ctx
    return linkedlib::helper()
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return linkedlib::helper()
}
'@ `
        -Manifest $nonLibraryImportManifest `
        -ConformanceFileName "issue553_nonlibrary_host_export_library_import_allowed.log" `
        -ExtraArgs @("--assembly", "probe") `
        -ExtraFiles @{
            "linkedlib/Library.cursive" = @'
public procedure helper() -> i32 {
    return 7
}
'@
        }
    Assert-BuildFailureResult -Result $nonLibraryImport -CaseId "issue553_nonlibrary_host_export_library_import_allowed" -ExpectedCodes @("E-SYS-3357") -ForbiddenCodes @("E-PRJ-0210")

    $missingContext = Invoke-BuildWithConformance `
        -CaseId "issue553_host_export_missing_context_bundle" `
        -Source @'
[[host_export("C")]]
public procedure bad_export(value: i32) -> i32 {
    return value
}
'@ `
        -Manifest $libraryManifest `
        -ConformanceFileName "issue553_host_export_missing_context_bundle.log"
    Assert-BuildFailureResult -Result $missingContext -CaseId "issue553_host_export_missing_context_bundle" -ExpectedCodes @("E-TYP-2632")

    $rawContext = Invoke-BuildWithConformance `
        -CaseId "issue553_host_export_raw_context_rejected" `
        -Source @'
[[host_export("C")]]
public procedure bad_export(ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@ `
        -Manifest $libraryManifest `
        -AllowMissingConformance `
        -ConformanceFileName "issue553_host_export_raw_context_rejected.log"
    Assert-BuildFailureResult -Result $rawContext -CaseId "issue553_host_export_raw_context_rejected" -ExpectedCodes @("E-TYP-2636")

    $aliasedContext = Invoke-BuildWithConformance `
        -CaseId "issue553_host_export_aliased_context_rejected" `
        -Source @'
type HostedAlias = Context

[[host_export("C")]]
public procedure bad_export(ctx: HostedAlias) -> i32 {
    let _ = ctx
    return 0
}
'@ `
        -Manifest $libraryManifest `
        -AllowMissingConformance `
        -ConformanceFileName "issue553_host_export_aliased_context_rejected.log"
    Assert-BuildFailureResult -Result $aliasedContext -CaseId "issue553_host_export_aliased_context_rejected" -ExpectedCodes @("E-TYP-2636")

    $moveContext = Invoke-BuildWithConformance `
        -CaseId "issue553_host_export_move_context_rejected" `
        -Source @'
record HostedProbeContext {
    fs: $FileSystem
}

[[host_export("C")]]
public procedure bad_export(move ctx: HostedProbeContext) -> i32 {
    let _ = ctx
    return 0
}
'@ `
        -Manifest $libraryManifest `
        -AllowMissingConformance `
        -ConformanceFileName "issue553_host_export_move_context_rejected.log"
    Assert-BuildFailureResult -Result $moveContext -CaseId "issue553_host_export_move_context_rejected" -ExpectedCodes @("E-TYP-2633")

    $hostExportRegionReturn = Invoke-BuildWithConformance `
        -CaseId "issue553_host_export_region_local_raw_ptr_return_rejected" `
        -Source @'
record HostedProbeContext {
    fs: $FileSystem
}

[[host_export("C")]]
public procedure bad_export(ctx: HostedProbeContext) -> *imm i32 {
    let _ = ctx
    var escaped: *imm i32 = null
    region as r {
        let region_value: i32 = r ^ 7
        let region_ptr: Ptr<i32>@Valid = &region_value
        let region_raw: *imm i32 =
            unsafe { transmute<Ptr<i32>@Valid, *imm i32>(region_ptr) }
        escaped = region_raw
    }
    return escaped
}
'@ `
        -Manifest $libraryManifest `
        -AllowMissingConformance `
        -ConformanceFileName "issue553_host_export_region_local_raw_ptr_return_rejected.log"
    Assert-BuildFailureResult -Result $hostExportRegionReturn -CaseId "issue553_host_export_region_local_raw_ptr_return_rejected" -ExpectedCodes @("E-SYS-3360")

    $hostExportStackReturn = Invoke-BuildWithConformance `
        -CaseId "issue553_host_export_stack_raw_ptr_return_allowed" `
        -Source (New-Issue553HostExportStackRawPtrReturnAllowedSource) `
        -Manifest $libraryManifest `
        -ConformanceFileName "issue553_host_export_stack_raw_ptr_return_allowed.log"
    if ($hostExportStackReturn.ExitCode -ne 0) {
        throw "Case 'issue553_host_export_stack_raw_ptr_return_allowed' expected exit 0 but got $($hostExportStackReturn.ExitCode)."
    }
    $hostExportStackReturnErrors = @($hostExportStackReturn.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($hostExportStackReturnErrors -ne 0) {
        throw "Case 'issue553_host_export_stack_raw_ptr_return_allowed' expected zero compile-time errors, observed $hostExportStackReturnErrors."
    }

    $hostExportRegionEscapeStillRejected = Invoke-BuildWithConformance `
        -CaseId "issue553_host_export_region_local_raw_ptr_return_still_rejected_when_returning_stack" `
        -Source (New-Issue553HostExportRegionLocalRawPtrReturnStillRejectedWhenReturningStackSource) `
        -Manifest $libraryManifest `
        -AllowMissingConformance `
        -ConformanceFileName "issue553_host_export_region_local_raw_ptr_return_still_rejected_when_returning_stack.log"
    Assert-BuildFailureResult `
        -Result $hostExportRegionEscapeStillRejected `
        -CaseId "issue553_host_export_region_local_raw_ptr_return_still_rejected_when_returning_stack" `
        -ExpectedCodes @("E-MEM-3020") `
        -ForbiddenCodes @("E-SYS-3360")

    $genericHostExport = Invoke-BuildWithConformance `
        -CaseId "issue553_host_export_generic_rejected" `
        -Source @'
record HostedProbeContext {
    fs: $FileSystem
}

[[host_export("C")]]
public procedure bad_export<T>(ctx: HostedProbeContext, value: T) -> i32 {
    let _ = ctx
    let _ = value
    return 0
}
'@ `
        -Manifest $libraryManifest `
        -AllowMissingConformance `
        -ConformanceFileName "issue553_host_export_generic_rejected.log"
    Assert-BuildFailureResult -Result $genericHostExport -CaseId "issue553_host_export_generic_rejected" -ExpectedCodes @("E-TYP-2634")

    $catchNonZeroable = Invoke-BuildWithConformance `
        -CaseId "issue553_host_export_catch_nonzeroable_rejected" `
        -Source @'
record HostedProbeContext {
    fs: $FileSystem
}

[[layout(C)]]
enum CatchPayload {
    Value = 1
}

[[host_export("C-unwind"), unwind("catch")]]
public procedure bad_export(ctx: HostedProbeContext) -> CatchPayload {
    let _ = ctx
    return CatchPayload::Value
}
'@ `
        -Manifest $libraryManifest `
        -AllowMissingConformance `
        -ConformanceFileName "issue553_host_export_catch_nonzeroable_rejected.log"
    Assert-BuildFailureResult -Result $catchNonZeroable -CaseId "issue553_host_export_catch_nonzeroable_rejected" -ExpectedCodes @("E-TYP-2635")

    $catchRecordLayoutCRequired = Invoke-BuildWithConformance `
        -CaseId "issue553_host_export_catch_record_layout_c_required" `
        -Source @'
record HostedProbeContext {
    fs: $FileSystem
}

record CatchPayload {
    value: i32
}

[[host_export("C-unwind"), unwind("catch")]]
public procedure bad_export(ctx: HostedProbeContext) -> CatchPayload {
    let _ = ctx
    return CatchPayload { value: 1 }
}
'@ `
        -Manifest $libraryManifest `
        -AllowMissingConformance `
        -ConformanceFileName "issue553_host_export_catch_record_layout_c_required.log"
    Assert-BuildFailureResult -Result $catchRecordLayoutCRequired -CaseId "issue553_host_export_catch_record_layout_c_required" -ExpectedCodes @("E-TYP-2624")

    $mixedModes = Invoke-BuildWithConformance `
        -CaseId "issue553_host_export_mixed_modes_rejected" `
        -Source @'
record HostedProbeContext {
    fs: $FileSystem
}

[[export("C")]]
public procedure raw_export() -> i32 {
    return 0
}

[[host_export("C")]]
public procedure hosted_export(ctx: HostedProbeContext) -> i32 {
    let _ = ctx
    return 0
}
'@ `
        -Manifest $libraryManifest `
        -AllowMissingConformance `
        -ConformanceFileName "issue553_host_export_mixed_modes_rejected.log"
    Assert-BuildFailureResult -Result $mixedModes -CaseId "issue553_host_export_mixed_modes_rejected" -ExpectedCodes @("E-SYS-3358")

    $importRestrictionManifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""library""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "",
        "[[assembly]]",
        "name = ""linkedlib""",
        "kind = ""library""",
        "root = ""linkedlib""",
        "out_dir = ""build/linkedlib"""
    )

    $importRestriction = Invoke-BuildWithConformance `
        -CaseId "issue553_hosted_library_import_rejected" `
        -Source @'
import linkedlib

record HostedProbeContext {
    fs: $FileSystem
}

[[host_export("C")]]
public procedure hosted_export(ctx: HostedProbeContext) -> i32 {
    let _ = ctx
    return linkedlib::helper()
}
'@ `
        -Manifest $importRestrictionManifest `
        -AllowMissingConformance `
        -ConformanceFileName "issue553_hosted_library_import_rejected.log" `
        -ExtraArgs @("--assembly", "probe") `
        -ExtraFiles @{
            "linkedlib/Library.cursive" = @'
public procedure helper() -> i32 {
    return 0
}
'@
        }
    Assert-BuildFailureResult -Result $importRestriction -CaseId "issue553_hosted_library_import_rejected" -ExpectedCodes @("E-PRJ-0210")

    $transitiveHostedManifest = @(
        "[[assembly]]",
        "name = ""app""",
        "kind = ""executable""",
        "root = "".""",
        "out_dir = ""build/app""",
        "",
        "[[assembly]]",
        "name = ""hostedlib""",
        "kind = ""library""",
        "root = ""hostedlib""",
        "out_dir = ""build/hostedlib""",
        "",
        "[[assembly]]",
        "name = ""linkedlib""",
        "kind = ""library""",
        "root = ""linkedlib""",
        "out_dir = ""build/linkedlib"""
    )

    $transitiveHosted = Invoke-CheckWithConformance `
        -CaseId "issue553_hosted_library_transitive_import_allowed" `
        -Source @'
import hostedlib

public procedure main(move ctx: Context) -> i32 {
    return hostedlib::run(move ctx)
}
'@ `
        -Manifest $transitiveHostedManifest `
        -ConformanceFileName "issue553_hosted_library_transitive_import_allowed.log" `
        -AllowMissingConformance `
        -ExtraArgs @("--assembly", "app") `
        -ExtraFiles @{
            "hostedlib/Library.cursive" = @'
import linkedlib

record HostedProbeContext {
    fs: $FileSystem
}

[[host_export("C")]]
public procedure hosted_export(ctx: HostedProbeContext) -> i32 {
    let _ = ctx
    return linkedlib::helper()
}

public procedure run(move ctx: Context) -> i32 {
    let _ = ctx
    return linkedlib::helper()
}
'@;
            "linkedlib/Library.cursive" = @'
public procedure helper() -> i32 {
    return 11
}
'@
        }
    if ($transitiveHosted.ExitCode -ne 0) {
        throw "Case 'issue553_hosted_library_transitive_import_allowed' expected exit 0 but got $($transitiveHosted.ExitCode)."
    }
    $transitiveHostedErrors = @($transitiveHosted.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($transitiveHostedErrors -ne 0) {
        throw "Case 'issue553_hosted_library_transitive_import_allowed' expected zero errors but observed $transitiveHostedErrors."
    }
    $transitiveHostedDiag = @($transitiveHosted.DiagJson.diagnostics | Where-Object {
        $_.code -eq "E-PRJ-0210"
    }).Count
    if ($transitiveHostedDiag -ne 0) {
        throw "Case 'issue553_hosted_library_transitive_import_allowed' must not emit E-PRJ-0210 when the selected assembly is not hosted."
    }

    $transitiveHostedDump = Invoke-StdoutModeWithConformance `
        -CaseId "issue553_hosted_library_transitive_dump_project_allowed" `
        -Source @'
import hostedlib

public procedure main(move ctx: Context) -> i32 {
    return hostedlib::run(move ctx)
}
'@ `
        -Manifest $transitiveHostedManifest `
        -ConformanceFileName "issue553_hosted_library_transitive_dump_project_allowed.log" `
        -AllowMissingConformance `
        -ExtraArgs @("--assembly", "app", "--dump") `
        -ExtraFiles @{
            "hostedlib/Library.cursive" = @'
import linkedlib

record HostedProbeContext {
    fs: $FileSystem
}

[[host_export("C")]]
public procedure hosted_export(ctx: HostedProbeContext) -> i32 {
    let _ = ctx
    return linkedlib::helper()
}

public procedure run(move ctx: Context) -> i32 {
    let _ = ctx
    return linkedlib::helper()
}
'@;
            "linkedlib/Library.cursive" = @'
public procedure helper() -> i32 {
    return 11
}
'@
        }
    if ($transitiveHostedDump.ExitCode -ne 0) {
        throw "Case 'issue553_hosted_library_transitive_dump_project_allowed' expected exit 0 but got $($transitiveHostedDump.ExitCode)."
    }
    if ($transitiveHostedDump.StdoutText -notmatch 'project_root:\s+') {
        throw "Case 'issue553_hosted_library_transitive_dump_project_allowed' expected dump output to include project_root."
    }
    if ($transitiveHostedDump.StdoutText -notmatch 'assembly_name:\s+app') {
        throw "Case 'issue553_hosted_library_transitive_dump_project_allowed' expected dump output to target the app assembly."
    }
    if ($transitiveHostedDump.StdoutText -notmatch 'assemblies:\s+\[(?=.*app)(?=.*hostedlib)(?=.*linkedlib)') {
        throw "Case 'issue553_hosted_library_transitive_dump_project_allowed' expected dump output to enumerate app, hostedlib, and linkedlib assemblies."
    }
    if ($transitiveHostedDump.StdoutText -match 'E-PRJ-0210') {
        throw "Case 'issue553_hosted_library_transitive_dump_project_allowed' must not report E-PRJ-0210 when the selected assembly is not hosted."
    }

    $unrelatedHostedManifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""executable""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "",
        "[[assembly]]",
        "name = ""hostedlib""",
        "kind = ""library""",
        "root = ""hostedlib""",
        "out_dir = ""build/hostedlib""",
        "",
        "[[assembly]]",
        "name = ""linkedlib""",
        "kind = ""library""",
        "root = ""linkedlib""",
        "out_dir = ""build/linkedlib"""
    )

    $unrelatedHosted = Invoke-CheckWithConformance `
        -CaseId "issue553_unrelated_hosted_library_import_allowed" `
        -Source @'
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@ `
        -Manifest $unrelatedHostedManifest `
        -ConformanceFileName "issue553_unrelated_hosted_library_import_allowed.log" `
        -ExtraArgs @("--assembly", "probe") `
        -ExtraFiles @{
            "hostedlib/Library.cursive" = @'
import linkedlib

record HostedProbeContext {
    fs: $FileSystem
}

[[host_export("C")]]
public procedure hosted_export(ctx: HostedProbeContext) -> i32 {
    let _ = ctx
    return linkedlib::helper()
}
'@;
            "linkedlib/Library.cursive" = @'
public procedure helper() -> i32 {
    return 0
}
'@
        }
    if ($unrelatedHosted.ExitCode -ne 0) {
        throw "Case 'issue553_unrelated_hosted_library_import_allowed' expected exit 0 but got $($unrelatedHosted.ExitCode)."
    }
    $unrelatedHostedDiag = @($unrelatedHosted.DiagJson.diagnostics | Where-Object {
        $_.code -eq "E-PRJ-0210"
    }).Count
    if ($unrelatedHostedDiag -ne 0) {
        throw "Case 'issue553_unrelated_hosted_library_import_allowed' must not emit E-PRJ-0210 for an unreachable hosted library."
    }

    Write-Host "[compiler-static] issue553_hosted_libraries: abi=$abiVersionCount create=$sessionCreateCount destroy=$sessionDestroyCount thunk=$hostThunkCount"
}

function Invoke-Issue553LibraryInitCleanupAndLinkFlagsCase {
    $presentChecks = @(
        @{ Path = $canonicalSpecPath; Pattern = 'cleanup MUST be limited to the successfully initialized prefix'; Absolute = $true },
        @{ Path = $canonicalSpecPath; Pattern = '`?InitPanicHandle\(m\)`?\s+MUST NOT execute\s+the full\s+`?DeinitFn\(m\)`?\s+body\.'; Absolute = $true },
        @{ Path = "cursive\\src\\05_codegen\\checks\\checks.cpp"; Pattern = 'handle\.cleanup_ir\s*=\s*EmitCleanupOnPanic\(ActiveStaticInitCleanupPlan\(ctx\),\s*ctx\);' },
        @{ Path = "cursive\\src\\05_codegen\\globals\\init.cpp"; Pattern = 'ir_parts\.push_back\(StaticStoreIR\(item,\s*module_path,\s*binds\)\);\s*ir_parts\.push_back\(InitPanicHandle\(ModulePathString\(module_path\),\s*ctx\)\);\s*RegisterInitializedStaticCleanup\(module_path,\s*item,\s*ctx\);' },
        @{ Path = "cursive\\src\\05_codegen\\llvm\\llvm_emit.cpp"; Pattern = 'IRInitPanicHandle &handle' },
        @{ Path = "cursive\\src\\05_codegen\\llvm\\llvm_emit.cpp"; Pattern = 'StoreInitPanicRecord\(emitter,\s*&builder\);' },
        @{ Path = "cursive\\src\\05_codegen\\llvm\\llvm_emit.cpp"; Pattern = 'for \(std::size_t deinit_index = module_index; deinit_index > 0; --deinit_index\)' },
        @{ Path = "cursive\\src\\05_codegen\\cleanup\\cleanup.cpp"; Pattern = 'loaded_value\.kind = IRValue::Kind::Symbol;' },
        @{ Path = "cursive\\src\\01_project\\link.cpp"; Pattern = 'args\.push_back\("--shared"\);' },
        @{ Path = "cursive\\src\\01_project\\link.cpp"; Pattern = 'args\.push_back\("--entry=main"\);' },
        @{ Path = "cursive\\src\\01_project\\link.cpp"; Pattern = 'args\.push_back\("--nostdlib"\);' },
        @{ Path = "cursive\\src\\01_project\\link.cpp"; Pattern = 'DelayImpLibraryPath\(\)' },
        @{ Path = "cursive\\src\\01_project\\link.cpp"; Pattern = 'ObjectFormatOf\(plan\.target_profile\) == ObjectFormat::Coff' },
        @{ Path = "cursive\\src\\01_project\\link.cpp"; Pattern = 'BuildPosixLinkArgsWide' },
        @{ Path = "cursive\\src\\01_project\\link.cpp"; Pattern = 'LinkInputMatchesObjectFormat\(\*runtime_lib,\s*ObjectFormatOf\(project\.target_profile\)\)' },
        @{ Path = "cursive\\src\\01_project\\link.cpp"; Pattern = 'LinkerToolName\(project\.target_profile\)' },
        @{ Path = "cursive\\src\\01_project\\link.cpp"; Pattern = 'ArchiverToolName\(project\.target_profile\)' },
        @{ Path = "cursive\\src\\01_project\\link.cpp"; Pattern = 'RuntimeLibNameFor\(project\.target_profile\)' },
        @{ Path = "cursive\\src\\01_project\\link.cpp"; Pattern = 'sym_inputs\.push_back\(\*runtime_lib\);' },
        @{ Path = "cursive\\src\\01_project\\link.cpp"; Pattern = 'inputs\.push_back\(\*runtime_lib\);' },
        @{ Path = "cursive\\src\\01_project\\outputs.cpp"; Pattern = 'EmitUnsupportedArtifactDiagnostic' },
        @{ Path = "cursive\\src\\01_project\\outputs.cpp"; Pattern = 'ContainsHostExports' },
        @{ Path = "cursive\\src\\01_project\\outputs.cpp"; Pattern = 'SupportsSharedLibraries\(project\.target_profile\)' },
        @{ Path = "cursive\\src\\01_project\\outputs.cpp"; Pattern = 'SupportsHostedLibraries\(project\.target_profile\)' },
        @{ Path = "cursive\\src\\01_project\\target_profile.cpp"; Pattern = 'bool SupportsSharedLibraries\(TargetProfile profile\)' },
        @{ Path = "cursive\\src\\01_project\\target_profile.cpp"; Pattern = 'bool SupportsHostedLibraries\(TargetProfile profile\)' },
        @{ Path = "cursive\\src\\01_project\\ffi_library.cpp"; Pattern = 'ResolveLibraryDelayLoadNameForCurrentTarget' },
        @{ Path = "cursive\\src\\01_project\\tool_resolution.cpp"; Pattern = 'SPEC_RULE\("ResolveTool-Err-Archiver"\);' },
        @{ Path = "cursive\\src\\00_core\\windows_bundle.cpp"; Pattern = 'support_root / "runtime" / "cursive0_rt\.lib"' },
        @{ Path = "cursive\\src\\00_core\\windows_bundle.cpp"; Pattern = 'support_root / "lib" / "delayimp\.lib"' },
        @{ Path = "cursive\\src\\00_core\\windows_bundle.cpp"; Pattern = 'support_root / "tools"' },
        @{ Path = "cursive\\src\\00_core\\windows_bundle.cpp"; Pattern = 'Missing compiler sidecar file:' },
        @{ Path = "cursive\\src\\05_codegen\\llvm\\llvm_emit.cpp"; Pattern = 'appendToGlobalCtors\(\*module_, ctor_fn, 65535\);' },
        @{ Path = "cursive\\src\\05_codegen\\llvm\\llvm_emit.cpp"; Pattern = 'appendToGlobalDtors\(\*module_, dtor_fn, 65535\);' },
        @{ Path = "cursive\\runtime\\CMakeLists.txt"; Pattern = 'add_library\(cursive0_rt STATIC' },
        @{ Path = "cursive\\src\\CMakeLists.txt"; Pattern = 'CURSIVE_DELAYIMP_LIB_PATH' },
        @{ Path = "cursive\\src\\06_driver\\main.cpp"; Pattern = 'const auto imported_assembly = ResolveImportedAssemblyName\(' },
        @{ Path = "cursive\\src\\06_driver\\main.cpp"; Pattern = 'ResolveExternLibraryDelayLoadDlls\(' },
        @{ Path = "cursive\\src\\06_driver\\main.cpp"; Pattern = 'ValidateHostedLibraryImportGraph\(' },
        @{ Path = "cursive\\src\\01_project\\assembly_graph.cpp"; Pattern = 'SortStringsDeterministically\(libraries\);' },
        @{ Path = "cursive\\src\\01_project\\assembly_graph.cpp"; Pattern = 'while \(order\.size\(\) < libraries\.size\(\)\)' },
        @{ Path = "cursive\\src\\01_project\\assembly_graph.cpp"; Pattern = 'for \(const auto& candidate : libraries\)' },
        @{ Path = "cursive\\src\\01_project\\assembly_graph.cpp"; Pattern = 'for \(const auto& dep_name : ComputeDirectLibraryImports\(candidate, graph\)\)' }
    )

    $absentChecks = @(
        @{ Path = "cursive\\src\\01_project\\link.cpp"; Pattern = 'args\.push_back\("-shared"\);' },
        @{ Path = "cursive\\src\\01_project\\link.cpp"; Pattern = 'runtime_satisfied_by_extra_inputs' }
    )

    foreach ($check in $presentChecks) {
        $isAbsolute = $check.ContainsKey("Absolute") -and [bool]$check.Absolute
        $fullPath = if ($isAbsolute) { $check.Path } else { Join-Path $workspaceRoot $check.Path }
        if (-not (Test-Path $fullPath)) {
            throw "Case 'issue553_library_init_cleanup_and_link_flags' missing file: $fullPath"
        }
        $text = Get-Content -Path $fullPath -Raw
        if ($text -notmatch $check.Pattern) {
            throw "Case 'issue553_library_init_cleanup_and_link_flags' missing expected pattern '$($check.Pattern)' in $fullPath."
        }
    }

    foreach ($check in $absentChecks) {
        $fullPath = Join-Path $workspaceRoot $check.Path
        if (-not (Test-Path $fullPath)) {
            throw "Case 'issue553_library_init_cleanup_and_link_flags' missing file: $fullPath"
        }
        $text = Get-Content -Path $fullPath -Raw
        if ($text -match $check.Pattern) {
            throw "Case 'issue553_library_init_cleanup_and_link_flags' found forbidden pattern '$($check.Pattern)' in $fullPath."
        }
    }

    $libraryManifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""library""",
        "root = "".""",
        "out_dir = ""build/probe""",
        "emit_ir = ""ll"""
    )

    $build = Invoke-BuildWithConformance `
        -CaseId "issue553_library_init_cleanup_and_link_flags" `
        -Source @'
import stagec

public procedure hc_issue553_library_init_probe() -> i32 {
    return stagec::stagec_ready()
}
'@ `
        -Manifest $libraryManifest `
        -ConformanceFileName "issue553_library_init_cleanup_and_link_flags.log" `
        -ExtraFiles @{
            "stagea/Main.cursive" = @'
var stage_a_drop_count: i32 = 0

record StageAGuard {
    value: i32
    token: unique i32

    procedure drop(~!) -> () {
        stage_a_drop_count = stage_a_drop_count + 1
        return ()
    }
}

procedure BuildStageAGuard() -> StageAGuard {
    return StageAGuard { value: 1, token: 11 }
}

var stage_a_guard: StageAGuard = BuildStageAGuard()

public procedure stagea_ready() -> i32 {
    return stage_a_guard.value
}
'@;
            "stageb/Main.cursive" = @'
import stagea

var stage_b_guard_drop_count: i32 = 0
var stage_b_panic_drop_count: i32 = 0

record StageBGuard {
    value: i32
    token: unique i32

    procedure drop(~!) -> () {
        stage_b_guard_drop_count = stage_b_guard_drop_count + 1
        return ()
    }
}

procedure BuildStageBGuard() -> StageBGuard {
    let _ = stagea::stagea_ready()
    return StageBGuard { value: 2, token: 22 }
}

record StageBPanicGuard {
    value: i32
    token: unique i32

    procedure drop(~!) -> () {
        stage_b_panic_drop_count = stage_b_panic_drop_count + 1
        return ()
    }
}

procedure BuildStageBPanic() -> StageBPanicGuard {
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
'@;
            "stagec/Main.cursive" = @'
import stageb

var stage_c_drop_count: i32 = 0

record StageCGuard {
    value: i32
    token: unique i32

    procedure drop(~!) -> () {
        stage_c_drop_count = stage_c_drop_count + 1
        return ()
    }
}

procedure BuildStageCGuard() -> StageCGuard {
    let _ = stageb::stageb_ready()
    return StageCGuard { value: 3, token: 44 }
}

var stage_c_guard: StageCGuard = BuildStageCGuard()

public procedure stagec_ready() -> i32 {
    return stage_c_guard.value
}
'@
        }

    if ($build.ExitCode -ne 0) {
        throw "Case 'issue553_library_init_cleanup_and_link_flags' expected exit 0 but got $($build.ExitCode)."
    }
    $buildErrors = @($build.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($buildErrors -ne 0) {
        throw "Case 'issue553_library_init_cleanup_and_link_flags' expected zero errors but observed $buildErrors."
    }

    $stageaObj = Join-Path $build.CaseRoot "build\probe\obj\probe_x3a_x3astagea.obj"
    $stagebObj = Join-Path $build.CaseRoot "build\probe\obj\probe_x3a_x3astageb.obj"
    $probeDll = Join-Path $build.CaseRoot "build\probe\bin\probe.dll"
    foreach ($required in @(
        $probeDll,
        (Join-Path $build.CaseRoot "build\probe\lib\probe.lib"),
        (Join-Path $build.CaseRoot "build\probe\ir\probe.ll"),
        $stageaObj,
        $stagebObj
    )) {
        if (-not (Test-Path $required)) {
            throw "Case 'issue553_library_init_cleanup_and_link_flags' missing required artifact: $required"
        }
    }

    $stageaDisasm = Invoke-LlvmToolText `
        -ToolName "llvm-objdump.exe" `
        -ToolArgs @("-dr", $stageaObj) `
        -FailureLabel "Case 'issue553_library_init_cleanup_and_link_flags' stagea objdump"
    $stagebInitSymbol = "cursive_x3a_x3aruntime_x3a_x3ainit_x3a_x3aprobe_x3a_x3astageb"
    $stagebDeinitSymbol = "cursive_x3a_x3aruntime_x3a_x3adeinit_x3a_x3aprobe_x3a_x3astageb"
    $stagebDisasm = Invoke-LlvmToolText `
        -ToolName "llvm-objdump.exe" `
        -ToolArgs @("-dr", $stagebObj) `
        -FailureLabel "Case 'issue553_library_init_cleanup_and_link_flags' stageb objdump"
    $probeDisasm = Invoke-LlvmToolText `
        -ToolName "llvm-objdump.exe" `
        -ToolArgs @("-dr", (Join-Path $build.CaseRoot "build\probe\obj\probe.obj")) `
        -FailureLabel "Case 'issue553_library_init_cleanup_and_link_flags' probe objdump"
    $stagebInitDisasm = Get-ObjdumpSymbolBlock `
        -DisassemblyText $stagebDisasm `
        -Symbol $stagebInitSymbol `
        -CaseId "issue553_library_init_cleanup_and_link_flags"
    $stagebDeinitDisasm = Get-ObjdumpSymbolBlock `
        -DisassemblyText $stagebDisasm `
        -Symbol $stagebDeinitSymbol `
        -CaseId "issue553_library_init_cleanup_and_link_flags"

    if ($stagebInitDisasm -notmatch 'IMAGE_REL_AMD64_REL32\s+probe_x3a_x3astageb_x3a_x3aStageBGuard_x3a_x3adrop') {
        throw "Case 'issue553_library_init_cleanup_and_link_flags' expected the stageb init panic path to drop the initialized StageBGuard prefix."
    }
    if ($stagebInitDisasm -match 'IMAGE_REL_AMD64_REL32\s+probe_x3a_x3astageb_x3a_x3aStageBPanicGuard_x3a_x3adrop') {
        throw "Case 'issue553_library_init_cleanup_and_link_flags' must not drop StageBPanicGuard from the stageb init panic path before the static is initialized."
    }
    if ($stagebInitDisasm -match 'IMAGE_REL_AMD64_REL32\s+cursive_x3a_x3aruntime_x3a_x3adeinit_x3a_x3aprobe_x3a_x3astageb') {
        throw "Case 'issue553_library_init_cleanup_and_link_flags' must not lower stageb init panic cleanup by calling the full stageb deinit wrapper."
    }
    if ($stagebDeinitDisasm -notmatch 'IMAGE_REL_AMD64_REL32\s+probe_x3a_x3astageb_x3a_x3aStageBPanicGuard_x3a_x3adrop') {
        throw "Case 'issue553_library_init_cleanup_and_link_flags' expected the stageb deinit wrapper to drop StageBPanicGuard."
    }
    if ($stagebDeinitDisasm -notmatch 'IMAGE_REL_AMD64_REL32\s+probe_x3a_x3astageb_x3a_x3aStageBGuard_x3a_x3adrop') {
        throw "Case 'issue553_library_init_cleanup_and_link_flags' expected the stageb deinit wrapper to drop StageBGuard."
    }

    if ($probeDisasm -notmatch 'IMAGE_REL_AMD64_REL32\s+cursive_x3a_x3aruntime_x3a_x3adeinit_x3a_x3aprobe_x3a_x3astagea') {
        throw "Case 'issue553_library_init_cleanup_and_link_flags' expected the library entry object to deinitialize stagea on later module-init failure."
    }
    if ($probeDisasm -notmatch 'IMAGE_REL_AMD64_REL32\s+cursive_x3a_x3aruntime_x3a_x3adeinit_x3a_x3aprobe_x3a_x3astageb') {
        throw "Case 'issue553_library_init_cleanup_and_link_flags' expected the library entry object to deinitialize stageb on later module-init failure or unload."
    }

    $dllExports = Get-CoffExportNames $probeDll
    if ($dllExports -contains "__cursive_library_entry") {
        throw "Case 'issue553_library_init_cleanup_and_link_flags' must not export the generated library entrypoint."
    }

    Write-Host "[compiler-static] issue553_library_init_cleanup_and_link_flags: source_checks=ok emitted_cleanup=ok"
}

function Invoke-Issue553RawDylibAndPosixLifecycleConformanceCase {
    $rawDylib = Invoke-BuildWithConformance `
        -CaseId "issue553_raw_dylib_delay_load" `
        -Source @'
[[library(name: "kernel32", kind: "dylib")]]
extern "system" {
    [[mangle(none)]]
    procedure GetCurrentProcessId() -> u32
}

[[library(name: "user32", kind: "raw-dylib")]]
extern "system" {
    [[mangle(none)]]
    procedure GetSystemMetrics(index: i32) -> i32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let _ = unsafe { GetCurrentProcessId() }
    let _ = unsafe { GetSystemMetrics(0) }
    return 0
}
'@ `
        -ExtraArgs @("--link-debug") `
        -ConformanceFileName "issue553_raw_dylib_delay_load.log"

    if ($rawDylib.ExitCode -ne 0) {
        throw "Case 'issue553_raw_dylib_delay_load' expected exit 0 but got $($rawDylib.ExitCode)."
    }
    $rawDylibErrors = @($rawDylib.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($rawDylibErrors -ne 0) {
        throw "Case 'issue553_raw_dylib_delay_load' expected zero errors but observed $rawDylibErrors."
    }
    $linkDebugCommands = @(Get-LinkDebugCommands $rawDylib.StderrPath)
    $exeLinkCommand = ""
    if ($linkDebugCommands.Count -ge 1) {
        $exeLinkCommand = $linkDebugCommands[0]
        if ($exeLinkCommand -notmatch '/DELAYLOAD:USER32\.dll') {
            throw "Case 'issue553_raw_dylib_delay_load' expected linker-wide /DELAYLOAD:USER32.dll."
        }
        if ($exeLinkCommand -notmatch 'delayimp\.lib') {
            throw "Case 'issue553_raw_dylib_delay_load' expected delayimp.lib for raw-dylib delay-load imports."
        }
    }

    $exePath = Join-Path $rawDylib.CaseRoot "build\probe\bin\probe.exe"
    $objPath = Join-Path $rawDylib.CaseRoot "build\probe\obj\probe.obj"
    if (-not (Test-Path $exePath)) {
        throw "Case 'issue553_raw_dylib_delay_load' missing executable artifact: $exePath"
    }
    & Assert-FreshMapSidecar "issue553_raw_dylib_delay_load" $exePath $exeLinkCommand
    if (-not (Test-Path $objPath)) {
        throw "Case 'issue553_raw_dylib_delay_load' missing object artifact: $objPath"
    }

    $importText = Get-LlvmReadObjText `
        -Args @("--coff-imports", $exePath) `
        -FailureLabel "llvm-readobj --coff-imports '$exePath'"
    if ($importText -notmatch 'Name:\s+KERNEL32\.dll') {
        throw "Case 'issue553_raw_dylib_delay_load' expected a normal KERNEL32.dll import."
    }
    if ($importText -notmatch 'DelayImport\s*\{[\s\S]*?Name:\s+USER32\.dll') {
        throw "Case 'issue553_raw_dylib_delay_load' expected USER32.dll in the PE delay import table."
    }

    $objDisasm = Invoke-LlvmToolText `
        -ToolName "llvm-objdump.exe" `
        -ToolArgs @("-dr", $objPath) `
        -FailureLabel "Case 'issue553_raw_dylib_delay_load' main objdump"
    if ($objDisasm -match 'cursive_raw_dylib_resolve') {
        throw "Case 'issue553_raw_dylib_delay_load' must not reference the raw-dylib resolver wrapper."
    }
    if ($objDisasm -notmatch 'GetSystemMetrics') {
        throw "Case 'issue553_raw_dylib_delay_load' expected the generated object to reference GetSystemMetrics directly."
    }
    $rawDylibRun = Start-Process -FilePath $exePath -NoNewWindow -Wait -PassThru
    if ($rawDylibRun.ExitCode -ne 0) {
        throw "Case 'issue553_raw_dylib_delay_load' expected runtime exit 0 but got $($rawDylibRun.ExitCode)."
    }

    $rawKernel32 = Invoke-BuildWithConformance `
        -CaseId "issue553_raw_dylib_kernel32_delay_load" `
        -Source @'
[[library(name: "kernel32", kind: "raw-dylib")]]
extern "system" {
    [[mangle(none)]]
    procedure GetCurrentProcessId() -> u32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let _ = unsafe { GetCurrentProcessId() }
    return 0
}
'@ `
        -ExtraArgs @("--link-debug") `
        -ConformanceFileName "issue553_raw_dylib_kernel32_delay_load.log"

    if ($rawKernel32.ExitCode -ne 0) {
        throw "Case 'issue553_raw_dylib_kernel32_delay_load' expected exit 0 but got $($rawKernel32.ExitCode)."
    }
    $rawKernel32Errors = @($rawKernel32.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($rawKernel32Errors -ne 0) {
        throw "Case 'issue553_raw_dylib_kernel32_delay_load' expected zero errors but observed $rawKernel32Errors."
    }
    $kernel32LinkDebugCommands = @(Get-LinkDebugCommands $rawKernel32.StderrPath)
    $kernel32LinkCommand = ""
    if ($kernel32LinkDebugCommands.Count -ge 1) {
        $kernel32LinkCommand = $kernel32LinkDebugCommands[0]
        if ($kernel32LinkCommand -match '/DELAYLOAD:KERNEL32\.dll') {
            throw "Case 'issue553_raw_dylib_kernel32_delay_load' must not delay-load KERNEL32.dll because the PE delay-load helper resolves imports through kernel32 loader APIs."
        }
        if ($kernel32LinkCommand -match 'delayimp\.lib') {
            throw "Case 'issue553_raw_dylib_kernel32_delay_load' must not link delayimp.lib when KERNEL32.dll is the only raw-dylib import."
        }
    }

    $rawKernel32Exe = Join-Path $rawKernel32.CaseRoot "build\probe\bin\probe.exe"
    if (-not (Test-Path $rawKernel32Exe)) {
        throw "Case 'issue553_raw_dylib_kernel32_delay_load' missing executable artifact: $rawKernel32Exe"
    }
    & Assert-FreshMapSidecar "issue553_raw_dylib_kernel32_delay_load" $rawKernel32Exe $kernel32LinkCommand
    $rawKernel32ImportText = Get-LlvmReadObjText `
        -Args @("--coff-imports", $rawKernel32Exe) `
        -FailureLabel "llvm-readobj --coff-imports '$rawKernel32Exe'"
    if ($rawKernel32ImportText -notmatch 'Name:\s+KERNEL32\.dll') {
        throw "Case 'issue553_raw_dylib_kernel32_delay_load' expected a normal KERNEL32.dll import."
    }
    if ($rawKernel32ImportText -match 'DelayImport\s*\{[\s\S]*?Name:\s+KERNEL32\.dll') {
        throw "Case 'issue553_raw_dylib_kernel32_delay_load' must not place KERNEL32.dll in the PE delay import table."
    }
    $rawKernel32Run = Start-Process -FilePath $rawKernel32Exe -NoNewWindow -Wait -PassThru
    if ($rawKernel32Run.ExitCode -ne 0) {
        throw "Case 'issue553_raw_dylib_kernel32_delay_load' expected runtime exit 0 but got $($rawKernel32Run.ExitCode)."
    }

    $rawMsvcrt = Invoke-BuildWithConformance `
        -CaseId "issue553_raw_dylib_msvcrt_delay_load" `
        -Source @'
[[library(name: "msvcrt", kind: "raw-dylib")]]
extern "C" {
    [[mangle(none)]]
    procedure _getpid() -> i32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let _ = unsafe { _getpid() }
    return 0
}
'@ `
        -ExtraArgs @("--link-debug") `
        -ConformanceFileName "issue553_raw_dylib_msvcrt_delay_load.log"

    if ($rawMsvcrt.ExitCode -ne 0) {
        throw "Case 'issue553_raw_dylib_msvcrt_delay_load' expected exit 0 but got $($rawMsvcrt.ExitCode)."
    }
    $rawMsvcrtErrors = @($rawMsvcrt.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($rawMsvcrtErrors -ne 0) {
        throw "Case 'issue553_raw_dylib_msvcrt_delay_load' expected zero errors but observed $rawMsvcrtErrors."
    }
    $msvcrtLinkDebugCommands = @(Get-LinkDebugCommands $rawMsvcrt.StderrPath)
    $msvcrtLinkCommand = ""
    if ($msvcrtLinkDebugCommands.Count -ge 1) {
        $msvcrtLinkCommand = $msvcrtLinkDebugCommands[0]
        if ($msvcrtLinkCommand -notmatch "/DELAYLOAD:MSVCRT\.dll") {
            throw "Case 'issue553_raw_dylib_msvcrt_delay_load' expected linker-wide /DELAYLOAD:MSVCRT.dll."
        }
        if ($msvcrtLinkCommand -notmatch "delayimp\.lib") {
            throw "Case 'issue553_raw_dylib_msvcrt_delay_load' expected delayimp.lib for raw-dylib delay-load imports."
        }
    }

    $rawMsvcrtExe = Join-Path $rawMsvcrt.CaseRoot "build\probe\bin\probe.exe"
    if (-not (Test-Path $rawMsvcrtExe)) {
        throw "Case 'issue553_raw_dylib_msvcrt_delay_load' missing executable artifact: $rawMsvcrtExe"
    }
    & Assert-FreshMapSidecar "issue553_raw_dylib_msvcrt_delay_load" $rawMsvcrtExe $msvcrtLinkCommand
    $rawMsvcrtImportText = Get-LlvmReadObjText `
        -Args @("--coff-imports", $rawMsvcrtExe) `
        -FailureLabel "llvm-readobj --coff-imports '$rawMsvcrtExe'"
    if ($rawMsvcrtImportText -notmatch "DelayImport\s*\{[\s\S]*?Name:\s+MSVCRT\.dll") {
        throw "Case 'issue553_raw_dylib_msvcrt_delay_load' expected MSVCRT.dll in the PE delay import table."
    }
    $rawMsvcrtRun = Start-Process -FilePath $rawMsvcrtExe -NoNewWindow -Wait -PassThru
    if ($rawMsvcrtRun.ExitCode -ne 0) {
        throw "Case 'issue553_raw_dylib_msvcrt_delay_load' expected runtime exit 0 but got $($rawMsvcrtRun.ExitCode)."
    }


    $rawDylibMissing = Invoke-BuildWithConformance `
        -CaseId "issue553_raw_dylib_missing_library_rejected" `
        -Source @'
[[library(name: "doesnotexist_abcxyz", kind: "raw-dylib")]]
extern "system" {
    [[mangle(none)]]
    procedure MissingSymbol() -> i32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return unsafe { MissingSymbol() }
}
'@ `
        -ConformanceFileName "issue553_raw_dylib_missing_library_rejected.log"
    Assert-BuildFailureResult -Result $rawDylibMissing -CaseId "issue553_raw_dylib_missing_library_rejected" -ExpectedCodes @("E-SYS-3347")

    $sharedManifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""library""",
        "root = "".""",
        "out_dir = ""build/probe"""
    )

    $nonWindowsShared = Invoke-BuildWithConformance `
        -CaseId "issue553_nonwindows_shared_library_rejected" `
        -Source @'
public procedure exported_probe() -> i32 {
    return 17
}
'@ `
        -Manifest $sharedManifest `
        -ExtraArgs @("--target-profile", "x86_64-sysv") `
        -ConformanceFileName "issue553_nonwindows_shared_library_rejected.log"
    Assert-BuildFailureResult -Result $nonWindowsShared -CaseId "issue553_nonwindows_shared_library_rejected" -ExpectedCodes @("E-OUT-0409")

    $hostedStaticManifest = @(
        "[[assembly]]",
        "name = ""probe""",
        "kind = ""library""",
        "link_kind = ""static""",
        "root = "".""",
        "out_dir = ""build/probe"""
    )

    $hostedStatic = Invoke-BuildWithConformance `
        -CaseId "issue553_hosted_library_requires_shared_artifact" `
        -Source @'
record HostedProbeContext {
    fs: $FileSystem
}

[[host_export("C"), mangle("hc_issue553_static_probe")]]
public procedure hosted_export(ctx: HostedProbeContext) -> i32 {
    let _ = ctx.fs
    return 17
}
'@ `
        -Manifest $hostedStaticManifest `
        -ConformanceFileName "issue553_hosted_library_requires_shared_artifact.log"
    Assert-BuildFailureResult -Result $hostedStatic -CaseId "issue553_hosted_library_requires_shared_artifact" -ExpectedCodes @("E-OUT-0409")

    Write-Host "[compiler-static] issue553_raw_dylib_and_posix_lifecycle: raw_delayload=1 nonwindows_shared=rejected hosted_static=rejected"
}

try {
    Invoke-ConformanceCase -Id "default_cap_100" -ExtraArgs @() -ExpectExit 1 -MinErrors 100 -MaxErrors 100
    Invoke-RegistryCompletenessCase
    Invoke-ParseAbortCase
    Invoke-ResolveAbortCase
    Invoke-Issue541ResolveModulesParseErrorConformanceCase
    Invoke-MainMissingStaticUndefinedCase
    Invoke-ConformanceCase -Id "phase_flow_cap_1" -ExtraArgs @("--max-errors", "1") -ExpectExit 1 -MinErrors 1 -MaxErrors 1
    Invoke-ConformanceCase -Id "strict_cap_5" -ExtraArgs @("--max-errors", "5") -ExpectExit 1 -MinErrors 5 -MaxErrors 5
    Invoke-ConformanceCase -Id "unlimited_cap_inf" -ExtraArgs @("--max-errors", "inf") -ExpectExit 1 -MinErrors 101 -MaxErrors 10000
    Invoke-StaticCheckCase
    Invoke-Issue16TokenSpanTraceCase
    Invoke-Issue16CodeSelectionTraceCase
    Invoke-Issue16ExternalNoSpanCase
    Invoke-Issue16InternalSpanRetentionCase
    Invoke-Issue16OrderingIdentityCase
    Invoke-Issue16DiagnosticSpecSyncCase
    Invoke-Issue16InvalidCastCodeCase
    Invoke-Issue16NoLegacyDiagnosticApiCase
    Invoke-Issue16EmittedCodesCoveredBySpecCase
    Invoke-Issue26ManifestLlvmBinMissingNoFallbackCase
    Invoke-Issue26NoSiblingExternFallbackCase
    Invoke-Issue26RepoLlvmNoPathFallbackForLlvmAsCase
    Invoke-Issue26AssembleIrErrTransitionCase
    Invoke-Issue26LlvmAsWrongVersionRejectedCase
    Invoke-Issue26CliRejectsLlvmBinFlagCase
    Invoke-Issue66EmitLlvmErrLlTransitionCase
    Invoke-Issue66EmitLlvmErrLowerBoundaryConformanceCase
    Invoke-Issue66LlvmResolveBoundaryCase
    Invoke-ExpectedDiagCodeCase -Id "issue27_export_catch_requires_c_unwind" -Source (New-Issue27ExportCatchWrongAbiSource) -ExpectedCodes @("E-SYS-3355")
    Invoke-ExpectedDiagCodeCase -Id "issue27_export_unknown_abi" -Source (New-Issue27ExportUnknownAbiSource) -ExpectedCodes @("E-SYS-3352")
    Invoke-Issue27FfiSurfaceTraceCase
    Invoke-Issue27ImportUnwindCodegenTraceCase
    Invoke-ExpectedDiagCodeCase -Id "issue513_mangle_non_ffi_rejected" -Source (New-Issue513MangleNonFfiSource) -ExpectedCodes @("E-SYS-3340")
    Invoke-ExpectedDiagCodeCase -Id "issue513_mangle_invalid_rejected" -Source (New-Issue513MangleInvalidSource) -ExpectedCodes @("E-SYS-3341")
    Invoke-ExpectedDiagCodeCase -Id "issue513_dynamic_dispatch_non_vtable_method" -Source (New-Issue513DynamicDispatchEligibilitySource) -ExpectedCodes @("E-TYP-2540")
    Invoke-ExpectedDiagCodeCase -Id "issue513_unknown_extern_verification_attr_rejected" -Source (New-Issue513UnknownExternVerificationAttrSource) -ExpectedCodes @("E-MOD-2451")
    Invoke-ExpectedDiagCodeCase -Id "issue513_weak_attribute_rejected" -Source (New-Issue513WeakAttributeRejectedSource) -ExpectedCodes @("E-MOD-2451")
    Invoke-ExpectedDiagCodeCase -Id "issue513_extern_mangle_malformed_rejected" -Source (New-Issue513ExternMangleMalformedSource) -ExpectedCodes @("E-SYS-3341")
    Invoke-ExpectedDiagCodeCase -Id "issue513_extern_mangle_multi_arg_rejected" -Source (New-Issue513ExternMangleMultiArgRejectedSource) -ExpectedCodes @("E-SYS-3341")
    Invoke-ExpectedDiagCodeCase -Id "issue513_extern_duplicate_explicit_mangle_rejected" -Source (New-Issue513ExternDuplicateExplicitMangleSource) -ExpectedCodes @("E-SYS-3342")
    Invoke-ExpectedDiagCodeCase -Id "issue513_mangle_conflicting_directives_rejected" -Source (New-Issue513ConflictingMangleDirectivesSource) -ExpectedCodes @("E-SYS-3351")
    Invoke-ExpectedDiagCodeCase -Id "issue513_extern_mangle_conflicting_directives_rejected" -Source (New-Issue513ExternConflictingMangleDirectivesSource) -ExpectedCodes @("E-SYS-3351")
    Invoke-ExpectedWarningCodeCase -Id "issue513_mangle_none_export_c_redundant_warning" -Source (New-Issue513MangleNoneExportWarningSource) -ExpectedWarningCodes @("W-SYS-3350")
    Invoke-ExpectedWarningCodeCase -Id "issue513_unwind_abort_redundant_warning" -Source (New-Issue513UnwindAbortWarningSource) -ExpectedWarningCodes @("W-SYS-3355")
    Invoke-ExpectedDiagCodeCase -Id "issue513_library_unknown_kind_rejected" -Source (New-Issue513LibraryUnknownKindSource) -ExpectedCodes @("E-SYS-3346")
    Invoke-ExpectedDiagCodeCase -Id "issue513_library_framework_kind_rejected_for_target" -Source (New-Issue513LibraryUnsupportedFrameworkSource) -ExpectedCodes @("E-SYS-3346")
    Invoke-ExpectedSuccessCase -Id "issue513_library_known_kinds_accepted" -Source (New-Issue513LibraryKnownKindsAcceptedSource)
    Invoke-ExpectedDiagCodeCase -Id "issue513_library_missing_name_rejected" -Source (New-Issue513LibraryMissingNameSource) -ExpectedCodes @("E-MOD-2450")
    Invoke-ExpectedDiagCodeCase -Id "issue513_unwind_non_ffi_rejected" -Source (New-Issue513UnwindNonFfiProcedureSource) -ExpectedCodes @("E-SYS-3356")
    Invoke-ExpectedDiagCodeCase -Id "issue513_unwind_invalid_string_mode_rejected" -Source (New-Issue513UnwindInvalidStringModeSource) -ExpectedCodes @("E-SYS-3355")
    Invoke-ExpectedDiagCodeCaseWithForbiddenCodes -Id "issue513_unwind_catch_wrong_abi_rejected" -Source (New-Issue513UnwindCatchWrongAbiRejectedSource) -ExpectedCodes @("E-SYS-3355") -ForbiddenCodes @("E-FFI-0351")
    Invoke-ExpectedDiagCodeCase -Id "issue513_ffi_pass_by_value_malformed_rejected" -Source (New-Issue513FfiPassByValueMalformedSource) -ExpectedCodes @("E-MOD-2450")
    Invoke-ExpectedDiagCodeCase -Id "issue513_ffi_pass_by_value_wrong_target_rejected" -Source (New-Issue513FfiPassByValueWrongTargetSource) -ExpectedCodes @("E-MOD-2452")
    Invoke-ExpectedSuccessCase -Id "issue513_ffi_pass_by_value_enum_target_accepted" -Source (New-Issue513FfiPassByValueEnumAcceptedSource)
    Invoke-Issue513ExportByValueTraceCase
    Invoke-ExpectedSuccessCase -Id "issue513_export_generic_ffisafe_unused_param_accepted" -Source (New-Issue513ExportGenericFfiSafeUnusedParamAcceptedSource)
    Invoke-ExpectedDiagCodeCase -Id "issue513_export_generic_ffisafe_missing_predicate_rejected" -Source (New-Issue513ExportGenericFfiSafeMissingPredicateRejectedSource) -ExpectedCodes @("E-TYP-2629")
    Invoke-ExpectedDiagCodeCase -Id "issue513_extern_nested_context_field_rejected" -Source (New-Issue513ExternNestedContextFieldRejectedSource) -ExpectedCodes @("E-TYP-2626")
    Invoke-ExpectedDiagCodeCase -Id "issue513_keyblock_memory_order_speculative_rejected" -Source (New-Issue513KeyBlockMemoryOrderSpeculativeSource) -ExpectedCodes @("E-CON-0096")
    Invoke-ExpectedDiagCodeCase -Id "issue513_malformed_attr_syntax_rejected" -Source (New-Issue513MalformedAttrSyntaxSource) -ExpectedCodes @("E-MOD-2450")
    Invoke-ExpectedDiagCodeCaseWithForbiddenCodes -Id "issue513_derive_malformed_args_rejected" -Source (New-Issue513DeriveMalformedArgsSource) -ExpectedCodes @("E-MOD-2450") -ForbiddenCodes @("E-CTE-0310")
    Invoke-ExpectedDiagCodeCase -Id "issue513_vendor_scoped_unknown_rejected" -Source (New-Issue513VendorScopedUnknownSource) -ExpectedCodes @("E-MOD-2451")
    Invoke-ExpectedDiagCodeCase -Id "issue513_vendor_short_scoped_unknown_rejected" -Source (New-Issue513VendorShortScopedUnknownSource) -ExpectedCodes @("E-MOD-2451")
    Invoke-ExpectedDiagCodeCase -Id "issue513_vendor_dotted_unknown_rejected" -Source (New-Issue513VendorDottedUnknownSource) -ExpectedCodes @("E-MOD-2450")
    Invoke-ExpectedDiagCodeCase -Id "issue513_vendor_mixed_scoped_dotted_malformed_rejected" -Source (New-Issue513VendorMixedScopedDottedMalformedSource) -ExpectedCodes @("E-MOD-2450")
    Invoke-ExpectedDiagCodeCase -Id "issue513_reserved_cursive_attr_rejected" -Source (New-Issue513ReservedCursiveAttrSource) -ExpectedCodes @("E-CNF-0402")
    Invoke-ExpectedDiagCodeCaseWithForbiddenCodes -Id "issue513_reserved_cursive_attr_not_unknown" -Source (New-Issue513ReservedCursiveAttrSource) -ExpectedCodes @("E-CNF-0402") -ForbiddenCodes @("E-MOD-2451")
    Invoke-ExpectedDiagCodeCase -Id "issue513_static_non_foreign_rejected" -Source (New-Issue513StaticOnNonForeignProcedureSource) -ExpectedCodes @("E-MOD-2452")
    Invoke-ExpectedDiagCodeCase -Id "issue513_unknown_procedure_verification_attr_rejected" -Source (New-Issue513UnknownProcedureVerificationAttrSource) -ExpectedCodes @("E-MOD-2451")
    Invoke-ExpectedDiagCodeCase -Id "issue513_library_attr_expression_rejected" -Source (New-Issue513LibraryAttrOnExpressionSource) -ExpectedCodes @("E-SYS-3345")
    Invoke-ExpectedDiagCodeCase -Id "issue513_comptime_procedure_attr_target_rejected" -Source (New-Issue513ComptimeProcedureAttrTargetSource) -ExpectedCodes @("E-MOD-2452")
    Invoke-ExpectedDiagCodeCase -Id "issue513_comptime_expr_malformed_attr_rejected" -Source (New-Issue513ComptimeExprMalformedAttrSource) -ExpectedCodes @("E-MOD-2450")
    Invoke-ExpectedSuccessCase -Id "issue513_memory_order_default_expr_override" -Source (New-Issue513ExprMemoryOrderDefaultAndOverrideSource)
    Invoke-ExpectedSuccessCase -Id "issue513_memory_order_subtree_shared_access" -Source (New-Issue513ExprMemoryOrderSubtreeSharedAccessSource)
    Invoke-ExpectedDiagCodeCase -Id "issue513_memory_order_expr_duplicate_rejected" -Source (New-Issue513ExprMemoryOrderDuplicateRejectedSource) -ExpectedCodes @("E-MOD-2450")
    Invoke-ExpectedDiagCodeCase -Id "issue513_memory_order_keyblock_duplicate_rejected" -Source (New-Issue513KeyBlockMemoryOrderDuplicateRejectedSource) -ExpectedCodes @("E-MOD-2450")
    Invoke-ExpectedDiagCodeCase -Id "issue513_memory_order_expr_invalid_placement_rejected" -Source (New-Issue513ExprMemoryOrderInvalidPlacementSource) -ExpectedCodes @("E-MOD-2450")
    Invoke-ExpectedDiagCodeCase -Id "issue513_memory_order_expr_speculative_rejected" -Source (New-Issue513ExprMemoryOrderSpeculativeRejectedSource) -ExpectedCodes @("E-CON-0096")
    Invoke-ExpectedDiagCodeCase -Id "issue513_inline_hint_rejected" -Source (New-Issue513InlineHintRejectedSource) -ExpectedCodes @("E-MOD-2450")
    Invoke-Issue513InlineHintParseTraceCase
    Invoke-ExpectedDiagCodeCase -Id "issue513_cold_hint_rejected" -Source (New-Issue513ColdHintRejectedSource) -ExpectedCodes @("E-MOD-2450")
    Invoke-Issue513ColdHintParseTraceCase
    Invoke-ExpectedDiagCodeCase -Id "issue513_unwind_identifier_mode_rejected" -Source (New-Issue513UnwindIdentifierModeRejectedSource) -ExpectedCodes @("E-SYS-3355")
    Invoke-ExpectedDiagCodeCase -Id "issue513_extern_unwind_identifier_mode_rejected" -Source (New-Issue513ExternUnwindIdentifierModeRejectedSource) -ExpectedCodes @("E-SYS-3355")
    Invoke-ExpectedSuccessCase -Id "issue513_class_dynamic_inherited_method" -Source (New-Issue513ClassDynamicInheritedMethodSource)
    Invoke-ExpectedDiagCodeCase -Id "issue513_class_dynamic_static_override" -Source (New-Issue513ClassDynamicStaticOverrideSource) -ExpectedCodes @("E-MOD-2452")
    Invoke-ExpectedWarningCodeCase -Id "issue513_inline_always_address_taken_warning" -Source (New-Issue513InlineAlwaysAddressTakenWarningSource) -ExpectedWarningCodes @("W-MOD-2452")
    Invoke-ExpectedDiagCodeCase -Id "issue513_log_standalone_statement" -Source (New-Issue513LogStandaloneStatementSource) -ExpectedCodes @("E-MOD-2458")
    Invoke-ExpectedSuccessCase -Id "issue513_expr_log_newline_continuation" -Source (New-Issue513ExprLogNewlineContinuationSource)
    Invoke-ExpectedDiagCodeCase -Id "issue513_log_expected_moved_binding_rejected" -Source (New-Issue513LogExpectedMovedBindingSource) -ExpectedCodes @("E-MOD-2457")
    Invoke-ExpectedDiagCodeCase -Id "issue513_log_binding_expected_moved_rejected" -Source (New-Issue513LogBindingExpectedMovedSource) -ExpectedCodes @("E-MOD-2457")
    Invoke-ExpectedDiagCodeCase -Id "issue513_log_malformed_args_rejected" -Source (New-Issue513LogMalformedArgsSource) -ExpectedCodes @("E-MOD-2456")
    Invoke-ExpectedDiagCodeCaseWithForbiddenCodes -Id "issue513_log_positional_arg_rejected" -Source (New-Issue513LogPositionalArgSource) -ExpectedCodes @("E-MOD-2450") -ForbiddenCodes @("E-MOD-2456")
    Invoke-ExpectedSuccessCase -Id "issue513_log_multiple_args_allowed" -Source (New-Issue513LogMultipleArgsSource)
    Invoke-ExpectedDiagCodeCase -Id "issue513_log_never_return_rejected" -Source (New-Issue513LogNeverReturnSource) -ExpectedCodes @("E-MOD-2459")
    Invoke-ExpectedDiagCodeCase -Id "issue513_dynamic_clause_direct_rejected" -Source (New-Issue513DynamicClauseDirectSource) -ExpectedCodes @("E-CON-0410")
    Invoke-ExpectedDiagCodeCase -Id "issue513_dynamic_clause_repeated_attr_lists_rejected" -Source (New-Issue513DynamicClauseRepeatedAttrListsSource) -ExpectedCodes @("E-CON-0410")
    Invoke-ExpectedDiagCodeCase -Id "issue513_dynamic_type_alias_rejected" -Source (New-Issue513DynamicTypeAliasSource) -ExpectedCodes @("E-CON-0411")
    Invoke-ExpectedDiagCodeCase -Id "issue513_dynamic_field_target_rejected" -Source (New-Issue513DynamicFieldTargetSource) -ExpectedCodes @("E-CON-0412")
    Invoke-ExpectedWarningCodeCase -Id "issue513_dynamic_no_runtime_warning" -Source (New-Issue513DynamicNoRuntimeWarningSource) -ExpectedWarningCodes @("W-CON-0401")
    Invoke-Issue513BindingDeprecatedWarningCase
    Invoke-ExpectedWarningCodeCase -Id "issue513_method_deprecated_warning" -Source (New-Issue513MethodDeprecatedWarningSource) -ExpectedWarningCodes @("W-CNF-0601")
    Invoke-ExpectedWarningCodeCase -Id "issue513_field_deprecated_warning" -Source (New-Issue513FieldDeprecatedWarningSource) -ExpectedWarningCodes @("W-CNF-0601")
    Invoke-ExpectedWarningCodeCase -Id "issue513_record_deprecated_warning" -Source (New-Issue513RecordDeprecatedWarningSource) -ExpectedWarningCodes @("W-CNF-0601")
    Invoke-ExpectedWarningCodeCase -Id "issue513_enum_deprecated_warning" -Source (New-Issue513EnumDeprecatedWarningSource) -ExpectedWarningCodes @("W-CNF-0601")
    Invoke-ExpectedDiagCodeCase -Id "issue5131_attr_test_unknown_rejected" -Source (New-Issue5131AttrTestUnknownSource) -ExpectedCodes @("E-MOD-2451")
    Invoke-ExpectedDiagCodeCase -Id "issue5131_attr_bench_unknown_rejected" -Source (New-Issue5131AttrBenchUnknownSource) -ExpectedCodes @("E-MOD-2451")
    Invoke-ExpectedDiagCodeCase -Id "issue5131_attr_debug_contract_unknown_rejected" -Source (New-Issue5131AttrDebugContractUnknownSource) -ExpectedCodes @("E-MOD-2451")
    Invoke-ExpectedDiagCodeCase -Id "issue5131_attr_trailing_comma_single_line" -Source (New-Issue5131AttrTrailingCommaSingleLineSource) -ExpectedCodes @("E-SRC-0521")
    Invoke-ExpectedDiagCodeCase -Id "issue5131_attr_spec_trailing_comma_single_line" -Source (New-Issue5131AttrSpecTrailingCommaSingleLineSource) -ExpectedCodes @("E-SRC-0521")
    Invoke-ExpectedSuccessCase -Id "issue5131_attr_spec_trailing_comma_multiline" -Source (New-Issue5131AttrSpecTrailingCommaMultilineSource)
    Invoke-ExpectedDiagCodeCase -Id "issue5131_vendor_dot_colon_policy" -Source (New-Issue5131VendorDotColonPolicySource) -ExpectedCodes @("E-MOD-2450")
    Invoke-ExpectedSuccessCase -Id "issue5131_method_inline_allowed" -Source (New-Issue5131MethodInlineAllowedSource)
    Invoke-ExpectedSuccessCase -Id "issue5131_method_deprecated_allowed" -Source (New-Issue5131MethodDeprecatedAllowedSource)
    Invoke-ExpectedDiagCodeCase -Id "issue5131_method_mangle_rejected" -Source (New-Issue5131MethodMangleRejectedSource) -ExpectedCodes @("E-MOD-2452")
    Invoke-ExpectedSuccessCase -Id "issue5131_multi_attr_lists_item_concat_allowed" -Source (New-Issue5131MultiAttrListsItemConcatAllowedSource)
    Invoke-ExpectedSuccessCase -Id "issue5131_method_cold_allowed" -Source (New-Issue5131MethodColdAllowedSource)
    Invoke-ExpectedSuccessCase -Id "issue5131_method_dynamic_allowed" -Source (New-Issue5131MethodDynamicAllowedSource)
    Invoke-ExpectedSuccessCase -Id "issue5131_method_log_allowed" -Source (New-Issue5131MethodLogAllowedSource)
    Invoke-ExpectedDiagCodeCase -Id "issue5131_method_static_rejected" -Source (New-Issue5131MethodStaticRejectedSource) -ExpectedCodes @("E-MOD-2452")
    Invoke-Issue5131AttrSpecTrailingCommaConformanceCase
    Invoke-Issue5131AttrSpecTrailingCommaSingleLineTraceCase
    Invoke-Issue513StaleWarningCase
    Invoke-Issue513DualPathDuplicateSymbolWiringCase
    Invoke-Issue513LlvmAbiInlineWiringCase
    Invoke-Issue513LayoutAlignWarningCase
    Invoke-Issue513LayoutLlWiringCase
    Invoke-NoAmbientExplicitContextFlowCase
    Invoke-ExpectedSuccessCase -Id "issue32_unsuffixed_float_default_f32" -Source (New-Issue32UnsuffixedFloatDefaultF32Source)
    Invoke-ExpectedSuccessCase -Id "issue32_declared_float_type_precedence" -Source (New-Issue32DeclaredFloatTypePrecedenceSource)
    Invoke-ExpectedSuccessCase -Id "issue32_untyped_int_literal_not_float" -Source (New-Issue32UntypedIntLiteralNotFloatSource)
    Invoke-ExpectedFailureCase -Id "issue32_decimal_to_int_rejected" -Source (New-Issue32DecimalToIntRejectedSource)
    Invoke-ExpectedDiagCodeCase -Id "issue32_explicit_float_suffix_mismatch" -Source (New-Issue32ExplicitSuffixMismatchSource) -ExpectedCodes @("E-TYP-1531")
    Invoke-ExpectedSuccessCase -Id "issue32_tuple_access_dot_disambiguation" -Source (New-Issue32TupleAccessDotDisambiguationSource)
    Invoke-Issue514TupleExprSingletonCommaRejectedCase
    Invoke-Issue547TupleLoweringTraceCase
    Invoke-Issue548TupleAccessEvalSigmaCase
    Invoke-Issue549ArrayIndexConformanceCase
    Invoke-Issue560IfStmtNonUnitBranchDiagnosticCase
    Invoke-Issue554CallTempNoProvenanceCase
    Invoke-Issue550ArrayEvalSigmaCase
    Invoke-Issue551IndexEvalSigmaCase
    Invoke-Issue554IfCaseTempOwnershipCase
    Invoke-Issue555SliceIndexEvalSigmaCase
    Invoke-Issue556RangeIndexEvalSigmaCase
    Invoke-Issue557RangeEvalSigmaCase
    Invoke-Issue558RecordEvalSigmaCase
    Invoke-ExpectedDiagCodeCase -Id "issue32_import_std_reserved" -Source (New-Issue32ImportStdSource) -ExpectedCodes @("E-MOD-1202")
    Invoke-ExpectedDiagCodeCase -Id "issue32_using_std_reserved" -Source (New-Issue32UsingStdSource) -ExpectedCodes @("E-MOD-1204")
    Invoke-Issue32NoStdReservedBypassCase
    Invoke-ExpectedDiagCodeCase -Id "issue51_using_list_duplicate_name" -Source (New-Issue51UsingListDuplicateNameSource) -ExpectedCodes @("E-MOD-1206")
    Invoke-ExpectedDiagCodeCase -Id "issue51_using_list_duplicate_alias" -Source (New-Issue51UsingListDuplicateAliasSource) -ExpectedCodes @("E-MOD-1206")
    Invoke-ExpectedDiagCodeCase -Id "issue51_using_list_self_alias_collision" -Source (New-Issue51UsingListSelfAliasCollisionSource) -ExpectedCodes @("E-MOD-1206")
    Invoke-ExpectedDiagCodeCase -Id "issue51_using_bare_path_rejected" -Source (New-Issue51UsingBarePathRejectedSource) -ExpectedCodes @("E-SRC-0520")
    Invoke-ExpectedDiagCodeCase -Id "issue51_using_bare_alias_rejected" -Source (New-Issue51UsingBareAliasRejectedSource) -ExpectedCodes @("E-SRC-0520")
    Invoke-ExpectedDiagCodeCase -Id "issue51_using_self_ordinary_name" -Source (New-Issue51UsingSelfOrdinaryNameSource) -ExpectedCodes @("E-MOD-1204")
    Invoke-ExpectedSuccessCase -Id "issue51_public_extern_block_visibility_parse" -Source (New-Issue51PublicExternBlockVisibilitySource)
    Invoke-ExpectedDiagCodeCase -Id "issue51_extern_proc_terminator_required" -Source (New-Issue51ExternProcTerminatorRequiredSource) -ExpectedCodes @("E-SRC-0510")
    Invoke-Issue51UsingItemParseTraceCase
    Invoke-Issue51UsingListParseTraceCase
    Invoke-Issue51UsingWildcardParseTraceCase
    Invoke-Issue51PublicUsingItemVisibilityCase
    Invoke-Issue51FfiAbiProfileConformanceCase
    Invoke-Issue33FixedIdentifiersCoverageCase
    Invoke-Issue33FixedIdentTokenPolicyCase
    Invoke-Issue33TypeWhereKeywordPolicyCase
    Invoke-Issue33UsingImportAttrWiringCase
    Invoke-Issue33ImportParseAttrListConformanceCase
    Invoke-Issue619ImportDeclSurfaceConformanceCase
    Invoke-Issue622UsingDeclAttributeListConformanceCase
    Invoke-Issue623UsingItemSurfaceConformanceCase
    Invoke-Issue624UsingDeclOptionalAttrSurfaceConformanceCase
    Invoke-Issue625StaticDeclParseAttrListConformanceCase
    Invoke-Issue627ExternBlockShellConformanceCase
    Invoke-Issue628PathStringSurfaceConformanceCase
    Invoke-Issue629StringOfPathRefSurfaceConformanceCase
    Invoke-Issue630ModalRefSurfaceConformanceCase
    Invoke-Issue626StaticDeclOptionalAttrSurfaceConformanceCase
    Invoke-Issue33AstTypeWiringCase
    Invoke-Issue33ExprWiringCase
    Invoke-Issue33QualifiedNamePhaseBoundaryWiringCase
    Invoke-Issue33QualifiedNameResolutionPipelineTraceCase
    Invoke-ExpectedFailureCase -Id "issue33_class_member_terminator_required" -Source (New-Issue33ClassMemberTerminatorRequiredSource)
    Invoke-ExpectedSuccessCase -Id "issue33_nested_generic_close" -Source (New-Issue33NestedGenericCloseSource)
    Invoke-ExpectedSuccessCase -Id "issue33_enum_item_separated" -Source (New-Issue33EnumItemSeparatedSource)
    Invoke-ExpectedSuccessCase -Id "issue33_enum_semicolon_separated" -Source (New-Issue33EnumSemicolonSeparatedSource)
    Invoke-ExpectedDiagCodeCase -Id "issue33_enum_comma_separated_rejected" -Source (New-Issue33EnumCommaSeparatedRejectedSource) -ExpectedCodes @("E-SRC-0520")
    Invoke-ExpectedDiagCodeCase -Id "issue33_enum_missing_terminator" -Source (New-Issue33EnumMissingTerminatorSource) -ExpectedCodes @("E-SRC-0510")
    Invoke-ExpectedDiagCodeCase -Id "issue33_import_attr_multiple_blocks_rejected" -Source (New-Issue33ImportAttrMultipleBlocksRejectedSource) -ExpectedCodes @("E-MOD-2452")
    Invoke-ExpectedDiagCodeCase -Id "issue33_static_attr_target_rejected" -Source (New-Issue33StaticAttrTargetRejectedSource) -ExpectedCodes @("E-MOD-2452")
    Invoke-ExpectedDiagCodeCaseWithForbiddenCodes `
        -Id "issue625_static_decl_attribute_list_multiple_blocks" `
        -Source (New-Issue625StaticDeclAttributeListMultipleBlocksSource) `
        -ExpectedCodes @("E-MOD-2452") `
        -ForbiddenCodes @("E-SRC-0520", "E-SRC-0510")
    Invoke-ExpectedDiagCodeCase -Id "issue33_class_duplicate_param_names" -Source (New-Issue33ClassDuplicateParamNamesSource) -ExpectedCodes @("E-SEM-2713")
    Invoke-ExpectedDiagCodeCase -Id "issue33_class_self_param_forbidden" -Source (New-Issue33ClassSelfParamForbiddenSource) -ExpectedCodes @("E-SEM-3011")
    Invoke-ExpectedDiagCodeCase -Id "issue33_modal_duplicate_param_names" -Source (New-Issue33ModalDuplicateParamNamesSource) -ExpectedCodes @("E-SEM-2713")
    Invoke-ExpectedDiagCodeCase -Id "issue33_modal_self_param_forbidden" -Source (New-Issue33ModalSelfParamForbiddenSource) -ExpectedCodes @("E-SEM-3011")
    Invoke-ExpectedDiagCodeCase -Id "issue33_enum_missing_class_method_impl" -Source (New-Issue33EnumMissingClassMethodImplSource) -ExpectedCodes @("E-TYP-2503")
    Invoke-ExpectedSuccessCase -Id "issue33_record_associated_type_member" -Source (New-Issue33RecordAssociatedTypeMemberSource)
    Invoke-ExpectedSuccessCase -Id "issue33_call_type_args_explicit" -Source (New-Issue33CallTypeArgsExplicitSource)
    Invoke-ExpectedSuccessCase -Id "issue33_range_type_family_surface" -Source (New-Issue33RangeTypeFamilySource)
    Invoke-ExpectedSuccessCase -Id "issue33_transmute_angle_syntax" -Source (New-Issue33TransmuteAngleSyntaxSource)
    Invoke-ExpectedFailureCase -Id "issue33_transmute_coloncolon_rejected" -Source (New-Issue33TransmuteColonColonRejectedSource)
    Invoke-Issue33AllocTypingTraceCase
    Invoke-Issue33AllocImplicitNoRegionCase
    Invoke-Issue33AllocExplicitFrozenRegionCase
    Invoke-Issue33AllocLoweringTraceCase
    Invoke-Issue33ExprHelperTraceCase
    Invoke-Issue52WiringGapCase
    Invoke-Issue52ExportVisErrTraceCase
    Invoke-Issue52ExportUnknownAbiTraceCase
    Invoke-Issue52ExternUnknownAbiTraceCase
    Invoke-Issue52ExternGenericErrTraceCase
    Invoke-Issue52ExternByValueErrTraceCase
    Invoke-Issue52WfExternBlockTraceCase
    Invoke-Issue544ExternItemListParseTraceCase
    Invoke-Issue52SynCallErrTraceCase
    Invoke-Issue52PackedFieldRefArgGapCase
    Invoke-Issue52PackedFieldRefArgUnsafeCase
    Invoke-Issue52GenericDefaultTypeArgsCase
    Invoke-Issue52IfCaseTraceCase
    Invoke-Issue52IfCaseClauseUnreachableIrrefutableCase
    Invoke-Issue52IfCaseClauseUnreachableEnumDuplicateCase
    Invoke-Issue52IfCaseClauseUnreachableUnionDuplicateCase
    Invoke-Issue52SolveStmtInferenceTraceCase
    Invoke-Issue52FieldAccessRecordTraceCase
    Invoke-Issue52FieldAccessEnumDiagCase
    Invoke-Issue52RangeLiftTraceCase
    Invoke-Issue52TransmuteInvalidTargetWarningCase
    Invoke-Issue52AsyncTryTraceCase
    Invoke-Issue52AsyncTryInfallibleErrCase
    Invoke-Issue52TransitionBorrowTraceCase
    Invoke-Issue52MetatheoryHooksCase
    Invoke-Issue561RulePremisesRegistryConformanceCase
    Invoke-Issue562UnicodeScalarDomainConformanceCase
    Invoke-Issue563ScalarsSequenceConformanceCase
    Invoke-Issue564StringAliasConformanceCase
    Invoke-Issue565NormalizeOutsideIdentifiersConformanceCase
    Invoke-Issue566SourceScalarsProjectionConformanceCase
    Invoke-Issue567LexSensitivePosConformanceCase
    Invoke-Issue568LiteralSpanConformanceCase
    Invoke-Issue569TokenizePartialSurfaceConformanceCase
    Invoke-Issue570RequiredTerminatorSurfaceConformanceCase
    Invoke-Issue571ContinuesLineSurfaceConformanceCase
    Invoke-Issue572SourceLoadStateConformanceCase
    Invoke-Issue573StepSizeTransitionConformanceCase
    Invoke-Issue574StepDecodeTransitionConformanceCase
    Invoke-Issue575StepDecodeErrorTransitionConformanceCase
    Invoke-Issue576StepBomTransitionConformanceCase
    Invoke-Issue577StepNormTransitionConformanceCase
    Invoke-Issue578StepEmbeddedBomErrorTransitionConformanceCase
    Invoke-Issue579StepLineMapTransitionConformanceCase
    Invoke-Issue580StepProhibitedTransitionConformanceCase
    Invoke-Issue581StepProhibitedErrorTransitionConformanceCase
    Invoke-Issue582SpanTempSourceConformanceCase
    Invoke-Issue583SpanAtLineStartConformanceCase
    Invoke-Issue584LexerInputProjectionConformanceCase
    Invoke-Issue585TokenEofConformanceCase
    Invoke-Issue586TokenRangeConformanceCase
    Invoke-Issue610ParserTokEofConformanceCase
    Invoke-Issue611ParserEofSpanHelperConformanceCase
    Invoke-Issue612ParserTokensBetweenConformanceCase
    Invoke-Issue613ParserInitialStateConformanceCase
    Invoke-Issue614ParseItemsEmptyEofOnlyConformanceCase
    Invoke-Issue615DocSeqSurfaceConformanceCase
    Invoke-Issue616ItemSeqSurfaceConformanceCase
    Invoke-Issue618ParseItemsConsTraceConformanceCase
    Invoke-Issue619Phase1DiagRulesSurfaceConformanceCase
    Invoke-Issue620QuoteProbeParseSyntaxErrConformanceCase
    Invoke-Issue621ParseSyntaxErrPremisesHoldConformanceCase
    Invoke-Issue587TokenInCommentConformanceCase
    Invoke-Issue588ScalarIndexConformanceCase
    Invoke-Issue589LexemeScalarSliceConformanceCase
    Invoke-Issue590ReservedNamespacePrefixConformanceCase
    Invoke-Issue591UniverseProtectedSetConformanceCase
    Invoke-Issue592BlockStateConformanceCase
    Invoke-Issue593AtHelperConformanceCase
    Invoke-Issue594RemoveHelperConformanceCase
    Invoke-Issue595ConcatHelperConformanceCase
    Invoke-Issue596ConcatSingletonConformanceCase
    Invoke-Issue597ConcatRecursiveConformanceCase
    Invoke-Issue598HexValueSequenceConformanceCase
    Invoke-Issue599DecDigitValueConformanceCase
    Invoke-Issue600OctDigitValueConformanceCase
    Invoke-Issue601BinDigitValueConformanceCase
    Invoke-Issue602DecValueSequenceConformanceCase
    Invoke-Issue603OctValueSequenceConformanceCase
    Invoke-Issue604BinValueSequenceConformanceCase
    Invoke-Issue605SuffixMatchConformanceCase
    Invoke-Issue606HasDotConformanceCase
    Invoke-Issue607HasExpConformanceCase
    Invoke-Issue608HasFloatCoreConformanceCase
    Invoke-Issue609DecimalLeadingZeroConformanceCase
    Invoke-Issue5131SpecCanonicalityCase
    Invoke-Issue53ClassMethodWfTraceCase
    Invoke-Issue53ClassCycleTraceCase
    Invoke-Issue53RecordMethodSemanticsTraceCase
    Invoke-Issue53RecordMethodCallTraceCase
    Invoke-Issue54ModalDefinitionsTraceCase
    Invoke-Issue54ModalStateIntroTraceCase
    Invoke-Issue54ModalClassParseTraceCase
    Invoke-Issue54RegionSurfaceConformanceCase
    Invoke-Issue54RegionUnsafeGateTraceCase
    Invoke-Issue54RegionAllocLoweringTraceCase
    Invoke-Issue54RegionResetLoweringTraceCase
    Invoke-Issue54RegionFreezeThawFreeLoweringTraceCase
    Invoke-Issue54ResolveDuplicateBindingDiagnosticCase
    Invoke-Issue55StateSpecificFieldConformanceCase
    Invoke-Issue56TransitionsAndMethodsConformanceCase
    Invoke-Issue56VisibilityConformanceCase
    Invoke-Issue617ProtectedVisibilityRejectedCase
    Invoke-Issue621ReservedKeywordIdentifierConformanceCase
    Invoke-Issue57LexicalIdentifierSecurityConformanceCase
    Invoke-Issue58StringBytesConformanceCase
    Invoke-Issue59CapabilitiesAndContextConformanceCase
    Invoke-Issue60TypeAnnotOptParseTraceCase
    Invoke-Issue61KeyPathResolutionConformanceCase
    Invoke-Issue514ListSmallStepParseTraceCase
    Invoke-Issue514TrailingCommaEndSetConformanceCase
    Invoke-Issue514TrailingCommaErrConformanceCase
    Invoke-ConsumeStateSurfaceConformanceCase
    Invoke-Issue514TrailingCommaEndSetConformanceCase
    Invoke-Issue510EnumDiscriminantDefaultsConformanceCase
    Invoke-Issue559EnumEmptyConformanceCase
    Invoke-Issue511FoundationalClassesAndPipeConformanceCase
    Invoke-Issue512InitializationPlanningConformanceCase
    Invoke-Issue545ResolveModulePathDirectConformanceCase
    Invoke-Issue546ImportPathAndCoverageConformanceCase
    Invoke-Issue552LinkKindManifestConformanceCase
    Invoke-Issue552ManifestNameProjectionConformanceCase
    Invoke-Issue552AssemblyGraphConformanceCase
    Invoke-Issue552ArtifactPipelineConformanceCase
    Invoke-SingleExeCompilerPackagingConformanceCase
    Invoke-Issue553HostedLibraryConformanceCase
    Invoke-Issue553LibraryInitCleanupAndLinkFlagsCase
    Invoke-Issue553RawDylibAndPosixLifecycleConformanceCase
    Invoke-Issue515SystemGetEnvConformanceCase
    Invoke-Issue516SystemExitConformanceCase
    Invoke-Issue517SystemRunConformanceCase
    Invoke-Issue518FileSystemOpenReadConformanceCase
    Invoke-Issue519FileSystemOpenWriteConformanceCase
    Invoke-Issue521FileSystemCreateWriteConformanceCase
    Invoke-Issue522FileSystemReadFileConformanceCase
    Invoke-Issue523FileSystemReadBytesConformanceCase
    Invoke-Issue524FileSystemWriteFileConformanceCase
    Invoke-Issue525FileSystemWriteStdoutConformanceCase
    Invoke-Issue526FileSystemWriteStderrConformanceCase
    Invoke-Issue527FileSystemExistsConformanceCase
    Invoke-Issue528FileSystemRemoveConformanceCase
    Invoke-Issue529FileSystemOpenDirConformanceCase
    Invoke-Issue530FileSystemCreateDirConformanceCase
    Invoke-Issue531FileSystemEnsureDirConformanceCase
    Invoke-Issue532FileSystemKindConformanceCase
    Invoke-Issue533FileSystemRestrictConformanceCase
    Invoke-Issue534FileModalReadConformanceCase
    Invoke-Issue535FileModalWriteConformanceCase
    Invoke-Issue536FileModalAppendConformanceCase
    Invoke-Issue537DirIterPrimitiveConformanceCase
    Invoke-Issue538NetworkRestrictHostConformanceCase
    Invoke-Issue539SharedClosureEscapeConformanceCase
    Invoke-Issue540InvariantResolutionConformanceCase
    Invoke-Issue542AsyncAliasSubtypingConformanceCase
    Invoke-Issue543RefinementUnificationConformanceCase
    Invoke-Issue520FileSystemOpenAppendConformanceCase
    Invoke-ExpectedDiagCodeCase -Id "no_ambient_system_ctor_safe" -Source (New-SystemCtorSafeSource) -ExpectedCodes @("E-CON-0020")
    Invoke-ExpectedDiagCodeCase -Id "no_ambient_system_record_literal_safe" -Source (New-SystemRecordLiteralSafeSource) -ExpectedCodes @("E-CON-0020")
    Invoke-ExpectedDiagCodeCase -Id "export_wrapped_capability_rejected" -Source (New-ExportWrappedCapabilitySource) -ExpectedCodes @("E-TYP-2623")
    $externStackArgAllowed = Invoke-CheckWithConformance `
        -CaseId "issue553_extern_stack_raw_ptr_arg_allowed" `
        -Source (New-Issue553ExternStackRawPtrArgAllowedSource) `
        -ConformanceFileName "issue553_extern_stack_raw_ptr_arg_allowed.log"
    if ($externStackArgAllowed.ExitCode -ne 0) {
        throw "Case 'issue553_extern_stack_raw_ptr_arg_allowed' expected exit 0 but got $($externStackArgAllowed.ExitCode)."
    }
    $externStackArgAllowedErrors = @($externStackArgAllowed.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($externStackArgAllowedErrors -ne 0) {
        throw "Case 'issue553_extern_stack_raw_ptr_arg_allowed' expected zero compile-time errors, observed $externStackArgAllowedErrors."
    }
    Invoke-ExpectedDiagCodeCase -Id "issue553_extern_region_local_raw_ptr_arg_rejected" -Source (New-Issue553ExternRegionLocalRawPtrArgRejectedSource) -ExpectedCodes @("E-SYS-3360")
    Invoke-ExpectedDiagCodeCase -Id "issue553_export_region_local_raw_ptr_return_rejected" -Source (New-Issue553ExportRegionLocalRawPtrReturnRejectedSource) -ExpectedCodes @("E-SYS-3360")
    Invoke-ExpectedDiagCodeCaseWithForbiddenCodes `
        -Id "issue553_export_region_local_raw_ptr_return_still_rejected_when_returning_null" `
        -Source (New-Issue553ExportRegionLocalRawPtrReturnStillRejectedWhenReturningNullSource) `
        -ExpectedCodes @("E-MEM-3020") `
        -ForbiddenCodes @("E-SYS-3360")
    $exportStackReturnAllowed = Invoke-CheckWithConformance `
        -CaseId "issue553_export_stack_raw_ptr_return_allowed" `
        -Source (New-Issue553ExportStackRawPtrReturnAllowedSource) `
        -ConformanceFileName "issue553_export_stack_raw_ptr_return_allowed.log"
    if ($exportStackReturnAllowed.ExitCode -ne 0) {
        throw "Case 'issue553_export_stack_raw_ptr_return_allowed' expected exit 0 but got $($exportStackReturnAllowed.ExitCode)."
    }
    $exportStackReturnAllowedErrors = @($exportStackReturnAllowed.DiagJson.diagnostics | Where-Object {
        $_.severity -eq "error" -or $_.severity -eq "panic"
    }).Count
    if ($exportStackReturnAllowedErrors -ne 0) {
        throw "Case 'issue553_export_stack_raw_ptr_return_allowed' expected zero compile-time errors, observed $exportStackReturnAllowedErrors."
    }

    Write-Host "[compiler-static] PASS"
} finally {
    try {
        if (Test-Path $workRoot) {
            $workFiles = @(Get-ChildItem -Path $workRoot -Recurse -File -Force)
            foreach ($file in $workFiles) {
                $relativePath = $file.FullName.Substring($workRoot.Length).TrimStart('\', '/')
                if ([string]::IsNullOrWhiteSpace($relativePath)) {
                    continue
                }

                $destinationRelativePath = $relativePath
                if ($file.Extension -ieq ".cursive") {
                    $destinationRelativePath = "$relativePath.txt"
                }

                $destinationPath = Join-Path $runCasesLogRoot $destinationRelativePath
                $destinationDir = Split-Path -Path $destinationPath -Parent
                if (-not (Test-Path $destinationDir)) {
                    New-Item -ItemType Directory -Path $destinationDir -Force | Out-Null
                }
                Copy-Item -Path $file.FullName -Destination $destinationPath -Force
            }
        }
        if (Test-Path $workRoot) {
            Remove-Item -Path $workRoot -Recurse -Force -ErrorAction SilentlyContinue
        }
        Write-Host "[compiler-static] artifacts=$runCasesLogRoot"
    } catch {
        Write-Host "[compiler-static] artifact_archive_failed=$($_.Exception.Message)"
    }
}
