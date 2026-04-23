#!/usr/bin/env pwsh
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

# Force UTF-8 for all native-command I/O so the Codex prompt (which includes
# spec content with Unicode math/Greek glyphs) is not mangled by the default
# us-ascii / OEM encoding on Windows PowerShell 5.1.
$utf8NoBom = [System.Text.UTF8Encoding]::new($false)
$OutputEncoding = $utf8NoBom
[Console]::OutputEncoding = $utf8NoBom
[Console]::InputEncoding = $utf8NoBom

function Show-Usage {
@'
Usage: powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\spec-audit-loop.ps1 [options]

Launch the repo-local spec-audit command one row at a time in isolated
worktrees until it reports `SPEC_AUDIT_STATUS: complete` or
`SPEC_AUDIT_STATUS: blocked`.

Options:
  --once                 Run a single row-item iteration and stop after the first status
  --max-iterations <N>   Stop after N iterations (0 means unlimited; default 0)
  --recover-stale-in-progress
                         Recover exactly one in_progress row when used with --row-line
  --row-line <N>         CSV logical line to recover with --recover-stale-in-progress
  --model <MODEL>        Override the Codex model (default: $SPEC_AUDIT_MODEL or gpt-5.5)
  --reasoning-effort <LEVEL>
                         Override Codex reasoning effort (default: $SPEC_AUDIT_REASONING_EFFORT or xhigh)
  --profile <PROFILE>    Optional Codex profile to pass to each exec call
  -h, --help             Show this help and exit

Environment:
  SPEC_AUDIT_WORKTREE_ROOT   Parent directory for per-row worktrees
  SPEC_AUDIT_WINDOWS_PRESET  Main-repo Windows build preset to verify after integration
  SPEC_AUDIT_WINDOWS_TARGET  Main-repo Windows build target to verify after integration
  SPEC_AUDIT_COMPILER_PATH   Explicit compiler executable path for non-default presets
'@ | Write-Output
}

function Write-Log([string]$Message) {
    Write-Host "[spec-audit-loop] $Message"
}

function Fail([string]$Message, [int]$ExitCode = 1) {
    [Console]::Error.WriteLine("[spec-audit-loop] error: $Message")
    exit $ExitCode
}

function Require-Command([string]$Name) {
    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($null -eq $command) {
        Fail "required command not found: $Name"
    }
    return $command.Source
}

function Resolve-CodexCommand {
    if ($env:OS -eq 'Windows_NT') {
        $codexCmd = Get-Command 'codex.cmd' -ErrorAction SilentlyContinue
        if ($null -ne $codexCmd) {
            # `codex.ps1` re-pipes pipeline input through `$input`, which breaks
            # `codex exec -` with "stdin is not a terminal" on Windows.
            return $codexCmd.Source
        }
    }

    return Require-Command 'codex'
}

function New-TempPath([string]$Prefix, [string]$Suffix = '') {
    return Join-Path ([System.IO.Path]::GetTempPath()) ($Prefix + [Guid]::NewGuid().ToString('N') + $Suffix)
}

function New-TempFile([string]$Prefix, [string]$Suffix = '') {
    $path = New-TempPath $Prefix $Suffix
    New-Item -ItemType File -Path $path -Force | Out-Null
    return $path
}

function New-TempDirectory([string]$Prefix) {
    $path = New-TempPath $Prefix
    New-Item -ItemType Directory -Path $path -Force | Out-Null
    return $path
}

function Write-Utf8NoBom([string]$Path, [string]$Text) {
    $encoding = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($Path, $Text, $encoding)
}

function Write-LinesUtf8NoBom([string]$Path, [string[]]$Lines) {
    $text = [string]::Join("`n", $Lines)
    if ($Lines.Count -gt 0) {
        $text += "`n"
    }
    Write-Utf8NoBom $Path $text
}

function Join-RepoPath([string]$Base, [string]$RelativePath) {
    $native = ($RelativePath -split '[\\/]+') -join [System.IO.Path]::DirectorySeparatorChar
    return Join-Path $Base $native
}

function Get-LastMatchingLine([string]$Path, [string]$Pattern) {
    if (-not (Test-Path -LiteralPath $Path)) {
        return ''
    }

    $match = Get-Content -LiteralPath $Path | Where-Object { $_ -match $Pattern } | Select-Object -Last 1
    if ($null -eq $match) {
        return ''
    }

    return [string]$match
}

function Replace-KeywordTrigger([string]$Text, [string]$Source, [string]$Target) {
    $pattern = '\b' + [regex]::Escape($Source) + '\b'
    return [regex]::Replace(
        $Text,
        $pattern,
        {
            param($Match)
            $token = $Match.Value
            if ($token -cmatch '^[A-Z]+$') {
                return $Target.ToUpperInvariant()
            }
            if ($token.Length -gt 0 -and $token.Substring(0, 1) -ceq $token.Substring(0, 1).ToUpperInvariant()) {
                if ($Target.Length -le 1) {
                    return $Target.ToUpperInvariant()
                }
                return $Target.Substring(0, 1).ToUpperInvariant() + $Target.Substring(1)
            }
            return $Target
        },
        [System.Text.RegularExpressions.RegexOptions]::IgnoreCase
    )
}

function Invoke-GitCapture([string[]]$Arguments, [switch]$AllowFailure) {
    $previousEap = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $output = & $script:GitExe @Arguments 2>&1
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousEap
    }
    if (-not $AllowFailure -and $exitCode -ne 0) {
        $rendered = (($output | ForEach-Object { "$_" }) -join "`n").Trim()
        if ($rendered) {
            throw $rendered
        }
        throw "git $($Arguments -join ' ') failed with exit code $exitCode"
    }

    return [PSCustomObject]@{
        ExitCode = $exitCode
        Output   = @($output | ForEach-Object { "$_" })
    }
}

function Invoke-PythonScript([string]$ScriptText, [string[]]$Arguments, [switch]$AllowFailure) {
    $scriptPath = New-TempFile 'spec-audit-py-' '.py'
    try {
        Write-Utf8NoBom $scriptPath $ScriptText
        $previousEap = $ErrorActionPreference
        $ErrorActionPreference = 'Continue'
        try {
            $output = & $script:PythonExe $scriptPath @Arguments 2>&1
            $exitCode = $LASTEXITCODE
        } finally {
            $ErrorActionPreference = $previousEap
        }
        if (-not $AllowFailure -and $exitCode -ne 0) {
            $rendered = (($output | ForEach-Object { "$_" }) -join "`n").Trim()
            if ($rendered) {
                throw $rendered
            }
            throw "python3 script failed with exit code $exitCode"
        }

        return [PSCustomObject]@{
            ExitCode = $exitCode
            Output   = @($output | ForEach-Object { "$_" })
        }
    } finally {
        if (Test-Path -LiteralPath $scriptPath) {
            Remove-Item -LiteralPath $scriptPath -Force
        }
    }
}

function Cleanup-TempFiles {
    foreach ($path in @(
        $script:LastMessageFile,
        $script:IterationPromptFile,
        $script:BaseDecisionsSnapshotFile,
        $script:HeadAuditCsvFile,
        $script:StagedAuditCsvFile,
        $script:HeadSpecDecisionsFile,
        $script:StagedSpecDecisionsFile,
        $script:CommitMessageFile,
        $script:NonSharedPathsFile,
        $script:AgentEventsFile,
        $script:BuildFailurePromptFile,
        $script:MainBuildLogFile
    )) {
        if ($path -and (Test-Path -LiteralPath $path)) {
            Remove-Item -LiteralPath $path -Force -ErrorAction SilentlyContinue
        }
    }

    if ($script:IntegrationBackupDir -and (Test-Path -LiteralPath $script:IntegrationBackupDir)) {
        Remove-Item -LiteralPath $script:IntegrationBackupDir -Recurse -Force -ErrorAction SilentlyContinue
    }
}

function Reset-TempState {
    Cleanup-TempFiles
    $script:LastMessageFile = ''
    $script:IterationPromptFile = ''
    $script:BaseDecisionsSnapshotFile = ''
    $script:HeadAuditCsvFile = ''
    $script:StagedAuditCsvFile = ''
    $script:HeadSpecDecisionsFile = ''
    $script:StagedSpecDecisionsFile = ''
    $script:CommitMessageFile = ''
    $script:IntegrationBackupDir = ''
    $script:NonSharedPathsFile = ''
    $script:AgentEventsFile = ''
    $script:BuildFailurePromptFile = ''
    $script:MainBuildLogFile = ''
    $script:ClaimLedgerPath = ''
    $script:VerifiedCompilerPath = ''
    $script:OverlayStarted = $false
}

$script:LastMessageFile = ''
$script:IterationPromptFile = ''
$script:BaseDecisionsSnapshotFile = ''
$script:HeadAuditCsvFile = ''
$script:StagedAuditCsvFile = ''
$script:HeadSpecDecisionsFile = ''
$script:StagedSpecDecisionsFile = ''
$script:CommitMessageFile = ''
$script:IntegrationBackupDir = ''
$script:NonSharedPathsFile = ''
$script:AgentEventsFile = ''
$script:BuildFailurePromptFile = ''
$script:MainBuildLogFile = ''
$script:WorktreeDir = ''
$script:WorktreeBranch = ''
$script:WorktreeBaseCommit = ''
$script:WorktreeCommit = ''
$script:WorktreeSessionId = ''
$script:SelectedRow = $null
$script:SelectedRowJson = ''
$script:SelectedRowRawIndex = 0
$script:SelectedItem = ''
$script:ClaimLedgerDir = ''
$script:ClaimLedgerPath = ''
$script:VerifiedCompilerPath = ''
$script:OverlayStarted = $false

$script:ScriptDir = (Resolve-Path -LiteralPath $PSScriptRoot).Path
$script:RepoRoot = (Resolve-Path -LiteralPath (Join-Path $script:ScriptDir '..')).Path
$script:PromptFile = Join-Path $script:RepoRoot '.codex\commands\spec-audit-loop.md'
$script:ScriptLocalPromptFile = Join-Path $script:ScriptDir '.codex\commands\spec-audit-loop.md'
$script:AuditCsv = Join-Path $script:RepoRoot 'docs\audit\SPEC_RULE_TABLE_BY_PHASE.csv'
$script:SpecDecisionsFile = Join-Path $script:RepoRoot 'docs\SpecDecisionsNeeded.md'
$defaultWorktreeRoot = Join-Path (Split-Path -Parent $script:RepoRoot) '.spec-audit-worktrees'
$script:WorktreeRoot = if ($env:SPEC_AUDIT_WORKTREE_ROOT) { $env:SPEC_AUDIT_WORKTREE_ROOT } else { $defaultWorktreeRoot }
$script:WindowsPreset = if ($env:SPEC_AUDIT_WINDOWS_PRESET) { $env:SPEC_AUDIT_WINDOWS_PRESET } elseif ($env:CURSIVE_WINDOWS_PRESET) { $env:CURSIVE_WINDOWS_PRESET } else { 'windows-debug' }
$script:WindowsTarget = if ($env:SPEC_AUDIT_WINDOWS_TARGET) { $env:SPEC_AUDIT_WINDOWS_TARGET } elseif ($env:CURSIVE_WINDOWS_TARGET) { $env:CURSIVE_WINDOWS_TARGET } else { 'cursive_out' }
$script:CompilerPathOverride = if ($env:SPEC_AUDIT_COMPILER_PATH) { $env:SPEC_AUDIT_COMPILER_PATH } else { '' }
$script:Once = $false
$script:RecoverStaleInProgress = $false
$script:RecoverRowLine = ''
$script:MaxIterations = if ($env:SPEC_AUDIT_MAX_ITERATIONS) { $env:SPEC_AUDIT_MAX_ITERATIONS } else { '0' }
$script:Model = if ($env:SPEC_AUDIT_MODEL) { $env:SPEC_AUDIT_MODEL } else { 'gpt-5.5' }
$script:ReasoningEffort = if ($env:SPEC_AUDIT_REASONING_EFFORT) { $env:SPEC_AUDIT_REASONING_EFFORT } else { 'xhigh' }
$script:Profile = if ($env:SPEC_AUDIT_PROFILE) { $env:SPEC_AUDIT_PROFILE } else { '' }

$cliArgs = @($args)
$showHelp = $false
for ($index = 0; $index -lt $cliArgs.Count;) {
    switch ($cliArgs[$index]) {
        '--once' {
            $script:Once = $true
            $index += 1
        }
        '--max-iterations' {
            if ($index + 1 -ge $cliArgs.Count) {
                Fail 'missing value for --max-iterations' 2
            }
            $script:MaxIterations = $cliArgs[$index + 1]
            $index += 2
        }
        '--recover-stale-in-progress' {
            $script:RecoverStaleInProgress = $true
            $index += 1
        }
        '--row-line' {
            if ($index + 1 -ge $cliArgs.Count) {
                Fail 'missing value for --row-line' 2
            }
            $script:RecoverRowLine = $cliArgs[$index + 1]
            $index += 2
        }
        '--model' {
            if ($index + 1 -ge $cliArgs.Count) {
                Fail 'missing value for --model' 2
            }
            $script:Model = $cliArgs[$index + 1]
            $index += 2
        }
        '--reasoning-effort' {
            if ($index + 1 -ge $cliArgs.Count) {
                Fail 'missing value for --reasoning-effort' 2
            }
            $script:ReasoningEffort = $cliArgs[$index + 1]
            $index += 2
        }
        '--profile' {
            if ($index + 1 -ge $cliArgs.Count) {
                Fail 'missing value for --profile' 2
            }
            $script:Profile = $cliArgs[$index + 1]
            $index += 2
        }
        '-h' {
            $showHelp = $true
            $index += 1
        }
        '--help' {
            $showHelp = $true
            $index += 1
        }
        default {
            [Console]::Error.WriteLine("unknown argument: $($cliArgs[$index])")
            Show-Usage | Write-Error
            exit 2
        }
    }
}

if ($showHelp) {
    Show-Usage
    exit 0
}

if (-not (Test-Path -LiteralPath $script:PromptFile)) {
    Fail "missing prompt file: $($script:PromptFile)"
}

if ((Test-Path -LiteralPath $script:ScriptLocalPromptFile) -and
    ((Get-Content -LiteralPath $script:PromptFile -Raw) -ne (Get-Content -LiteralPath $script:ScriptLocalPromptFile -Raw))) {
    Fail "script-local prompt differs from authoritative prompt:`n  authoritative: $($script:PromptFile)`n  script-local:  $($script:ScriptLocalPromptFile)"
}

if (-not (Test-Path -LiteralPath $script:AuditCsv)) {
    Fail "missing audit csv: $($script:AuditCsv)"
}

if ($script:RecoverStaleInProgress -and $script:RecoverRowLine -notmatch '^[0-9]+$') {
    Fail '--recover-stale-in-progress requires --row-line <N>' 2
}

if ((-not $script:RecoverStaleInProgress) -and -not [string]::IsNullOrWhiteSpace($script:RecoverRowLine)) {
    Fail '--row-line is only valid with --recover-stale-in-progress' 2
}

if ($script:MaxIterations -notmatch '^[0-9]+$') {
    Fail "invalid --max-iterations value: $($script:MaxIterations)" 2
}

if ($script:ReasoningEffort -notin @('low', 'medium', 'high', 'xhigh')) {
    Fail "invalid --reasoning-effort value: $($script:ReasoningEffort)" 2
}

$script:CodexExe = Resolve-CodexCommand
$script:GitExe = Require-Command 'git'
$script:PythonExe = Require-Command 'python3'
$script:GitCheckoutHookArgs = @()
if (-not (Get-Command 'git-lfs' -ErrorAction SilentlyContinue)) {
    $script:GitCheckoutHookArgs = @('-c', 'core.hooksPath=/dev/null')
}

$gitCommonDir = ((Invoke-GitCapture @('-C', $script:RepoRoot, 'rev-parse', '--git-common-dir')).Output | Select-Object -Last 1).Trim()
if (-not [System.IO.Path]::IsPathRooted($gitCommonDir)) {
    $gitCommonDir = Join-RepoPath $script:RepoRoot $gitCommonDir
}
$script:LockFile = Join-Path $gitCommonDir 'spec-audit-loop.lock'
$script:ClaimLedgerDir = Join-Path $gitCommonDir 'spec-audit-loop-claims'

# MARKER: CORE

function Claim-NextRow {
$scriptText = @'
import csv
import datetime as dt
import json
import os
import secrets
import sys
import tempfile
import time
from pathlib import Path

audit_csv = Path(sys.argv[1])
lock_path = Path(sys.argv[2])
ledger_dir = Path(sys.argv[3])
worktree_root = Path(sys.argv[4])
launcher_pid = sys.argv[5]
deadline = time.time() + 300

while True:
    try:
        lock_fd = os.open(lock_path, os.O_CREAT | os.O_EXCL | os.O_WRONLY)
        os.write(lock_fd, f"{os.getpid()}\n".encode("ascii"))
        os.close(lock_fd)
        break
    except FileExistsError:
        if time.time() >= deadline:
            print(f"timed out waiting for lock: {lock_path}", file=sys.stderr)
            sys.exit(3)
        time.sleep(0.1)

try:
    ledger_dir.mkdir(parents=True, exist_ok=True)
    with audit_csv.open(encoding="utf-8", newline="") as fh:
        raw_rows = list(csv.reader(fh))
        if not raw_rows or not raw_rows[0]:
            print(f"missing CSV header in {audit_csv}", file=sys.stderr)
            sys.exit(2)

    selected = None
    in_progress_rows = []
    logical_line = 2
    for raw_index, raw_row in enumerate(raw_rows[1:], start=1):
        if not raw_row or not any(cell.strip() for cell in raw_row):
            continue

        cells = list(raw_row)
        while len(cells) < 6:
            cells.append("")

        row = {
            "phase": cells[0],
            "implemented": cells[1],
            "rule name": cells[2],
            "rule content": cells[3],
            "spec location": cells[4],
            "compiler location": ",".join(cells[5:]),
        }

        implemented = row["implemented"].strip()
        current_line = logical_line
        logical_line += 1
        if implemented == "in_progress":
            in_progress_rows.append({
                "line": current_line,
                "raw_index": raw_index,
                "rule name": row["rule name"],
                "spec location": row["spec location"],
            })
            continue
        if implemented in {"complete", "ambiguous", "in_progress"}:
            continue

        selected = dict(row)
        selected["_line"] = current_line
        selected["_raw_index"] = raw_index
        selected["_previous_implemented"] = implemented
        selected["implemented"] = "in_progress"
        stamp = dt.datetime.now(dt.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
        nonce = secrets.token_hex(3)
        worktree_id = f"spec-audit-{stamp}-{current_line}-{nonce}"
        branch = f"spec-audit/{stamp}-line{current_line}-{nonce}"
        worktree_dir = worktree_root / worktree_id
        ledger_path = ledger_dir / f"{worktree_id}.json"
        selected["_worktree_id"] = worktree_id
        selected["_worktree_branch"] = branch
        selected["_worktree_dir"] = str(worktree_dir)
        selected["_ledger_path"] = str(ledger_path)
        while len(raw_rows[raw_index]) < 2:
            raw_rows[raw_index].append("")
        raw_rows[raw_index][1] = "in_progress"
        break

    if selected is None:
        if in_progress_rows:
            print("no actionable rows remain outside in_progress claims", file=sys.stderr)
            for row in in_progress_rows:
                print(
                    f"in_progress line {row['line']}: {row['rule name']} @ {row['spec location']}",
                    file=sys.stderr,
                )
            sys.exit(11)
        sys.exit(10)

    if in_progress_rows:
        print("skipping existing in_progress rows:", file=sys.stderr)
        for row in in_progress_rows:
            print(
                f"  line {row['line']}: {row['rule name']} @ {row['spec location']}",
                file=sys.stderr,
            )

    fd, tmp_name = tempfile.mkstemp(
        prefix="spec-audit-claim.",
        suffix=".csv",
        dir=str(audit_csv.parent),
        text=True,
    )
    try:
        with os.fdopen(fd, "w", encoding="utf-8", newline="") as tmp:
            writer = csv.writer(tmp)
            writer.writerows(raw_rows)
        os.replace(tmp_name, audit_csv)
    finally:
        if os.path.exists(tmp_name):
            os.unlink(tmp_name)

    ledger = {
        "row_line": selected["_line"],
        "raw_index": selected["_raw_index"],
        "previous_implemented": selected["_previous_implemented"],
        "rule_name": selected["rule name"],
        "spec_location": selected["spec location"],
        "branch": selected["_worktree_branch"],
        "worktree": selected["_worktree_dir"],
        "worktree_id": selected["_worktree_id"],
        "created_at": dt.datetime.now(dt.timezone.utc).isoformat(),
        "launcher_pid": launcher_pid,
        "stage": "claimed",
    }
    ledger_tmp = ledger_path.with_suffix(ledger_path.suffix + ".tmp")
    ledger_tmp.write_text(json.dumps(ledger, ensure_ascii=True, indent=2) + "\n", encoding="utf-8")
    os.replace(ledger_tmp, ledger_path)

    print(json.dumps(selected, ensure_ascii=True))
finally:
    try:
        lock_path.unlink()
    except FileNotFoundError:
        pass
'@

    return Invoke-PythonScript $scriptText @(
        $script:AuditCsv,
        $script:LockFile,
        $script:ClaimLedgerDir,
        $script:WorktreeRoot,
        [string]$PID
    ) -AllowFailure
}

function Recover-StaleInProgressRow {
$scriptText = @'
import csv
import datetime as dt
import json
import os
import sys
import tempfile
import time
from pathlib import Path

audit_csv = Path(sys.argv[1])
lock_path = Path(sys.argv[2])
ledger_dir = Path(sys.argv[3])
target_line = int(sys.argv[4])
deadline = time.time() + 300

while True:
    try:
        lock_fd = os.open(lock_path, os.O_CREAT | os.O_EXCL | os.O_WRONLY)
        os.write(lock_fd, f"{os.getpid()}\n".encode("ascii"))
        os.close(lock_fd)
        break
    except FileExistsError:
        if time.time() >= deadline:
            print(f"timed out waiting for lock: {lock_path}", file=sys.stderr)
            sys.exit(3)
        time.sleep(0.1)

try:
    with audit_csv.open(encoding="utf-8", newline="") as fh:
        rows = list(csv.reader(fh))
    if not rows or not rows[0]:
        print(f"missing CSV header in {audit_csv}", file=sys.stderr)
        sys.exit(2)

    logical_line = 2
    target_raw_index = None
    target_row = None
    for raw_index, raw_row in enumerate(rows[1:], start=1):
        if not raw_row or not any(cell.strip() for cell in raw_row):
            continue
        current_line = logical_line
        logical_line += 1
        if current_line == target_line:
            target_raw_index = raw_index
            target_row = list(raw_row)
            break

    if target_raw_index is None or target_row is None:
        print(f"row line not found: {target_line}", file=sys.stderr)
        sys.exit(2)
    while len(target_row) < 6:
        target_row.append("")
    if target_row[1].strip() != "in_progress":
        print(f"row line {target_line} is not in_progress (current: {target_row[1]!r})", file=sys.stderr)
        sys.exit(2)

    ledgers = []
    if ledger_dir.exists():
        for path in ledger_dir.glob("*.json"):
            try:
                data = json.loads(path.read_text(encoding="utf-8"))
            except (OSError, json.JSONDecodeError):
                continue
            if int(data.get("row_line", -1)) == target_line:
                ledgers.append((path, data))
    ledgers.sort(key=lambda item: item[1].get("created_at", ""))
    ledger_path = ledgers[-1][0] if ledgers else None
    ledger = ledgers[-1][1] if ledgers else {}
    previous = str(ledger.get("previous_implemented", "")).strip()
    if not previous or previous in {"complete", "ambiguous", "in_progress"}:
        previous = "incomplete"
        print(
            f"previous implemented state unavailable for row line {target_line}; resetting to incomplete",
            file=sys.stderr,
        )

    print(f"recovering row line {target_line}: {target_row[2]} @ {target_row[4]}", file=sys.stderr)
    print(f"implemented: in_progress -> {previous}", file=sys.stderr)
    target_row[1] = previous
    rows[target_raw_index] = target_row

    fd, tmp_name = tempfile.mkstemp(
        prefix="spec-audit-recover.",
        suffix=".csv",
        dir=str(audit_csv.parent),
        text=True,
    )
    try:
        with os.fdopen(fd, "w", encoding="utf-8", newline="") as tmp:
            csv.writer(tmp).writerows(rows)
        os.replace(tmp_name, audit_csv)
    finally:
        if os.path.exists(tmp_name):
            os.unlink(tmp_name)

    if ledger_path is not None:
        ledger["stage"] = "recovered"
        ledger["recovered_at"] = dt.datetime.now(dt.timezone.utc).isoformat()
        ledger["recovered_to"] = previous
        tmp = ledger_path.with_suffix(ledger_path.suffix + ".tmp")
        tmp.write_text(json.dumps(ledger, ensure_ascii=True, indent=2) + "\n", encoding="utf-8")
        os.replace(tmp, ledger_path)
finally:
    try:
        lock_path.unlink()
    except FileNotFoundError:
        pass
'@

    $null = Invoke-PythonScript $scriptText @(
        $script:AuditCsv,
        $script:LockFile,
        $script:ClaimLedgerDir,
        $script:RecoverRowLine
    )
}

function Update-ClaimLedgerStage([string]$Stage, [string]$Note = '') {
    if ([string]::IsNullOrWhiteSpace($script:ClaimLedgerPath)) {
        return
    }

$scriptText = @'
import datetime as dt
import json
import os
import sys
import time
from pathlib import Path

lock_path = Path(sys.argv[1])
ledger_path = Path(sys.argv[2])
stage = sys.argv[3]
note = sys.argv[4]
deadline = time.time() + 300

while True:
    try:
        lock_fd = os.open(lock_path, os.O_CREAT | os.O_EXCL | os.O_WRONLY)
        os.write(lock_fd, f"{os.getpid()}\n".encode("ascii"))
        os.close(lock_fd)
        break
    except FileExistsError:
        if time.time() >= deadline:
            sys.exit(3)
        time.sleep(0.1)

try:
    try:
        data = json.loads(ledger_path.read_text(encoding="utf-8"))
    except FileNotFoundError:
        data = {}
    data["stage"] = stage
    data["updated_at"] = dt.datetime.now(dt.timezone.utc).isoformat()
    if note:
        data["note"] = note
    if stage.startswith("interrupted"):
        data["interrupted_at"] = data["updated_at"]
    tmp = ledger_path.with_suffix(ledger_path.suffix + ".tmp")
    tmp.write_text(json.dumps(data, ensure_ascii=True, indent=2) + "\n", encoding="utf-8")
    os.replace(tmp, ledger_path)
finally:
    try:
        lock_path.unlink()
    except FileNotFoundError:
        pass
'@

    $null = Invoke-PythonScript $scriptText @($script:LockFile, $script:ClaimLedgerPath, $Stage, $Note)
}

function Copy-SharedContextToWorktree([string]$WorktreeDir) {
    $worktreeSpecDecisions = Join-RepoPath $WorktreeDir 'docs/SpecDecisionsNeeded.md'
    if (Test-Path -LiteralPath $script:SpecDecisionsFile) {
        $parent = Split-Path -Parent $worktreeSpecDecisions
        if ($parent) {
            New-Item -ItemType Directory -Path $parent -Force | Out-Null
        }
        Copy-Item -LiteralPath $script:SpecDecisionsFile -Destination $worktreeSpecDecisions -Force
        Copy-Item -LiteralPath $script:SpecDecisionsFile -Destination $script:BaseDecisionsSnapshotFile -Force
    } else {
        Write-Utf8NoBom $script:BaseDecisionsSnapshotFile ''
    }
}

function Build-IterationPrompt {
    $base = Get-Content -LiteralPath $script:PromptFile -Raw
    $selectionLines = @(
        '',
        'Launcher Context:',
        "- Shared checkout: $($script:RepoRoot)",
        "- Assigned worktree: $($script:WorktreeDir)",
        "- Assigned branch: $($script:WorktreeBranch)",
        "- Worktree base commit: $($script:WorktreeBaseCommit)",
        '',
        'Selected Audit Row:',
        "- CSV line: $($script:SelectedRow._line)",
        "- Assigned From: $($script:SelectedRow._previous_implemented)",
        "- Implemented: $($script:SelectedRow.implemented)",
        "- Phase: $($script:SelectedRow.phase)",
        "- Rule Name: $($script:SelectedRow.'rule name')",
        "- Rule Content: $($script:SelectedRow.'rule content')",
        "- Spec Location: $($script:SelectedRow.'spec location')",
        "- Compiler Location: $($script:SelectedRow.'compiler location')",
        '',
        'Iteration instruction:',
        '- Do not rescan the CSV for this iteration.',
        '- Work only the selected audit row above.',
        '- Multiple audit agents may be active at the same time.',
        '- Ignore rows whose `implemented` value is `in_progress`; those rows are already assigned.',
        '- All changes for this row are isolated in the assigned worktree above.',
        '- The launcher is the sole owner of validation after your row-item commit is created.',
        '- Do not use temporary fixes, fallbacks, wrappers, shims, stubs, placeholders, compatibility shortcuts, or avoidance of the correct compiler work. A row fix must be the durable, spec-aligned implementation.',
        '- If implementing the selected row exposes another spec conformance issue that is required for this row to work correctly, fix that issue as part of the same row result instead of working around it. Only treat an issue as unrelated when it is not required for the selected row''s correct implementation, tests, lowering, codegen, runtime behavior, or diagnostics.',
        '- Do not mark a row complete if any part of its implementation or proof relies on a fallback path, placeholder behavior, temporary accommodation, or intentionally incomplete compiler behavior.',
        '- Add or extend comprehensive row-specific coverage in `HelloCursive` that would have failed before the fix.',
        '- All executable regression tests for updated rows must live under the `HelloCursive` project. Do not add new row-specific tests outside `HelloCursive`.',
        '- A single sentinel fixture is not enough for a row fix unless the row has only one observable behavior. Cover the full affected behavior surface: positive acceptance, negative/diagnostic rejection, default/edge cases, cross-module or generic variants, conformance-trace evidence, and runtime/semantic assertions as applicable.',
        '- Follow the existing `HelloCursive` organization and style: place runtime/semantic/conformance exercises in the relevant feature assembly/module and route them into the full `HelloCursive` executable; place compiler-diagnostic fixtures under `HelloCursive/TestProjects/...`; expose compile-time negative/diagnostic checks through `HelloCursive/CompileChecks` when the expected result is a compiler diagnostic.',
        '- Include lowering/codegen/runtime coverage when the row can affect lowered representation, generated layout, emitted calls, dispatch, or runtime behavior. If a stage truly cannot be affected by the row, state that explicitly in `SPEC_AUDIT_NOTE` and in the commit body.',
        '- Prefer extending existing feature routers, compile-check groups, generated-check style, and runtime log comparison style over inventing a new test shape.',
        '- Name the exact `HelloCursive` test files/functions, stage coverage, and pre-fix failure mechanisms in `SPEC_AUDIT_NOTE` and in the commit `Tested:` trailer.',
        '- Do not run any build, configure, test, package, bootstrap, or verification command from the assigned worktree.',
        '- Do not run `cmake`, `ctest`, `ninja`, `make`, `gmake`, `msbuild`, `scripts/build_cursive_all.sh`, `CompileChecks/Main.cursive`, `CompileChecks/build/compilechecks/bin/compilechecks.exe`, `setup_extern.ps1`, or any equivalent command from the worktree.',
        '- Do not create or rely on worktree-local `build/`, `extern/`, or other generated validation artifacts.',
        '- If you invoke Python from a shell command, use `python3`; this environment does not guarantee a `python` alias.',
        '- Create exactly one commit for this worker turn in that worktree, then end. The initial turn creates the row commit; each launcher verification retry may create exactly one follow-up commit.',
        '- Do not start the next row yourself. The launcher will transfer the verified result into the main repo, destroy this worktree, and create a fresh worktree for the next row if needed.',
        '- If you record an ambiguity in `docs/SpecDecisionsNeeded.md` for this row, end after that update and finish with `SPEC_AUDIT_STATUS: continue` so the launcher can integrate the ambiguity, destroy this worktree, and move on.'
    )
    $selection = [string]::Join("`n", $selectionLines)

    $prompt = $base + $selection
    $prompt = Replace-KeywordTrigger $prompt 'stop' 'end'
    $prompt = Replace-KeywordTrigger $prompt 'cancel' 'clear'
    $prompt = Replace-KeywordTrigger $prompt 'abort' 'terminate'
    Write-Utf8NoBom $script:IterationPromptFile $prompt
}

function Build-FailureRetryPrompt {
    $buildLog = if (Test-Path -LiteralPath $script:MainBuildLogFile) {
        Get-Content -LiteralPath $script:MainBuildLogFile -Raw
    } else {
        ''
    }
    $tail = (($buildLog -split "`r?`n") | Select-Object -Last 200) -join "`n"
    $failedLabelMatches = [regex]::Matches($buildLog, '(?m)^FAILED_COMMAND_LABEL=(.+)$')
    $commandLabelMatches = [regex]::Matches($buildLog, '(?m)^COMMAND_LABEL=(.+)$')
    $commandExitMatches = [regex]::Matches($buildLog, '(?m)^COMMAND_EXIT=(.+)$')
    $failedLabel = if ($failedLabelMatches.Count -gt 0) {
        $failedLabelMatches[$failedLabelMatches.Count - 1].Groups[1].Value
    } elseif ($commandLabelMatches.Count -gt 0) {
        $commandLabelMatches[$commandLabelMatches.Count - 1].Groups[1].Value
    } else {
        'unknown'
    }
    $exitCode = if ($commandExitMatches.Count -gt 0) {
        $commandExitMatches[$commandExitMatches.Count - 1].Groups[1].Value
    } else {
        'unknown'
    }
    $currentWorktreeHead = ((Invoke-GitCapture @('-C', $script:WorktreeDir, 'rev-parse', 'HEAD')).Output | Select-Object -Last 1).Trim()

    $promptLines = @(
        'Launcher-side verification failed for the same assigned audit row.',
        '',
        'Stay on this exact row and this exact worktree:',
        "- Assigned worktree: $($script:WorktreeDir)",
        "- Assigned branch: $($script:WorktreeBranch)",
        "- Current worktree head: $currentWorktreeHead",
        "- CSV line: $($script:SelectedRow._line)",
        "- Rule Name: $($script:SelectedRow.'rule name')",
        "- Spec Location: $($script:SelectedRow.'spec location')",
        '',
        'The launcher already applied your committed worktree changes onto the main repo and then ran launcher-owned verification there. Verification failed.',
        '',
        'Structured failure context:',
        "- Failed command label: $failedLabel",
        "- Exit code: $exitCode",
        "- Full launcher verification log: $($script:MainBuildLogFile)",
        '',
        'Rules for this retry:',
        '- Do not rescan the CSV.',
        '- Do not switch to a different row.',
        '- Do not run any build, configure, test, package, bootstrap, or verification command from the assigned worktree.',
        '- If you invoke Python from a shell command, use `python3`; this environment does not guarantee a `python` alias.',
        '- Keep or add comprehensive row-specific regression coverage in `HelloCursive` only, following the existing full-executable feature module / `TestProjects` / `CompileChecks` structure and style.',
        '- If launcher verification failed because the row proof was missing or misplaced, add the missing `HelloCursive` test and name it in `SPEC_AUDIT_NOTE` plus the commit `Tested:` trailer.',
        '- If launcher verification failed during full logged `HelloCursive` build, run, or runtime log validation, fix the row coverage or implementation so the whole project remains a complete conformance regression suite.',
        '- Fix only the issues surfaced by the launcher-side verification failure below.',
        '- Do not use temporary fixes, fallbacks, wrappers, shims, stubs, placeholders, compatibility shortcuts, or avoidance of the correct compiler work. Fix the root conformance or compiler-quality issue.',
        '- If launcher verification surfaces another spec conformance issue required for this selected row to be correct, fix that issue as part of this retry instead of working around it. Only treat a failure as unrelated when it is not required for the selected row''s correct implementation, tests, lowering, codegen, runtime behavior, or diagnostics.',
        '- Keep retry edits scoped to the selected row and row-required corollary conformance fixes. If the failure is unrelated to the selected row under that definition, report `SPEC_AUDIT_STATUS: blocked`.',
        '- Create exactly one new follow-up commit in the same assigned worktree for this retry turn, then end.',
        '- End with the same `SPEC_AUDIT_STATUS`, `SPEC_AUDIT_ITEM`, and `SPEC_AUDIT_NOTE` footer lines as usual.',
        '',
        'Launcher-side verification output tail:',
        '```text',
        $tail,
        '```'
    )
    $prompt = [string]::Join("`n", $promptLines)

    $prompt = Replace-KeywordTrigger $prompt 'stop' 'end'
    $prompt = Replace-KeywordTrigger $prompt 'cancel' 'clear'
    $prompt = Replace-KeywordTrigger $prompt 'abort' 'terminate'
    Write-Utf8NoBom $script:BuildFailurePromptFile $prompt
}

function Get-JsonProperty {
    param(
        [Parameter(Mandatory = $true, Position = 0)]
        [AllowNull()]
        $Object,
        [Parameter(Mandatory = $true, Position = 1)]
        [string]$Name
    )
    if ($null -eq $Object) { return $null }
    $prop = $Object.PSObject.Properties[$Name]
    if ($null -eq $prop) { return $null }
    return $prop.Value
}

function Get-ThreadIdFromEvents {
    if (-not (Test-Path -LiteralPath $script:AgentEventsFile)) {
        return ''
    }

    foreach ($line in Get-Content -LiteralPath $script:AgentEventsFile) {
        if ([string]::IsNullOrWhiteSpace($line)) {
            continue
        }
        try {
            $event = $line | ConvertFrom-Json -ErrorAction Stop
        } catch {
            continue
        }
        if ((Get-JsonProperty $event 'type') -eq 'thread.started') {
            return [string](Get-JsonProperty $event 'thread_id')
        }
    }

    return ''
}

function Write-AgentEventSummary(
    [string]$Line,
    [ref]$SeenThread,
    [ref]$SeenAgentMessage,
    [ref]$SeenCommand
) {
    if ([string]::IsNullOrWhiteSpace($Line)) {
        return
    }

    try {
        $event = $Line | ConvertFrom-Json -ErrorAction Stop
    } catch {
        return
    }

    $type = Get-JsonProperty $event 'type'

    if (-not $SeenThread.Value -and $type -eq 'thread.started') {
        $threadId = [string](Get-JsonProperty $event 'thread_id')
        if ($threadId) {
            Write-Log "Codex session started: $threadId"
        } else {
            Write-Log 'Codex session started'
        }
        $SeenThread.Value = $true
        return
    }

    $item = Get-JsonProperty $event 'item'
    if ($null -eq $item) {
        return
    }

    $itemType = Get-JsonProperty $item 'type'

    if (-not $SeenAgentMessage.Value -and $type -eq 'item.completed' -and $itemType -eq 'agent_message') {
        $text = [string](Get-JsonProperty $item 'text')
        $text = ($text -split '\s+' | Where-Object { $_ }) -join ' '
        if ($text) {
            if ($text.Length -gt 200) {
                $text = $text.Substring(0, 197) + '...'
            }
            Write-Log "worker: $text"
            $SeenAgentMessage.Value = $true
        }
        return
    }

    if (-not $SeenCommand.Value -and $type -eq 'item.started' -and $itemType -eq 'command_execution') {
        $command = [string](Get-JsonProperty $item 'command')
        $command = ($command -split '\s+' | Where-Object { $_ }) -join ' '
        if ($command) {
            if ($command.Length -gt 160) {
                $command = $command.Substring(0, 157) + '...'
            }
            Write-Log "worker command: $command"
            $SeenCommand.Value = $true
        }
    }
}

function Run-AgentTurn([string]$Mode, [string]$PromptPath) {
    Write-Utf8NoBom $script:AgentEventsFile ''

    $codexArgs = [System.Collections.Generic.List[string]]::new()
    if ($Mode -eq 'initial') {
        Write-Log "launching Codex worker ($Mode)"
        foreach ($value in @(
            'exec',
            '--json',
            '-C', $script:WorktreeDir,
            '-s', 'danger-full-access',
            '-m', $script:Model,
            '-c', "model_reasoning_effort=`"$($script:ReasoningEffort)`"",
            '-o', $script:LastMessageFile
        )) {
            $null = $codexArgs.Add($value)
        }
        if (-not [string]::IsNullOrWhiteSpace($script:Profile)) {
            $null = $codexArgs.Add('-p')
            $null = $codexArgs.Add($script:Profile)
        }
        $null = $codexArgs.Add('-')
    } else {
        Write-Log "launching Codex worker ($Mode)"
        foreach ($value in @(
            'exec', 'resume',
            '--json',
            $script:WorktreeSessionId,
            '-m', $script:Model,
            '-c', "model_reasoning_effort=`"$($script:ReasoningEffort)`"",
            '-o', $script:LastMessageFile,
            '-'
        )) {
            $null = $codexArgs.Add($value)
        }
    }

    $promptText = Get-Content -LiteralPath $PromptPath -Raw
    $seenThread = $false
    $seenAgentMessage = $false
    $seenCommand = $false

    $previousEap = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $null = $promptText |
            & $script:CodexExe @($codexArgs.ToArray()) 2>&1 |
            Tee-Object -FilePath $script:AgentEventsFile |
            ForEach-Object {
                Write-AgentEventSummary ([string]$_) ([ref]$seenThread) ([ref]$seenAgentMessage) ([ref]$seenCommand)
            }

        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousEap
    }
    if ($exitCode -ne 0) {
        [Console]::Error.WriteLine("[spec-audit-loop] Codex exited with code $exitCode")
        if ($script:WorktreeDir -and (Test-Path -LiteralPath $script:WorktreeDir)) {
            $preservedEvents = Join-Path $script:WorktreeDir 'spec-audit-events.jsonl'
            $preservedPrompt = Join-Path $script:WorktreeDir 'spec-audit-prompt.md'
            if (Test-Path -LiteralPath $script:AgentEventsFile) {
                Copy-Item -LiteralPath $script:AgentEventsFile -Destination $preservedEvents -Force -ErrorAction SilentlyContinue
                [Console]::Error.WriteLine("[spec-audit-loop] preserved events log: $preservedEvents")
            }
            if ($PromptPath -and (Test-Path -LiteralPath $PromptPath)) {
                Copy-Item -LiteralPath $PromptPath -Destination $preservedPrompt -Force -ErrorAction SilentlyContinue
                [Console]::Error.WriteLine("[spec-audit-loop] preserved prompt: $preservedPrompt")
            }
        }
        if (Test-Path -LiteralPath $script:AgentEventsFile) {
            $tail = Get-Content -LiteralPath $script:AgentEventsFile -Tail 20 -ErrorAction SilentlyContinue
            if ($tail) {
                [Console]::Error.WriteLine("[spec-audit-loop] last events from Codex:")
                foreach ($line in $tail) {
                    [Console]::Error.WriteLine("  $line")
                }
            } else {
                [Console]::Error.WriteLine("[spec-audit-loop] events log is empty")
            }
        }
        return $false
    }

    if ($Mode -eq 'initial') {
        $script:WorktreeSessionId = Get-ThreadIdFromEvents
        if ([string]::IsNullOrWhiteSpace($script:WorktreeSessionId)) {
            [Console]::Error.WriteLine("[spec-audit-loop] failed to capture Codex session id for worktree $($script:WorktreeDir)")
            return $false
        }
    }

    return $true
}

function Assert-MainRepoReadyForIntegration {
    $status = Invoke-GitCapture @('-C', $script:RepoRoot, 'status', '--porcelain', '--untracked-files=no')
    $unrelated = @()
    foreach ($line in $status.Output) {
        if ([string]::IsNullOrWhiteSpace($line) -or $line.Length -lt 4) {
            continue
        }
        $relpath = $line.Substring(3)
        if ($relpath -in @('docs/audit/SPEC_RULE_TABLE_BY_PHASE.csv', 'docs/SpecDecisionsNeeded.md')) {
            continue
        }
        $unrelated += $line
    }

    if ($unrelated.Count -gt 0) {
        Write-Log 'continuing with unrelated main-repo changes present:'
        $unrelated | ForEach-Object { Write-Host "  $_" }
    }

    return $true
}

# MARKER: INTEGRATION

function Stage-SelectedAuditRow {
    $headCsv = (Invoke-GitCapture @('-C', $script:RepoRoot, 'show', 'HEAD:docs/audit/SPEC_RULE_TABLE_BY_PHASE.csv')).Output
    Write-LinesUtf8NoBom $script:HeadAuditCsvFile $headCsv

$scriptText = @'
import csv
import sys
from pathlib import Path

root_path = Path(sys.argv[1])
worktree_path = Path(sys.argv[2])
head_path = Path(sys.argv[3])
staged_path = Path(sys.argv[4])
target_raw_index = int(sys.argv[5])


def load_rows(path: Path):
    with path.open(encoding="utf-8", newline="") as fh:
        rows = list(csv.reader(fh))
    if not rows or not rows[0]:
        raise RuntimeError(f"missing CSV header in {path}")
    return rows


def extract_state(rows, raw_index):
    if raw_index < 1 or raw_index >= len(rows):
        raise RuntimeError(f"raw row index {raw_index} out of range")
    row = list(rows[raw_index])
    while len(row) < 6:
        row.append("")
    implemented = row[1].strip()
    compiler_location = ",".join(row[5:])
    return implemented, compiler_location


def update_target(rows, raw_index, implemented, compiler_location):
    if raw_index < 1 or raw_index >= len(rows):
        raise RuntimeError(f"raw row index {raw_index} out of range")
    row = list(rows[raw_index])
    while len(row) < 6:
        row.append("")
    row[1] = implemented
    row[5] = compiler_location
    del row[6:]
    rows[raw_index] = row


def write_rows(path: Path, rows):
    with path.open("w", encoding="utf-8", newline="") as fh:
        writer = csv.writer(fh)
        writer.writerows(rows)


root_rows = load_rows(root_path)
worktree_rows = load_rows(worktree_path)
head_rows = load_rows(head_path)
if len(worktree_rows) != len(head_rows):
    raise RuntimeError(
        f"audit CSV row count changed in worktree ({len(worktree_rows)} != {len(head_rows)})"
    )

def normalized(row):
    cells = list(row)
    while len(cells) < 6:
        cells.append("")
    cells[5] = ",".join(cells[5:])
    del cells[6:]
    return cells

for index, (worktree_row, head_row) in enumerate(zip(worktree_rows, head_rows)):
    if index == 0:
        if worktree_row != head_row:
            raise RuntimeError("audit CSV header changed in worktree")
        continue
    if index == target_raw_index:
        worktree_norm = normalized(worktree_row)
        head_norm = normalized(head_row)
        if worktree_norm[:1] + worktree_norm[2:5] != head_norm[:1] + head_norm[2:5]:
            raise RuntimeError("selected audit row changed fields other than implemented/compiler location")
        continue
    if normalized(worktree_row) != normalized(head_row):
        raise RuntimeError(f"non-selected audit row changed at raw row index {index}")

implemented, compiler_location = extract_state(worktree_rows, target_raw_index)
if implemented not in {"complete", "ambiguous"}:
    raise RuntimeError(
        f"selected row ended in unexpected state `{implemented}`; expected complete or ambiguous"
    )

update_target(root_rows, target_raw_index, implemented, compiler_location)
update_target(head_rows, target_raw_index, implemented, compiler_location)
write_rows(root_path, root_rows)
write_rows(staged_path, head_rows)
'@

    $null = Invoke-PythonScript $scriptText @(
        $script:AuditCsv,
        (Join-RepoPath $script:WorktreeDir 'docs/audit/SPEC_RULE_TABLE_BY_PHASE.csv'),
        $script:HeadAuditCsvFile,
        $script:StagedAuditCsvFile,
        [string]$script:SelectedRowRawIndex
    )

    $blobSha = ((Invoke-GitCapture @('-C', $script:RepoRoot, 'hash-object', '-w', $script:StagedAuditCsvFile)).Output | Select-Object -Last 1).Trim()
    $null = Invoke-GitCapture @(
        '-C', $script:RepoRoot,
        'update-index',
        '--add',
        '--cacheinfo',
        '100644', $blobSha, 'docs/audit/SPEC_RULE_TABLE_BY_PHASE.csv'
    )
}

function Stage-SpecDecisionsAppendIfNeeded {
    $relpath = 'docs/SpecDecisionsNeeded.md'
    & $script:GitExe -C $script:WorktreeDir diff --quiet $script:WorktreeBaseCommit $script:WorktreeCommit -- $relpath
    $diffExit = $LASTEXITCODE
    if ($diffExit -eq 0) {
        return
    }
    if ($diffExit -ne 1) {
        throw "git diff failed while checking $relpath"
    }

    $worktreeFile = Join-RepoPath $script:WorktreeDir $relpath
    & $script:GitExe -C $script:RepoRoot cat-file -e "HEAD:$relpath" 2>$null
    if ($LASTEXITCODE -eq 0) {
        $head = (Invoke-GitCapture @('-C', $script:RepoRoot, 'show', "HEAD:$relpath")).Output
        Write-LinesUtf8NoBom $script:HeadSpecDecisionsFile $head
    } else {
        Write-Utf8NoBom $script:HeadSpecDecisionsFile ''
    }

$scriptText = @'
import sys
from pathlib import Path

base_path = Path(sys.argv[1])
worktree_path = Path(sys.argv[2])
root_path = Path(sys.argv[3])
head_path = Path(sys.argv[4])
staged_path = Path(sys.argv[5])

base = base_path.read_text(encoding="utf-8") if base_path.exists() else ""
worktree = worktree_path.read_text(encoding="utf-8") if worktree_path.exists() else ""
root = root_path.read_text(encoding="utf-8") if root_path.exists() else ""
head = head_path.read_text(encoding="utf-8") if head_path.exists() else ""

if not worktree.startswith(base):
    raise RuntimeError(
        "docs/SpecDecisionsNeeded.md was not append-only within the worktree; refusing automatic integration"
    )

suffix = worktree[len(base):]
if suffix:
    root_path.write_text(root + suffix, encoding="utf-8")
    staged_path.write_text(head + suffix, encoding="utf-8")
else:
    staged_path.write_text(head, encoding="utf-8")
'@

    $null = Invoke-PythonScript $scriptText @(
        $script:BaseDecisionsSnapshotFile,
        $worktreeFile,
        $script:SpecDecisionsFile,
        $script:HeadSpecDecisionsFile,
        $script:StagedSpecDecisionsFile
    )

    $blobSha = ((Invoke-GitCapture @('-C', $script:RepoRoot, 'hash-object', '-w', $script:StagedSpecDecisionsFile)).Output | Select-Object -Last 1).Trim()
    $null = Invoke-GitCapture @(
        '-C', $script:RepoRoot,
        'update-index',
        '--add',
        '--cacheinfo',
        '100644', $blobSha, $relpath
    )
}

function Get-NonSharedPaths {
    if (-not $script:NonSharedPathsFile -or -not (Test-Path -LiteralPath $script:NonSharedPathsFile)) {
        return @()
    }
    return @(Get-Content -LiteralPath $script:NonSharedPathsFile | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
}

function Is-NonSharedPath([string]$RelPath) {
    foreach ($path in Get-NonSharedPaths) {
        if ($path -eq $RelPath) {
            return $true
        }
    }
    return $false
}

function Is-LauncherBuildSideEffectPath([string]$RelPath) {
    return $RelPath -in @(
        'cursive/src/00_core/generated/static_rule_registry.inc',
        'cursive/src/00_core/generated/diag_registry.inc',
        'cursive/src/04_analysis/typing/item/typecheck_diag_map.inc'
    )
}

function Apply-NonSharedChangesToMainWorktree {
    $diff = Invoke-GitCapture @(
        '-C', $script:WorktreeDir,
        'diff',
        '--name-status',
        '--no-renames',
        $script:WorktreeBaseCommit,
        $script:WorktreeCommit
    )

    Write-Utf8NoBom $script:NonSharedPathsFile ''
    foreach ($line in $diff.Output) {
        if ([string]::IsNullOrWhiteSpace($line)) {
            continue
        }

        $parts = $line -split "`t", 2
        if ($parts.Count -lt 2) {
            continue
        }

        $change = $parts[0]
        $relpath = $parts[1]
        if ($relpath -in @('docs/audit/SPEC_RULE_TABLE_BY_PHASE.csv', 'docs/SpecDecisionsNeeded.md')) {
            continue
        }

        $status = Invoke-GitCapture @('-C', $script:RepoRoot, 'status', '--porcelain', '--', $relpath)
        if ($status.Output.Count -gt 0) {
            [Console]::Error.WriteLine("[spec-audit-loop] refusing integration; main repo already has local changes in $relpath")
            return $false
        }

        Add-Content -LiteralPath $script:NonSharedPathsFile -Value $relpath -Encoding utf8

        $rootTarget = Join-RepoPath $script:RepoRoot $relpath
        $worktreeTarget = Join-RepoPath $script:WorktreeDir $relpath
        $backupTarget = Join-RepoPath $script:IntegrationBackupDir $relpath
        $backupParent = Split-Path -Parent $backupTarget
        if ($backupParent) {
            New-Item -ItemType Directory -Path $backupParent -Force | Out-Null
        }
        $absentMarker = "$backupTarget.absent"

        if (Test-Path -LiteralPath $rootTarget) {
            Copy-Item -LiteralPath $rootTarget -Destination $backupTarget -Force
            if (Test-Path -LiteralPath $absentMarker) {
                Remove-Item -LiteralPath $absentMarker -Force
            }
        } else {
            Write-Utf8NoBom $absentMarker ''
        }

        switch ($change) {
            'A' {
                $parent = Split-Path -Parent $rootTarget
                if ($parent) { New-Item -ItemType Directory -Path $parent -Force | Out-Null }
                Copy-Item -LiteralPath $worktreeTarget -Destination $rootTarget -Force
            }
            'M' {
                $parent = Split-Path -Parent $rootTarget
                if ($parent) { New-Item -ItemType Directory -Path $parent -Force | Out-Null }
                Copy-Item -LiteralPath $worktreeTarget -Destination $rootTarget -Force
            }
            'D' {
                if (Test-Path -LiteralPath $rootTarget) {
                    Remove-Item -LiteralPath $rootTarget -Force
                }
            }
            default {
                [Console]::Error.WriteLine("[spec-audit-loop] unsupported diff status `$change` for $relpath")
                return $false
            }
        }
    }

    return $true
}

function Restore-NonSharedChangesInMainWorktree {
    foreach ($relpath in Get-NonSharedPaths) {
        $rootTarget = Join-RepoPath $script:RepoRoot $relpath
        $backupTarget = Join-RepoPath $script:IntegrationBackupDir $relpath
        $absentMarker = "$backupTarget.absent"
        if (Test-Path -LiteralPath $absentMarker) {
            if (Test-Path -LiteralPath $rootTarget) {
                Remove-Item -LiteralPath $rootTarget -Force
            }
            continue
        }

        if (Test-Path -LiteralPath $backupTarget) {
            $parent = Split-Path -Parent $rootTarget
            if ($parent) { New-Item -ItemType Directory -Path $parent -Force | Out-Null }
            Copy-Item -LiteralPath $backupTarget -Destination $rootTarget -Force
        }
    }
}

function Restore-LauncherBuildSideEffectsInMainWorktree {
    $status = Invoke-GitCapture @('-C', $script:RepoRoot, 'status', '--porcelain', '--untracked-files=no')
    foreach ($line in $status.Output) {
        if ([string]::IsNullOrWhiteSpace($line) -or $line.Length -lt 4) {
            continue
        }
        $relpath = $line.Substring(3)
        if ($relpath -in @('docs/audit/SPEC_RULE_TABLE_BY_PHASE.csv', 'docs/SpecDecisionsNeeded.md')) {
            continue
        }
        if (Is-NonSharedPath $relpath) {
            continue
        }
        if (-not (Is-LauncherBuildSideEffectPath $relpath)) {
            continue
        }

        & $script:GitExe -C $script:RepoRoot ls-files --error-unmatch -- $relpath *> $null
        if ($LASTEXITCODE -eq 0) {
            $null = Invoke-GitCapture @($script:GitCheckoutHookArgs + @('-C', $script:RepoRoot, 'restore', '--worktree', '--source=HEAD', '--', $relpath))
        }
    }
}

function Stage-NonSharedChangesInMainRepo {
    foreach ($relpath in Get-NonSharedPaths) {
        $rootTarget = Join-RepoPath $script:RepoRoot $relpath
        if (Test-Path -LiteralPath $rootTarget) {
            $null = Invoke-GitCapture @('-C', $script:RepoRoot, 'add', '--', $relpath)
        } else {
            & $script:GitExe -C $script:RepoRoot ls-files --error-unmatch -- $relpath *> $null
            if ($LASTEXITCODE -eq 0) {
                $null = Invoke-GitCapture @('-C', $script:RepoRoot, 'rm', '--quiet', '--', $relpath)
            }
        }
    }
}

function Stage-LauncherBuildSideEffectsInMainRepo {
    $status = Invoke-GitCapture @('-C', $script:RepoRoot, 'status', '--porcelain', '--untracked-files=no')
    foreach ($line in $status.Output) {
        if ([string]::IsNullOrWhiteSpace($line) -or $line.Length -lt 4) {
            continue
        }
        $relpath = $line.Substring(3)
        if ($relpath -in @('docs/audit/SPEC_RULE_TABLE_BY_PHASE.csv', 'docs/SpecDecisionsNeeded.md')) {
            continue
        }
        if (Is-NonSharedPath $relpath) {
            continue
        }
        if (-not (Is-LauncherBuildSideEffectPath $relpath)) {
            continue
        }

        $rootTarget = Join-RepoPath $script:RepoRoot $relpath
        if (Test-Path -LiteralPath $rootTarget) {
            $null = Invoke-GitCapture @('-C', $script:RepoRoot, 'add', '--', $relpath)
        } else {
            & $script:GitExe -C $script:RepoRoot ls-files --error-unmatch -- $relpath *> $null
            if ($LASTEXITCODE -eq 0) {
                $null = Invoke-GitCapture @('-C', $script:RepoRoot, 'rm', '--quiet', '--', $relpath)
            }
        }
    }
}

function Unstage-LauncherIntegrationPaths {
    $paths = New-Object System.Collections.Generic.List[string]
    $paths.Add('docs/audit/SPEC_RULE_TABLE_BY_PHASE.csv')
    $paths.Add('docs/SpecDecisionsNeeded.md')
    $paths.Add('cursive/src/00_core/generated/static_rule_registry.inc')
    $paths.Add('cursive/src/00_core/generated/diag_registry.inc')
    $paths.Add('cursive/src/04_analysis/typing/item/typecheck_diag_map.inc')

    foreach ($path in Get-NonSharedPaths) {
        if (-not [string]::IsNullOrWhiteSpace($path)) {
            $paths.Add($path)
        }
    }

    $null = Invoke-GitCapture @(@('-C', $script:RepoRoot, 'reset', '-q', 'HEAD', '--') + $paths.ToArray()) -AllowFailure
}

function Recover-StatusFooterFromWorktree {
$scriptText = @'
import csv
import subprocess
import sys
from pathlib import Path

worktree_dir = Path(sys.argv[1])
csv_path = Path(sys.argv[2])
raw_index = int(sys.argv[3])
base_commit = sys.argv[4]
selected_item = sys.argv[5]

status = subprocess.run(
    ["git", "-C", str(worktree_dir), "status", "--porcelain", "--untracked-files=no"],
    text=True,
    capture_output=True,
    check=False,
)
if status.stdout.strip():
    sys.exit(1)

commit_count = subprocess.run(
    ["git", "-C", str(worktree_dir), "rev-list", "--count", f"{base_commit}..HEAD"],
    text=True,
    capture_output=True,
    check=False,
)
if commit_count.returncode != 0 or int(commit_count.stdout.strip() or "0") < 1:
    sys.exit(1)

with csv_path.open(encoding="utf-8", newline="") as fh:
    rows = list(csv.reader(fh))

if raw_index < 1 or raw_index >= len(rows):
    sys.exit(1)

row = list(rows[raw_index])
while len(row) < 6:
    row.append("")
implemented = row[1].strip()

if implemented not in {"complete", "ambiguous"}:
    sys.exit(1)

note = (
    f"Recovered {implemented} row state from committed worktree after worker output omitted "
    f"SPEC_AUDIT_STATUS footer; proceeding with launcher integration for {selected_item}."
)

print("SPEC_AUDIT_STATUS: continue")
print(f"SPEC_AUDIT_ITEM: {selected_item}")
print(f"SPEC_AUDIT_NOTE: {note}")
'@

    $result = Invoke-PythonScript $scriptText @(
        $script:WorktreeDir,
        (Join-RepoPath $script:WorktreeDir 'docs/audit/SPEC_RULE_TABLE_BY_PHASE.csv'),
        [string]$script:SelectedRowRawIndex,
        $script:WorktreeBaseCommit,
        $script:SelectedItem
    ) -AllowFailure

    if ($result.ExitCode -ne 0) {
        return $null
    }

    return @($result.Output)
}

function Run-MainRepoWindowsBuild {
    Write-Log "Verifying main-repo Windows build and HelloCursive with configured preset: $($script:WindowsPreset)"
    $cmakeCmd = Get-Command 'cmake.exe' -ErrorAction SilentlyContinue
    if (-not $cmakeCmd) {
        $cmakeCmd = Get-Command 'cmake' -ErrorAction SilentlyContinue
    }
    if (-not $cmakeCmd) {
        throw 'cmake was not found in PATH.'
    }
    $powershellCmd = Get-Command 'powershell.exe' -ErrorAction SilentlyContinue
    if (-not $powershellCmd) {
        $powershellCmd = Get-Command 'powershell' -ErrorAction SilentlyContinue
    }
    if (-not $powershellCmd) {
        throw 'powershell was not found in PATH.'
    }

    $sourceDir = Join-Path $script:RepoRoot 'cursive'
    $helloDir = Join-Path $script:RepoRoot 'HelloCursive'
    Push-Location $sourceDir
    try {
        $previousEap = $ErrorActionPreference
        $ErrorActionPreference = 'Continue'
        try {
            'COMMAND_LABEL=windows-build' | Tee-Object -FilePath $script:MainBuildLogFile -Append
            $null = & $cmakeCmd.Source --build --preset $script:WindowsPreset --target $script:WindowsTarget 2>&1 |
                Tee-Object -FilePath $script:MainBuildLogFile -Append
            $buildExitCode = $LASTEXITCODE
            "COMMAND_EXIT=$buildExitCode" | Tee-Object -FilePath $script:MainBuildLogFile -Append
        } finally {
            $ErrorActionPreference = $previousEap
        }
        if ($buildExitCode -ne 0) {
            'FAILED_COMMAND_LABEL=windows-build' | Tee-Object -FilePath $script:MainBuildLogFile -Append
            return $false
        }

        if (-not [string]::IsNullOrWhiteSpace($script:CompilerPathOverride)) {
            $compilerPath = $script:CompilerPathOverride
        } elseif ($script:WindowsPreset -eq 'windows-debug' -and $script:WindowsTarget -eq 'cursive_out') {
            $compilerPath = Join-Path $sourceDir 'build\windows\out\cursive.exe'
        } else {
            throw "unknown compiler artifact mapping for preset=$($script:WindowsPreset) target=$($script:WindowsTarget); set SPEC_AUDIT_COMPILER_PATH"
        }
        if (-not (Test-Path -LiteralPath $compilerPath)) {
            'FAILED_COMMAND_LABEL=resolve-compiler' | Tee-Object -FilePath $script:MainBuildLogFile -Append
            throw "Missing expected compiler artifact: $compilerPath"
        }
        $compilerPath = (Resolve-Path -LiteralPath $compilerPath).Path
        $compilerInfo = Get-Item -LiteralPath $compilerPath
        $script:VerifiedCompilerPath = $compilerPath
        "CompilerPath=$compilerPath" | Tee-Object -FilePath $script:MainBuildLogFile -Append
        "CompilerLastWriteTimeUtc=$([DateTime]::SpecifyKind($compilerInfo.LastWriteTimeUtc, [DateTimeKind]::Utc).ToString('o'))" | Tee-Object -FilePath $script:MainBuildLogFile -Append

        $compileChecksDir = Join-Path $helloDir 'CompileChecks'
        Push-Location $compileChecksDir
        try {
            'COMMAND_LABEL=hello-cursive-compilechecks-build' | Tee-Object -FilePath $script:MainBuildLogFile -Append
            "HelloCursiveCompilerPath=$compilerPath" | Tee-Object -FilePath $script:MainBuildLogFile -Append
            $previousEap = $ErrorActionPreference
            $ErrorActionPreference = 'Continue'
            try {
                $null = & $compilerPath --incremental off --diag-json --quiet Main.cursive 2>&1 |
                    Tee-Object -FilePath $script:MainBuildLogFile -Append
                $helloBuildExitCode = $LASTEXITCODE
            } finally {
                $ErrorActionPreference = $previousEap
            }
            "COMMAND_EXIT=$helloBuildExitCode" | Tee-Object -FilePath $script:MainBuildLogFile -Append
            if ($helloBuildExitCode -ne 0) {
                'FAILED_COMMAND_LABEL=hello-cursive-compilechecks-build' | Tee-Object -FilePath $script:MainBuildLogFile -Append
                return $false
            }

            'COMMAND_LABEL=hello-cursive-compilechecks-run' | Tee-Object -FilePath $script:MainBuildLogFile -Append
            $compileChecksExe = Join-Path $compileChecksDir 'build\compilechecks\bin\compilechecks.exe'
            if (-not (Test-Path -LiteralPath $compileChecksExe)) {
                'FAILED_COMMAND_LABEL=hello-cursive-compilechecks-exe' | Tee-Object -FilePath $script:MainBuildLogFile -Append
                return $false
            }
            $previousCompilerUnderTest = $env:CURSIVE_COMPILER_UNDER_TEST
            $env:CURSIVE_COMPILER_UNDER_TEST = $compilerPath
            try {
                $null = & $compileChecksExe 2>&1 | Tee-Object -FilePath $script:MainBuildLogFile -Append
                $helloExitCode = $LASTEXITCODE
            } finally {
                if ($null -eq $previousCompilerUnderTest) {
                    Remove-Item Env:CURSIVE_COMPILER_UNDER_TEST -ErrorAction SilentlyContinue
                } else {
                    $env:CURSIVE_COMPILER_UNDER_TEST = $previousCompilerUnderTest
                }
            }
            "COMMAND_EXIT=$helloExitCode" | Tee-Object -FilePath $script:MainBuildLogFile -Append
            if ($helloExitCode -ne 0) {
                'FAILED_COMMAND_LABEL=hello-cursive-compilechecks-run' | Tee-Object -FilePath $script:MainBuildLogFile -Append
                return $false
            }
        } finally {
            Pop-Location
        }

        Push-Location $helloDir
        try {
            $helloRuntimeLogDir = Join-Path $helloDir 'build\main\logs\runtime'
            New-Item -ItemType Directory -Force -Path $helloRuntimeLogDir | Out-Null
            $helloRuntimeLog = Join-Path $helloRuntimeLogDir 'spec-audit-runtime-validation.log'
            Remove-Item -LiteralPath $helloRuntimeLog -Force -ErrorAction SilentlyContinue

            'COMMAND_LABEL=hello-cursive-full-build' | Tee-Object -FilePath $script:MainBuildLogFile -Append
            "HelloCursiveCompilerPath=$compilerPath" | Tee-Object -FilePath $script:MainBuildLogFile -Append
            "HelloCursiveRuntimeLog=$helloRuntimeLog" | Tee-Object -FilePath $script:MainBuildLogFile -Append
            $previousEap = $ErrorActionPreference
            $ErrorActionPreference = 'Continue'
            try {
                $null = & $compilerPath --incremental off --diag-json --quiet --log-file $helloRuntimeLog 'Main\Main.cursive' 2>&1 |
                    Tee-Object -FilePath $script:MainBuildLogFile -Append
                $helloFullBuildExitCode = $LASTEXITCODE
            } finally {
                $ErrorActionPreference = $previousEap
            }
            "COMMAND_EXIT=$helloFullBuildExitCode" | Tee-Object -FilePath $script:MainBuildLogFile -Append
            if ($helloFullBuildExitCode -ne 0) {
                'FAILED_COMMAND_LABEL=hello-cursive-full-build' | Tee-Object -FilePath $script:MainBuildLogFile -Append
                return $false
            }

            $helloMainExe = Join-Path $helloDir 'build\main\bin\main.exe'
            if (-not (Test-Path -LiteralPath $helloMainExe)) {
                'FAILED_COMMAND_LABEL=hello-cursive-full-exe' | Tee-Object -FilePath $script:MainBuildLogFile -Append
                return $false
            }

            'COMMAND_LABEL=hello-cursive-full-run' | Tee-Object -FilePath $script:MainBuildLogFile -Append
            $null = & $helloMainExe 2>&1 | Tee-Object -FilePath $script:MainBuildLogFile -Append
            $helloFullRunExitCode = $LASTEXITCODE
            "COMMAND_EXIT=$helloFullRunExitCode" | Tee-Object -FilePath $script:MainBuildLogFile -Append
            if ($helloFullRunExitCode -ne 0) {
                'FAILED_COMMAND_LABEL=hello-cursive-full-run' | Tee-Object -FilePath $script:MainBuildLogFile -Append
                return $false
            }

            'COMMAND_LABEL=hello-cursive-full-log-validate' | Tee-Object -FilePath $script:MainBuildLogFile -Append
            if (-not (Test-Path -LiteralPath $helloRuntimeLog)) {
                'FAILED_COMMAND_LABEL=hello-cursive-full-log-missing' | Tee-Object -FilePath $script:MainBuildLogFile -Append
                return $false
            }

            $helloRuntimeLogText = Get-Content -LiteralPath $helloRuntimeLog -Raw -ErrorAction Stop
            $helloRuntimeLogLines = if ([string]::IsNullOrEmpty($helloRuntimeLogText)) {
                0
            } else {
                ($helloRuntimeLogText -split "`r?`n").Count
            }
            $helloRuntimeCmpPass = [regex]::Matches($helloRuntimeLogText, 'cmp(?:=|%3D)pass').Count
            $helloRuntimeCmpFail = [regex]::Matches($helloRuntimeLogText, 'cmp(?:=|%3D)fail').Count
            $helloRuntimeErrorLines = [regex]::Matches($helloRuntimeLogText, '(?im)(?:level(?:=|%3D)error|\[error\])').Count
            "HelloCursiveRuntimeLog=$helloRuntimeLog" | Tee-Object -FilePath $script:MainBuildLogFile -Append
            "HelloCursiveRuntimeLogLines=$helloRuntimeLogLines" | Tee-Object -FilePath $script:MainBuildLogFile -Append
            "HelloCursiveRuntimeCmpPass=$helloRuntimeCmpPass" | Tee-Object -FilePath $script:MainBuildLogFile -Append
            "HelloCursiveRuntimeCmpFail=$helloRuntimeCmpFail" | Tee-Object -FilePath $script:MainBuildLogFile -Append
            "HelloCursiveRuntimeErrorLines=$helloRuntimeErrorLines" | Tee-Object -FilePath $script:MainBuildLogFile -Append
            $helloRuntimeValidationExit = 0
            if ($helloRuntimeLogLines -le 0 -or $helloRuntimeCmpPass -le 0 -or $helloRuntimeCmpFail -ne 0 -or $helloRuntimeErrorLines -ne 0) {
                $helloRuntimeValidationExit = 1
            }
            "COMMAND_EXIT=$helloRuntimeValidationExit" | Tee-Object -FilePath $script:MainBuildLogFile -Append
            if ($helloRuntimeValidationExit -ne 0) {
                'FAILED_COMMAND_LABEL=hello-cursive-full-log-validate' | Tee-Object -FilePath $script:MainBuildLogFile -Append
                return $false
            }
        } finally {
            Pop-Location
        }

        return $true
    } finally {
        Pop-Location
    }
}

function Integrate-VerifiedCommitIntoMainRepo {
    $null = Assert-MainRepoReadyForIntegration

    $commitCountText = ((Invoke-GitCapture @('-C', $script:WorktreeDir, 'rev-list', '--count', "$($script:WorktreeBaseCommit)..HEAD")).Output | Select-Object -Last 1).Trim()
    $commitCount = [int]$commitCountText
    if ($commitCount -lt 1) {
        [Console]::Error.WriteLine("[spec-audit-loop] expected at least one worktree commit for the row, found $commitCount in $($script:WorktreeBaseCommit)..HEAD")
        return 1
    }

    $script:WorktreeCommit = ((Invoke-GitCapture @('-C', $script:WorktreeDir, 'rev-parse', 'HEAD')).Output | Select-Object -Last 1).Trim()
    $latestMessage = (Invoke-GitCapture @('-C', $script:WorktreeDir, 'log', '-1', '--format=%B', $script:WorktreeCommit)).Output
    Write-LinesUtf8NoBom $script:CommitMessageFile $latestMessage

    if ($commitCount -gt 1) {
        $related = (Invoke-GitCapture @('-C', $script:WorktreeDir, 'rev-list', '--reverse', "$($script:WorktreeBaseCommit)..HEAD~")).Output
        if ($related.Count -gt 0) {
            Add-Content -LiteralPath $script:CommitMessageFile -Value ''
            Add-Content -LiteralPath $script:CommitMessageFile -Value ('Related: prior worktree retry commits ' + (($related | ForEach-Object { "$_" }) -join ', '))
        }
    }

    $script:IntegrationBackupDir = New-TempDirectory 'spec-audit-main-overlay-'
    $script:NonSharedPathsFile = New-TempFile 'spec-audit-main-overlay-paths-' '.txt'
    Write-Utf8NoBom $script:NonSharedPathsFile ''

    Update-ClaimLedgerStage 'overlaying' 'applying worker changes to main checkout'
    $script:OverlayStarted = $true
    if (-not (Apply-NonSharedChangesToMainWorktree)) {
        Restore-NonSharedChangesInMainWorktree
        Restore-LauncherBuildSideEffectsInMainWorktree
        $script:OverlayStarted = $false
        return 1
    }

    Update-ClaimLedgerStage 'verifying' 'running launcher-owned Windows build and HelloCursive'
    try {
        $verificationOk = Run-MainRepoWindowsBuild
    } catch {
        Restore-NonSharedChangesInMainWorktree
        Restore-LauncherBuildSideEffectsInMainWorktree
        $script:OverlayStarted = $false
        Add-Content -LiteralPath $script:MainBuildLogFile -Value ('FAILED_COMMAND_LABEL=verification-exception')
        Add-Content -LiteralPath $script:MainBuildLogFile -Value ('COMMAND_EXIT=exception')
        Add-Content -LiteralPath $script:MainBuildLogFile -Value $_.Exception.Message
        [Console]::Error.WriteLine('[spec-audit-loop] launcher verification threw after applying worktree changes; restored main-repo files')
        return 2
    }
    if (-not $verificationOk) {
        Restore-NonSharedChangesInMainWorktree
        Restore-LauncherBuildSideEffectsInMainWorktree
        $script:OverlayStarted = $false
        [Console]::Error.WriteLine('[spec-audit-loop] launcher verification failed after applying worktree changes; restored main-repo files')
        return 2
    }

    Add-Content -LiteralPath $script:CommitMessageFile -Value ''
    Add-Content -LiteralPath $script:CommitMessageFile -Value ("Tested: cmake --build --preset $($script:WindowsPreset) --target $($script:WindowsTarget)")
    Add-Content -LiteralPath $script:CommitMessageFile -Value ("Tested: Windows build; HelloCursive/CompileChecks build/run; full logged HelloCursive build/run with runtime log validation -CompilerPath $($script:VerifiedCompilerPath)")

    Update-ClaimLedgerStage 'staging' 'staging verified main-repo changes'
    try {
        Stage-NonSharedChangesInMainRepo
        Stage-LauncherBuildSideEffectsInMainRepo
        Stage-SelectedAuditRow
        Stage-SpecDecisionsAppendIfNeeded
    } catch {
        Unstage-LauncherIntegrationPaths
        Restore-NonSharedChangesInMainWorktree
        Restore-LauncherBuildSideEffectsInMainWorktree
        $script:OverlayStarted = $false
        [Console]::Error.WriteLine('[spec-audit-loop] failed to stage verified changes; restored main-repo files')
        [Console]::Error.WriteLine($_.Exception.Message)
        return 1
    }

    & $script:GitExe -C $script:RepoRoot diff --cached --quiet
    $diffExit = $LASTEXITCODE
    if ($diffExit -eq 0) {
        Unstage-LauncherIntegrationPaths
        Restore-NonSharedChangesInMainWorktree
        Restore-LauncherBuildSideEffectsInMainWorktree
        $script:OverlayStarted = $false
        [Console]::Error.WriteLine('[spec-audit-loop] no staged changes were prepared for main-repo commit')
        return 1
    }
    if ($diffExit -ne 1) {
        throw 'git diff --cached --quiet failed while preparing main-repo commit'
    }

    Update-ClaimLedgerStage 'committing' 'creating verified main-repo commit'
    $commitResult = Invoke-GitCapture @('-C', $script:RepoRoot, 'commit', '-F', $script:CommitMessageFile) -AllowFailure
    if ($commitResult.ExitCode -ne 0) {
        Unstage-LauncherIntegrationPaths
        Restore-NonSharedChangesInMainWorktree
        Restore-LauncherBuildSideEffectsInMainWorktree
        $script:OverlayStarted = $false
        [Console]::Error.WriteLine('[spec-audit-loop] failed to create verified main-repo commit; restored main-repo files')
        return 1
    }
    $script:OverlayStarted = $false
    return 0
}

function Destroy-CompletedWorktree {
    $null = Invoke-GitCapture @('-C', $script:RepoRoot, 'worktree', 'remove', '--force', $script:WorktreeDir)
    $null = Invoke-GitCapture @('-C', $script:RepoRoot, 'branch', '-D', $script:WorktreeBranch)
    if (-not [string]::IsNullOrWhiteSpace($script:ClaimLedgerPath) -and (Test-Path -LiteralPath $script:ClaimLedgerPath)) {
        Remove-Item -LiteralPath $script:ClaimLedgerPath -Force
    }
    $script:ClaimLedgerPath = ''
}

function Cleanup-InterruptedState {
    if ([string]::IsNullOrWhiteSpace($script:ClaimLedgerPath)) {
        return
    }
    if (-not (Test-Path -LiteralPath $script:ClaimLedgerPath)) {
        return
    }

    $rowLine = if ($null -ne $script:SelectedRow -and $script:SelectedRow.PSObject.Properties.Name -contains '_line') {
        [string]$script:SelectedRow._line
    } else {
        ''
    }

    if ($script:OverlayStarted) {
        Restore-NonSharedChangesInMainWorktree
        Restore-LauncherBuildSideEffectsInMainWorktree
        Update-ClaimLedgerStage 'interrupted_after_overlay' 'main checkout overlay restored after interrupt'
        [Console]::Error.WriteLine('[spec-audit-loop] restored main-checkout overlay after interrupt')
    } else {
        Update-ClaimLedgerStage 'interrupted_before_overlay' 'worktree preserved after interrupt'
    }

    if (-not [string]::IsNullOrWhiteSpace($rowLine)) {
        [Console]::Error.WriteLine("[spec-audit-loop] claimed row remains in_progress: line $rowLine")
        [Console]::Error.WriteLine("[spec-audit-loop] recovery command: .\scripts\spec-audit-loop.ps1 --recover-stale-in-progress --row-line $rowLine")
    }
}

# MARKER: MAIN
try {
    Set-Location $script:RepoRoot
    if ($script:RecoverStaleInProgress) {
        Recover-StaleInProgressRow
        exit 0
    }

    $maxIterationsValue = [int]$script:MaxIterations
    $iteration = 1

    while ($true) {
        if ($maxIterationsValue -gt 0 -and $iteration -gt $maxIterationsValue) {
            Write-Log "reached max iterations ($maxIterationsValue); stopping"
            exit 0
        }

        $claim = Claim-NextRow
        if ($claim.ExitCode -eq 10) {
            Write-Log 'no actionable unresolved audit row remains after skipping complete, ambiguous, and in_progress rows'
            exit 0
        }
        if ($claim.ExitCode -eq 11) {
            [Console]::Error.WriteLine('[spec-audit-loop] remaining unresolved audit work is already in_progress; use --recover-stale-in-progress --row-line <N> for an abandoned claim')
            exit 11
        }
        if ($claim.ExitCode -ne 0) {
            [Console]::Error.WriteLine('[spec-audit-loop] failed to claim the next audit row')
            exit $claim.ExitCode
        }

        $script:SelectedRowJson = (($claim.Output | Select-Object -Last 1) -join '').Trim()
        $script:SelectedRow = $script:SelectedRowJson | ConvertFrom-Json
        $script:SelectedRowRawIndex = [int]$script:SelectedRow._raw_index
        $selectedRowLine = [int]$script:SelectedRow._line

        $script:WorktreeBranch = [string]$script:SelectedRow._worktree_branch
        $script:WorktreeDir = [string]$script:SelectedRow._worktree_dir
        $script:ClaimLedgerPath = [string]$script:SelectedRow._ledger_path
        New-Item -ItemType Directory -Path $script:WorktreeRoot -Force | Out-Null

        $null = Invoke-GitCapture @($script:GitCheckoutHookArgs + @('-C', $script:RepoRoot, 'worktree', 'add', '-b', $script:WorktreeBranch, $script:WorktreeDir, 'HEAD'))
        $script:WorktreeBaseCommit = ((Invoke-GitCapture @('-C', $script:WorktreeDir, 'rev-parse', 'HEAD')).Output | Select-Object -Last 1).Trim()
        Update-ClaimLedgerStage 'worktree_created' 'created assigned worktree'
        $script:WorktreeSessionId = ''
        $script:WorktreeCommit = ''
        $script:VerifiedCompilerPath = ''
        $script:OverlayStarted = $false
        $lastFailedWorktreeCommit = ''

        $script:LastMessageFile = New-TempFile 'spec-audit-last-message-' '.txt'
        $script:IterationPromptFile = New-TempFile 'spec-audit-prompt-' '.md'
        $script:BaseDecisionsSnapshotFile = New-TempFile 'spec-audit-decisions-base-' '.md'
        $script:HeadAuditCsvFile = New-TempFile 'spec-audit-head-csv-' '.csv'
        $script:StagedAuditCsvFile = New-TempFile 'spec-audit-staged-csv-' '.csv'
        $script:HeadSpecDecisionsFile = New-TempFile 'spec-audit-head-decisions-' '.md'
        $script:StagedSpecDecisionsFile = New-TempFile 'spec-audit-staged-decisions-' '.md'
        $script:CommitMessageFile = New-TempFile 'spec-audit-commit-message-' '.txt'
        $script:AgentEventsFile = New-TempFile 'spec-audit-events-' '.jsonl'
        $script:BuildFailurePromptFile = New-TempFile 'spec-audit-build-failure-prompt-' '.md'
        $script:MainBuildLogFile = New-TempFile 'spec-audit-main-build-log-' '.txt'

        Copy-SharedContextToWorktree $script:WorktreeDir
        Build-IterationPrompt

        Write-Log "iteration $iteration"
        $script:SelectedItem = "$($script:SelectedRow.'rule name') @ $($script:SelectedRow.'spec location')"
        Write-Log "selected: $($script:SelectedItem)"
        Write-Log "worktree: $($script:WorktreeDir)"

        $agentMode = 'initial'
        $agentPromptPath = $script:IterationPromptFile

        while ($true) {
            if (-not (Run-AgentTurn $agentMode $agentPromptPath)) {
                [Console]::Error.WriteLine("[spec-audit-loop] Codex $agentMode failed; preserving worktree for inspection: $($script:WorktreeDir)")
                exit 1
            }

            $statusLine = Get-LastMatchingLine $script:LastMessageFile '^SPEC_AUDIT_STATUS: '
            $itemLine = Get-LastMatchingLine $script:LastMessageFile '^SPEC_AUDIT_ITEM: '
            $noteLine = Get-LastMatchingLine $script:LastMessageFile '^SPEC_AUDIT_NOTE: '

            if (-not $statusLine) {
                $recovered = Recover-StatusFooterFromWorktree
                if ($null -ne $recovered -and $recovered.Count -gt 0) {
                    Add-Content -LiteralPath $script:LastMessageFile -Value $recovered -Encoding utf8
                    $statusLine = Get-LastMatchingLine $script:LastMessageFile '^SPEC_AUDIT_STATUS: '
                    $itemLine = Get-LastMatchingLine $script:LastMessageFile '^SPEC_AUDIT_ITEM: '
                    $noteLine = Get-LastMatchingLine $script:LastMessageFile '^SPEC_AUDIT_NOTE: '
                    Write-Log 'recovered missing worker footer from committed worktree state'
                } else {
                    [Console]::Error.WriteLine("[spec-audit-loop] missing SPEC_AUDIT_STATUS footer in $($script:LastMessageFile)")
                    [Console]::Error.WriteLine("[spec-audit-loop] preserving worktree for inspection: $($script:WorktreeDir)")
                    exit 1
                }
            }

            $status = $statusLine -replace '^SPEC_AUDIT_STATUS: ', ''
            $item = $itemLine -replace '^SPEC_AUDIT_ITEM: ', ''
            $note = $noteLine -replace '^SPEC_AUDIT_NOTE: ', ''

            if ($itemLine) { Write-Log "item: $item" }
            if ($noteLine) { Write-Log "note: $note" }

            switch ($status) {
                'continue' {
                    $currentWorktreeHead = ((Invoke-GitCapture @('-C', $script:WorktreeDir, 'rev-parse', 'HEAD')).Output | Select-Object -Last 1).Trim()
                    if ($lastFailedWorktreeCommit -and $currentWorktreeHead -eq $lastFailedWorktreeCommit) {
                        [Console]::Error.WriteLine("[spec-audit-loop] launcher-side verification already failed for worktree commit $currentWorktreeHead and no follow-up commit was created")
                        [Console]::Error.WriteLine("[spec-audit-loop] preserving worktree for inspection: $($script:WorktreeDir)")
                        exit 1
                    }

                    $integrateRc = Integrate-VerifiedCommitIntoMainRepo
                    if ($integrateRc -eq 2) {
                        $lastFailedWorktreeCommit = $currentWorktreeHead
                        Build-FailureRetryPrompt
                        $agentMode = 'resume'
                        $agentPromptPath = $script:BuildFailurePromptFile
                        Write-Log 'resuming same worktree agent with launcher-side build failure output'
                        continue
                    }
                    if ($integrateRc -ne 0) {
                        [Console]::Error.WriteLine("[spec-audit-loop] failed to integrate verified worktree commit back into the main repo; preserving worktree: $($script:WorktreeDir)")
                        exit 1
                    }

                    Destroy-CompletedWorktree
                    Write-Log 'integrated verified worktree changes into main repo and destroyed worktree'

                    if ($script:Once) {
                        Write-Log 'stopping after one iteration (--once)'
                        exit 0
                    }
                    break
                }
                'complete' {
                    $currentWorktreeHead = ((Invoke-GitCapture @('-C', $script:WorktreeDir, 'rev-parse', 'HEAD')).Output | Select-Object -Last 1).Trim()
                    if ($lastFailedWorktreeCommit -and $currentWorktreeHead -eq $lastFailedWorktreeCommit) {
                        [Console]::Error.WriteLine("[spec-audit-loop] launcher-side verification already failed for worktree commit $currentWorktreeHead and no follow-up commit was created")
                        [Console]::Error.WriteLine("[spec-audit-loop] preserving worktree for inspection: $($script:WorktreeDir)")
                        exit 1
                    }

                    $integrateRc = Integrate-VerifiedCommitIntoMainRepo
                    if ($integrateRc -eq 2) {
                        $lastFailedWorktreeCommit = $currentWorktreeHead
                        Build-FailureRetryPrompt
                        $agentMode = 'resume'
                        $agentPromptPath = $script:BuildFailurePromptFile
                        Write-Log 'resuming same worktree agent with launcher-side build failure output'
                        continue
                    }
                    if ($integrateRc -ne 0) {
                        [Console]::Error.WriteLine("[spec-audit-loop] failed to integrate verified worktree commit back into the main repo; preserving worktree: $($script:WorktreeDir)")
                        exit 1
                    }

                    Destroy-CompletedWorktree
                    Write-Log 'integrated verified worktree changes into main repo and destroyed worktree'
                    Write-Log 'complete'
                    exit 0
                }
                'blocked' {
                    Update-ClaimLedgerStage 'blocked' $note
                    [Console]::Error.WriteLine('[spec-audit-loop] blocked')
                    [Console]::Error.WriteLine("[spec-audit-loop] preserving blocked worktree for inspection: $($script:WorktreeDir)")
                    exit 2
                }
                default {
                    Update-ClaimLedgerStage 'unknown_worker_status' $status
                    [Console]::Error.WriteLine("[spec-audit-loop] unknown SPEC_AUDIT_STATUS value: $status")
                    [Console]::Error.WriteLine("[spec-audit-loop] preserving worktree for inspection: $($script:WorktreeDir)")
                    exit 1
                }
            }

            break
        }

        Reset-TempState
        $iteration += 1
    }
} finally {
    if (-not [string]::IsNullOrWhiteSpace($script:ClaimLedgerPath) -and (Test-Path -LiteralPath $script:ClaimLedgerPath)) {
        Cleanup-InterruptedState
    }
    Cleanup-TempFiles
}
