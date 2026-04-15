param(
    [string]$CompilerPath = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\\..\\..")).Path
if ([string]::IsNullOrWhiteSpace($CompilerPath)) {
    $compilerCandidates = @(
        (Join-Path $repoRoot "cursive\\build\\windows\\out\\cursive.exe"),
        (Join-Path $repoRoot "cursive\\build\\windows\\Debug\\cursive.exe"),
        (Join-Path $repoRoot "cursive\\build\\windows\\Release\\cursive.exe"),
        (Join-Path $repoRoot "cursive\\build\\Debug\\cursive.exe"),
        (Join-Path $repoRoot "cursive\\build\\Release\\cursive.exe"),
        (Join-Path $repoRoot "build\\Debug\\cursive.exe"),
        (Join-Path $repoRoot "build\\Release\\cursive.exe")
    )
    foreach ($candidate in $compilerCandidates) {
        if (Test-Path $candidate) {
            $CompilerPath = $candidate
            break
        }
    }
    if ([string]::IsNullOrWhiteSpace($CompilerPath)) {
        $CompilerPath = $compilerCandidates[0]
    }
}
$runId = Get-Date -Format "yyyyMMdd_HHmmss_fff"
$logsRoot = Join-Path $repoRoot "build\\logs"
New-Item -ItemType Directory -Path $logsRoot -Force | Out-Null
$scratchRoot = Join-Path ([System.IO.Path]::GetTempPath()) "cursive_static_conformance"
New-Item -ItemType Directory -Path $scratchRoot -Force | Out-Null
$workRoot = Join-Path $scratchRoot ("tmp_attributes_static_conformance_" + $runId)
New-Item -ItemType Directory -Path $workRoot -Force | Out-Null

$defaultManifestLines = @(
    "[[assembly]]",
    "name = ""probe""",
    "kind = ""executable""",
    "root = ""src""",
    "out_dir = ""build/probe"""
)

$defaultBuildArgs = @("--check", "--diag-json", "--quiet")

$cases = @(
    @{
        id = "E-MOD-2450"
        expectCodes = @("E-MOD-2450")
        expectExit = 1
        source = @'
[[export]]
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
    },
    @{
        id = "E-MOD-2450-inline-unsupported-mode"
        expectCodes = @("E-MOD-2450")
        expectExit = 1
        source = @'
[[inline(hint)]]
public procedure InlineUnsupportedModeProbe() -> i32 {
    return 0
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
    },
    @{
        id = "E-MOD-2450-inline-named-arg"
        expectCodes = @("E-MOD-2450")
        expectExit = 1
        source = @'
[[inline(mode: always)]]
public procedure InlineNamedArgProbe() -> i32 {
    return 0
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
    },
    @{
        id = "E-MOD-2450-inline-multiple-args"
        expectCodes = @("E-MOD-2450")
        expectExit = 1
        source = @'
[[inline(always, never)]]
public procedure InlineMultipleArgsProbe() -> i32 {
    return 0
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
    },
    @{
        id = "E-MOD-2451"
        expectCodes = @("E-MOD-2451")
        expectExit = 1
        source = @'
[[unknown_attr]]
public procedure main(move ctx: Context) -> i32 {
    return 0
}
'@
    },
    @{
        id = "E-MOD-2451-extern-proc-attr"
        expectCodes = @("E-MOD-2451")
        expectExit = 1
        source = @'
extern "C" {
    [[unknown_attr]]
    procedure foreign_probe(value: i32) -> i32
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
    },
    @{
        id = "E-MOD-2451-packed-standalone"
        expectCodes = @("E-MOD-2451")
        expectExit = 1
        source = @'
[[packed]]
public record PackedStandalone {
    value: i32
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
    },
    @{
        id = "E-MOD-2451-align-standalone"
        expectCodes = @("E-MOD-2451")
        expectExit = 1
        source = @'
[[align(8)]]
public record AlignStandalone {
    value: i32
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
    },
    @{
        id = "E-UNS-0101-reflect"
        expectCodes = @("E-UNS-0101")
        expectExit = 1
        source = @'
[[reflect]]
public record DeferredReflect {
    value: i32
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
    },
    @{
        id = "E-MOD-2450-derive-missing-target"
        expectCodes = @("E-MOD-2450")
        expectExit = 1
        source = @'
[[derive]]
public enum DeferredDerive {
    A
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
    },
    @{
        id = "E-CTE-0312-derive-duplicate-target"
        expectCodes = @("E-CTE-0312")
        expectExit = 1
        source = @'
derive target Display(target: Type) {
    let _ = target
}

[[derive(Display, Display)]]
public record DeferredDeriveDuplicate {
    value: i32
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
    },
    @{
        id = "E-UNS-0101-emit"
        expectCodes = @("E-UNS-0101")
        expectExit = 1
        source = @'
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let value: i32 = [[emit]] 1
    return value
}
'@
    },
    @{
        id = "E-UNS-0101-files"
        expectCodes = @("E-UNS-0101")
        expectExit = 1
        source = @'
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let value: i32 = [[files]] 1
    return value
}
'@
    },
    @{
        id = "E-MOD-2450-memory-order-expression-placement"
        expectCodes = @("E-MOD-2450")
        expectExit = 1
        source = @'
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let value: i32 = [[acquire]] (1 + 2)
    return value
}
'@
    },
    @{
        id = "E-MOD-2450-layout-missing-parens"
        expectCodes = @("E-MOD-2450")
        expectExit = 1
        source = @'
[[layout]]
public record MissingLayoutArgs {
    value: i32
}
public procedure main(move ctx: Context) -> i32 {
    let _ = MissingLayoutArgs { value: 1 }
    return 0
}
'@
    },
    @{
        id = "E-MOD-2450-layout-named-arg"
        expectCodes = @("E-MOD-2450")
        expectExit = 1
        source = @'
[[layout(mode: C)]]
public record LayoutNamedArgProbe {
    value: i32
}
public procedure main(move ctx: Context) -> i32 {
    let _ = LayoutNamedArgProbe { value: 1 }
    return 0
}
'@
    },
    @{
        id = "E-MOD-2450-layout-invalid-int-type"
        expectCodes = @("E-MOD-2450")
        expectExit = 1
        source = @'
[[layout(i128)]]
public enum LayoutInvalidIntTypeProbe {
    Value
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let _ = LayoutInvalidIntTypeProbe::Value
    return 0
}
'@
    },
    @{
        id = "E-MOD-2450-layout-usize-discriminant-type"
        expectCodes = @("E-MOD-2450")
        expectExit = 1
        source = @'
[[layout(usize)]]
public enum LayoutUsizeDiscriminantProbe {
    Value
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let _ = LayoutUsizeDiscriminantProbe::Value
    return 0
}
'@
    },
    @{
        id = "E-MOD-2452"
        expectCodes = @("E-MOD-2452")
        expectExit = 1
        source = @'
[[layout(C)]]
public procedure main(move ctx: Context) -> i32 {
    return 0
}
'@
    },
    @{
        id = "E-MOD-2452-stale-ok-expression"
        expectCodes = @("E-MOD-2452")
        expectExit = 1
        source = @'
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let value: i32 = [[stale_ok]] 1
    return value
}
'@
    },
    @{
        id = "E-MOD-2452-import-decl"
        expectCodes = @("E-MOD-2452")
        expectExit = 1
        source = @'
[[deprecated]]
import probe::missing

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
    },
    @{
        id = "E-MOD-2452-using-decl"
        expectCodes = @("E-MOD-2452")
        expectExit = 1
        source = @'
[[deprecated]]
using probe::*

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
    },
    @{
        id = "E-MOD-2452-class-decl"
        expectCodes = @("E-MOD-2452")
        expectExit = 1
        source = @'
[[deprecated]]
public class UnsupportedAttrClass {
    procedure score(~) -> i32
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
    },
    @{
        id = "E-MOD-2452-static-decl"
        expectCodes = @("E-MOD-2452")
        expectExit = 1
        source = @'
[[deprecated]]
let unsupported_attr_value: i32 = 1

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return unsupported_attr_value
}
'@
    },
    @{
        id = "E-MOD-2452-static-extern-block"
        expectCodes = @("E-MOD-2452")
        expectExit = 1
        source = @'
[[static]]
extern "C" {
    procedure static_block_probe(value: i32) -> i32
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
    },
    @{
        id = "E-MOD-2452-mangle-extern-block"
        expectCodes = @("E-MOD-2452")
        expectExit = 1
        source = @'
[[mangle("bad_target")]]
extern "C" {
    procedure mangle_block_probe(value: i32) -> i32
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
    },
    @{
        id = "E-MOD-2453"
        expectCodes = @("E-MOD-2453")
        expectExit = 1
        source = @'
[[layout(align(3))]]
public record BadAlign {
    value: i32
}
public procedure main(move ctx: Context) -> i32 {
    let _ = BadAlign { value: 1 }
    return 0
}
'@
    },
    @{
        id = "E-MOD-2454"
        expectCodes = @("E-MOD-2454")
        expectExit = 1
        source = @'
[[layout(packed)]]
public enum BadPacked {
    A
}
public procedure main(move ctx: Context) -> i32 {
    return 0
}
'@
    },
    @{
        id = "E-MOD-2455"
        expectCodes = @("E-MOD-2455")
        expectExit = 1
        source = @'
[[layout(packed, align(8))]]
public record BadConflict {
    value: i32
}
public procedure main(move ctx: Context) -> i32 {
    let _ = BadConflict { value: 1 }
    return 0
}
'@
    },
    @{
        id = "E-MOD-2456"
        expectCodes = @("E-MOD-2456")
        expectExit = 1
        source = @'
[[log(bad: 1)]]
public procedure main(move ctx: Context) -> i32 {
    return 0
}
'@
    },
    @{
        id = "E-MOD-2457"
        expectCodes = @("E-MOD-2457")
        expectExit = 1
        source = @'
[[log(expected: "bad")]]
public procedure main(move ctx: Context) -> i32 {
    return 0
}
'@
    },
    @{
        id = "E-MOD-2458"
        expectCodes = @("E-MOD-2458")
        expectExit = 1
        source = @'
procedure Add1(value: i32) -> i32 {
    return value + 1
}
[[log(expected: Add1)]]
procedure ReturnsFunc() -> (i32) -> i32 {
    return Add1
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ReturnsFunc
    return 0
}
'@
    },
    @{
        id = "E-MOD-2459"
        expectCodes = @("E-MOD-2459")
        expectExit = 1
        source = @'
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
    },
    @{
        id = "E-CON-0410"
        expectCodes = @("E-CON-0410")
        expectExit = 1
        source = @'
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
    },
    @{
        id = "E-CON-0411"
        expectCodes = @("E-CON-0411")
        expectExit = 1
        source = @'
[[dynamic]]
type Alias = i32
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    let value: Alias = 1
    return value
}
'@
    },
    @{
        id = "E-CON-0412"
        expectCodes = @("E-CON-0412")
        expectExit = 1
        source = @'
public record DynamicFieldTarget {
    [[dynamic]]
    value: i32
}
public procedure main(move ctx: Context) -> i32 {
    let _ = DynamicFieldTarget { value: 1 }
    return 0
}
'@
    },
    @{
        id = "E-CON-0096"
        expectCodes = @("E-CON-0096")
        expectExit = 1
        source = @'
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    var gate: shared i32 = 0
    # gate speculative write {
        gate = [[relaxed]] (gate + 1)
    }
    return gate
}
'@
    },
    @{
        id = "E-SYS-3340"
        expectCodes = @("E-SYS-3340")
        expectExit = 1
        source = @'
[[mangle("plain_symbol")]]
public procedure main(move ctx: Context) -> i32 {
    return 0
}
'@
    },
    @{
        id = "E-SYS-3341"
        expectCodes = @("E-SYS-3341")
        expectExit = 1
        source = @'
[[mangle(""), export("C")]]
public procedure EmptySymbol() -> i32 {
    return 0
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return EmptySymbol()
}
'@
    },
    @{
        id = "E-SYS-3341-mangle-extra-args"
        expectCodes = @("E-SYS-3341")
        expectExit = 1
        source = @'
[[mangle(none, "extra_symbol"), export("C")]]
public procedure MangleExtraArgs() -> i32 {
    return 0
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return MangleExtraArgs()
}
'@
    },
    @{
        id = "E-SYS-3342"
        expectCodes = @("E-SYS-3342")
        expectExit = 1
        manifest_lines = @(
            "[[assembly]]",
            "name = ""probe""",
            "kind = ""executable""",
            "root = ""src""",
            "out_dir = ""build/probe"""
        )
        files = @{
            "dup_a.cursive" = @'
[[mangle("dup_symbol"), export("C")]]
public procedure DupA() -> i32 {
    return 1
}
'@
            "dup_b.cursive" = @'
[[mangle("dup_symbol"), export("C")]]
public procedure DupB() -> i32 {
    return 2
}
'@
            "main.cursive" = @'
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return DupA() + DupB()
}
'@
        }
        build_args = @("--diag-json", "--quiet")
    },
    @{
        id = "E-SYS-3345"
        expectCodes = @("E-SYS-3345")
        expectExit = 1
        source = @'
[[library(name: "kernel32", kind: "dylib")]]
public procedure main(move ctx: Context) -> i32 {
    return 0
}
'@
    },
    @{
        id = "E-SYS-3346"
        expectCodes = @("E-SYS-3346")
        expectExit = 1
        source = @'
[[library(name: "kernel32", kind: "unknown")]]
extern "C" {
    procedure BadLibrary() -> i32
}
public procedure main(move ctx: Context) -> i32 {
    return 0
}
'@
    },
    @{
        id = "E-MOD-2450-library-missing-name"
        expectCodes = @("E-MOD-2450")
        expectExit = 1
        source = @'
[[library(kind: "static")]]
extern "C" {
    procedure MissingLibraryName() -> i32
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
    },
    @{
        id = "E-MOD-2450-library-extra-key"
        expectCodes = @("E-MOD-2450")
        expectExit = 1
        source = @'
[[library(name: "kernel32", kind: "dylib", extra: "bad")]]
extern "C" {
    procedure LibraryExtraKey() -> i32
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
    },
    @{
        id = "E-MOD-2450-library-positional-name"
        expectCodes = @("E-MOD-2450")
        expectExit = 1
        source = @'
[[library("kernel32")]]
extern "C" {
    procedure PositionalLibraryProbe() -> i32
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
    },
    @{
        id = "LIB-raw-dylib-accepted"
        expectCodes = @()
        expectExit = 0
        source = @'
[[library(name: "kernel32", kind: "raw-dylib")]]
extern "C" {
    procedure RawDylibLibraryProbe() -> i32
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
    },
    @{
        id = "E-SYS-3347"
        expectCodes = @("E-SYS-3347")
        expectExit = 1
        build_args = @("--diag-json", "--quiet")
        source = @'
[[library(name: "./missing/does_not_exist_for_3347", kind: "static")]]
extern "C" {
    procedure MissingLibraryProbe() -> i32
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
    },
    @{
        id = "E-SYS-3347-named-library"
        expectCodes = @("E-SYS-3347")
        expectExit = 1
        build_args = @("--diag-json", "--quiet")
        source = @'
[[library(name: "definitely_missing_library_xyz")]]
extern "C" {
    procedure MissingNamedLibraryProbe() -> i32
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
    },
    @{
        id = "E-SYS-3350"
        expectCodes = @("E-SYS-3350")
        expectExit = 1
        source = @'
[[mangle(none)]]
public procedure main(move ctx: Context) -> i32 {
    return 0
}
'@
    },
    @{
        id = "E-SYS-3341-mangle-none-plus-symbol"
        expectCodes = @("E-SYS-3341")
        expectExit = 1
        source = @'
[[mangle(none, "sym_conflict"), export("C")]]
public procedure SymConflict() -> i32 {
    return 0
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return SymConflict()
}
'@
    },
    @{
        id = "E-SYS-3353"
        expectCodes = @("E-SYS-3353")
        expectExit = 1
        source = @'
[[export("C")]]
internal procedure HiddenExport() -> i32 {
    return 0
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return HiddenExport()
}
'@
    },
    @{
        id = "E-SYS-3355"
        expectCodes = @("E-SYS-3355")
        expectExit = 1
        source = @'
[[mangle("bad_unwind"), export("C-unwind"), unwind(oops)]]
public procedure BadUnwind() -> i32 {
    return 0
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return BadUnwind()
}
'@
    },
    @{
        id = "E-SYS-3356"
        expectCodes = @("E-SYS-3356")
        expectExit = 1
        source = @'
[[unwind(catch)]]
procedure PlainUnwind() -> i32 {
    return 0
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return PlainUnwind()
}
'@
    },
    @{
        id = "E-MOD-2450-ffi-pass-by-value-args"
        expectCodes = @("E-MOD-2450")
        expectExit = 1
        source = @'
[[ffi_pass_by_value(mode: "bad")]]
public record InvalidByValueAttr {
    value: i32
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
    },
    @{
        id = "E-TYP-2540"
        expectCodes = @("E-TYP-2540")
        expectExit = 1
        source = @'
public class DynBase {
    procedure only_static<T>(~, value: T) -> T {
        return value
    }
}
procedure CallOnDynamic(target: $DynBase) -> i32 {
    return target~>only_static(9)
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
    },
    @{
        id = "E-TYP-2624"
        expectCodes = @("E-TYP-2624")
        expectExit = 1
        source = @'
public record RecordWithoutLayout {
    value: i32
}
extern "C" {
    procedure TakeRecord(value: RecordWithoutLayout) -> i32
}
public procedure main(move ctx: Context) -> i32 {
    return 0
}
'@
    },
    @{
        id = "E-TYP-2625"
        expectCodes = @("E-TYP-2625")
        expectExit = 1
        source = @'
public enum EnumWithoutLayout {
    A
    B
}
extern "C" {
    procedure TakeEnum(value: EnumWithoutLayout) -> i32
}
public procedure main(move ctx: Context) -> i32 {
    return 0
}
'@
    },
    @{
        id = "E-TYP-2630"
        expectCodes = @("E-TYP-2630")
        expectExit = 1
        source = @'
[[layout(C)]]
public record DropPayload {
    value: unique i32
    procedure drop(~!) -> () {
    }
}
[[mangle("drop_payload_export"), export("C")]]
public procedure DropPayloadExport(value: DropPayload) -> i32 {
    let _ = value
    return 0
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
    },
    @{
        id = "E-TYP-2631"
        expectCodes = @("E-TYP-2631")
        expectExit = 1
        source = @'
[[mangle("catch_non_zeroable"), export("C-unwind"), unwind("catch")]]
public procedure CatchNonZeroable() -> string@Managed {
    return "hello"
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return 0
}
'@
    },
    @{
        id = "W-CNF-0601"
        expectCodes = @("W-CNF-0601")
        expectExit = 0
        source = @'
[[deprecated("legacy")]]
procedure LegacyProcedure() -> i32 {
    return 1
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return LegacyProcedure()
}
'@
    },
    @{
        id = "W-MOD-2451"
        expectCodes = @("W-MOD-2451")
        expectExit = 0
        source = @'
[[layout(align(1))]]
public record LowAlignRecord {
    value: i64
}
public procedure main(move ctx: Context) -> i32 {
    let _ = LowAlignRecord { value: 1 }
    return 0
}
'@
    },
    @{
        id = "W-MOD-2452"
        expectCodes = @("W-MOD-2452")
        expectExit = 0
        source = @'
[[inline(always)]]
public procedure RecursiveAlwaysInline(value: i32) -> i32 {
    if (value <= 0) {
        return 0
    }
    let next = value - 1
    return RecursiveAlwaysInline(next)
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return RecursiveAlwaysInline(1)
}
'@
    },
    @{
        id = "W-CON-0401"
        expectCodes = @("W-CON-0401")
        expectExit = 0
        source = @'
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
    },
    @{
        id = "W-SYS-3350"
        expectCodes = @("W-SYS-3350")
        expectExit = 0
        source = @'
[[mangle(none), export("C")]]
public procedure no_mangle_warn() -> i32 {
    return 0
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return no_mangle_warn()
}
'@
    },
    @{
        id = "W-SYS-3355"
        expectCodes = @("W-SYS-3355")
        expectExit = 0
        source = @'
[[mangle("unwind_abort_warn"), export("C"), unwind("abort")]]
public procedure unwind_abort_warn() -> i32 {
    return 0
}
public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    return unwind_abort_warn()
}
'@
    }
)

function Get-ObservedCodes {
    param([string[]]$OutputLines)

    $codes = @()

    foreach ($line in $OutputLines) {
        $trimmed = $line.TrimStart()
        if (-not $trimmed.StartsWith("{")) {
            continue
        }
        try {
            $payload = $trimmed | ConvertFrom-Json
            if ($null -ne $payload.diagnostics) {
                foreach ($diag in $payload.diagnostics) {
                    if ($null -ne $diag.code) {
                        $codes += [string]$diag.code
                    }
                }
            }
        } catch {
            # Keep scanning; some lines may not be JSON diagnostics.
        }
    }

    if ($codes.Count -eq 0) {
        $pattern = '[EW]-[A-Z]{3}-[0-9]{4}'
        foreach ($line in $OutputLines) {
            $matches = [regex]::Matches($line, $pattern)
            foreach ($m in $matches) {
                $codes += $m.Value
            }
        }
    }

    return @($codes | Select-Object -Unique)
}

function Invoke-AttributeDiagnosticCase {
    param([hashtable]$Case)

    $caseDirName = $Case.id.Replace("-", "_")
    $caseDir = Join-Path $workRoot $caseDirName
    New-Item -ItemType Directory -Path $caseDir | Out-Null
    $sourceRoot = Join-Path $caseDir "src"
    New-Item -ItemType Directory -Path $sourceRoot -Force | Out-Null

    $manifestLines = if ($Case.ContainsKey("manifest_lines")) {
        $Case.manifest_lines
    } else {
        $defaultManifestLines
    }
    Set-Content -Path (Join-Path $caseDir "Cursive.toml") -Value $manifestLines

    if ($Case.ContainsKey("files")) {
        foreach ($name in $Case.files.Keys) {
            $targetPath = Join-Path $sourceRoot $name
            $targetDir = Split-Path -Parent $targetPath
            if (-not [string]::IsNullOrWhiteSpace($targetDir)) {
                New-Item -ItemType Directory -Path $targetDir -Force | Out-Null
            }
            Set-Content -Path $targetPath -Value $Case.files[$name]
        }
    } else {
        Set-Content -Path (Join-Path $sourceRoot "main.cursive") -Value $Case.source
    }

    $assemblyName = if ($Case.ContainsKey("assembly_name")) {
        [string]$Case.assembly_name
    } else {
        "probe"
    }

    $buildArgs = if ($Case.ContainsKey("build_args")) {
        @($Case.build_args)
    } else {
        $defaultBuildArgs
    }

    $argList = @("build", ".", "--assembly", $assemblyName) + $buildArgs

    Push-Location $caseDir
    $savedErrorPreference = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $rawOutput = & $CompilerPath @argList 2>&1
        $rc = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $savedErrorPreference
        Pop-Location
    }

    $outputLines = @()
    foreach ($line in $rawOutput) {
        $outputLines += $line.ToString()
    }

    $observedCodes = Get-ObservedCodes -OutputLines $outputLines

    $missingCodes = @()
    foreach ($code in $Case.expectCodes) {
        if (-not ($observedCodes -contains $code)) {
            $missingCodes += $code
        }
    }

    $unexpectedCodes = @()
    foreach ($code in $observedCodes) {
        if (-not ($Case.expectCodes -contains $code)) {
            $unexpectedCodes += $code
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
        unexpected_codes = $unexpectedCodes
        case_dir = $caseDir
        args = $argList
        output = $outputLines
    }
}

$results = @()
foreach ($case in $cases) {
    $result = Invoke-AttributeDiagnosticCase -Case $case
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

$targetProfilePath = Join-Path $repoRoot "cursive\\src\\01_project\\target_profile.cpp"
$ffiLibraryPath = Join-Path $repoRoot "cursive\\src\\01_project\\ffi_library.cpp"
$driverPath = Join-Path $repoRoot "cursive\\src\\06_driver\\main.cpp"

foreach ($required in @($targetProfilePath, $ffiLibraryPath, $driverPath)) {
    if (-not (Test-Path $required)) {
        throw "Attribute static conformance missing expected source file: $required"
    }
}

$targetProfileText = Get-Content -Path $targetProfilePath -Raw
foreach ($pattern in @(
    'if \(kind == "dylib"\) \{\s+return std::string\(LibraryPrefix\(profile\)\) \+ std::string\(name\) \+\s+std::string\(SharedLibSuffix\(profile\)\);',
    'if \(kind == "raw-dylib" && profile == TargetProfile::X86_64Win64\) \{\s+return std::string\(name\) \+ "\.dll";'
)) {
    if ($targetProfileText -notmatch $pattern) {
        throw "Attribute static conformance missing expected Win64 library-name resolution pattern '$pattern' in $targetProfilePath."
    }
}

$ffiLibraryText = Get-Content -Path $ffiLibraryPath -Raw
foreach ($pattern in @(
    'ResolveLibraryLinkInputForCurrentTarget\(',
    'candidate\.replace_extension\("\.lib"\);'
)) {
    if ($ffiLibraryText -notmatch $pattern) {
        throw "Attribute static conformance missing expected linker-input pattern '$pattern' in $ffiLibraryPath."
    }
}

$driverText = Get-Content -Path $driverPath -Raw
if ($driverText -notmatch 'ResolveLibraryLinkInputForCurrentTarget\(') {
    throw "Attribute static conformance expected main.cpp to resolve linker inputs through ResolveLibraryLinkInputForCurrentTarget()."
}
if ($driverText -if 'candidate\.replace_extension\("\.lib"\);') is {
    throw "Attribute static conformance found stale inline .dll-to-.lib rewriting in main.cpp."
}

$reportPath = Join-Path $logsRoot ("attribute_static_conformance_report_" + $runId + ".json")
$results | ConvertTo-Json -Depth 10 | Set-Content -Path $reportPath
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
