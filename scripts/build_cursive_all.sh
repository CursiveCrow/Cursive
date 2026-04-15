#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
SOURCE_DIR="$REPO_ROOT/cursive"

LINUX_PRESET="${CURSIVE_LINUX_PRESET:-linux-debug}"
WINDOWS_PRESET="${CURSIVE_WINDOWS_PRESET:-windows-debug}"
LINUX_TARGET="${CURSIVE_LINUX_TARGET:-cursive_out}"
WINDOWS_TARGET="${CURSIVE_WINDOWS_TARGET:-cursive_out}"

BUILD_LINUX=1
BUILD_WINDOWS=1
CONFIGURE_ONLY=0
SETUP_LINUX_EXTERN=1
SETUP_WINDOWS_EXTERN=1
JOBS="${CURSIVE_BUILD_JOBS:-}"

log() {
  printf '[build-cursive-all] %s\n' "$*"
}

fail() {
  printf '[build-cursive-all] error: %s\n' "$*" >&2
  exit 1
}

usage() {
  cat <<'EOF'
Usage: scripts/build_cursive_all.sh [options]

Build the Linux and Windows Cursive compiler packages using the existing
CMake presets and the cursive_out staging target.

Options:
  --platform NAME          Build target selection: linux, windows, or all (default: all)
  --linux-only              Build only the Linux package
  --windows-only            Build only the Windows package
  --skip-linux              Skip the Linux package
  --skip-windows            Skip the Windows package
  --configure-only          Run CMake configure only; skip the build step
  --linux-preset NAME       Override the Linux configure/build preset
  --windows-preset NAME     Override the Windows configure/build preset
  --linux-target NAME       Override the Linux build target (default: cursive_out)
  --windows-target NAME     Override the Windows build target (default: cursive_out)
  --jobs N                  Pass an explicit parallelism level to cmake --build
  --no-setup-linux-extern   Do not auto-run scripts/setup_extern_linux.sh
  --no-setup-windows-extern Do not auto-run scripts/setup_extern.ps1
  -h, --help                Show this help text

Environment overrides:
  CURSIVE_LINUX_PRESET
  CURSIVE_WINDOWS_PRESET
  CURSIVE_LINUX_TARGET
  CURSIVE_WINDOWS_TARGET
  CURSIVE_BUILD_JOBS
EOF
}

apply_platform_selection() {
  case "$1" in
    linux)
      BUILD_LINUX=1
      BUILD_WINDOWS=0
      ;;
    windows)
      BUILD_LINUX=0
      BUILD_WINDOWS=1
      ;;
    all)
      BUILD_LINUX=1
      BUILD_WINDOWS=1
      ;;
    *)
      fail "invalid --platform value: $1 (expected linux, windows, or all)"
      ;;
  esac
}

require_cmd() {
  local name="$1"
  command -v "$name" >/dev/null 2>&1 || fail "required command not found: $name"
}

escape_powershell_literal() {
  local value="$1"
  printf '%s' "${value//\'/\'\'}"
}

to_windows_path() {
  local input="$1"
  if command -v wslpath >/dev/null 2>&1; then
    wslpath -w "$input"
    return
  fi
  if command -v cygpath >/dev/null 2>&1; then
    cygpath -w "$input"
    return
  fi
  fail "no Windows path conversion tool found (expected wslpath or cygpath)"
}

run_windows_powershell() {
  local script="$1"
  require_cmd powershell.exe
  powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "$script"
}

ensure_linux_extern() {
  local required=(
    "$REPO_ROOT/extern/tomlplusplus/include/toml++/toml.hpp"
    "$REPO_ROOT/extern/icu/linux/include"
    "$REPO_ROOT/extern/icu/linux/lib/libicui18n.so.72"
    "$REPO_ROOT/extern/icu/linux/lib/libicuuc.so.72"
    "$REPO_ROOT/extern/icu/linux/lib/libicudata.so.72"
    "$REPO_ROOT/extern/icu/linux/lib/icudt72l.dat"
    "$REPO_ROOT/extern/llvm/llvm-21.1.8-x86_64-sysv/bin/clang++"
    "$REPO_ROOT/extern/llvm/llvm-21.1.8-x86_64-sysv/bin/ld.lld"
    "$REPO_ROOT/extern/llvm/llvm-21.1.8-x86_64-sysv/bin/llvm-ar"
    "$REPO_ROOT/extern/llvm/llvm-21.1.8-x86_64-sysv/bin/llvm-as"
    "$REPO_ROOT/extern/llvm/llvm-21.1.8-x86_64-sysv/include"
    "$REPO_ROOT/extern/llvm/llvm-21.1.8-x86_64-sysv/lib/cmake/llvm"
  )

  local missing=0
  local path
  for path in "${required[@]}"; do
    if [[ ! -e "$path" ]]; then
      missing=1
      break
    fi
  done

  if (( missing == 1 )); then
    if (( SETUP_LINUX_EXTERN == 0 )); then
      fail "Linux extern payload is incomplete under extern/"
    fi
    log "Linux extern payload missing; running scripts/setup_extern_linux.sh"
    bash "$SCRIPT_DIR/setup_extern_linux.sh"
  fi

  for path in "${required[@]}"; do
    [[ -e "$path" ]] || fail "Linux prerequisite still missing after setup: $path"
  done
}

ensure_windows_extern() {
  local required=(
    "$REPO_ROOT/extern/tomlplusplus/include/toml++/toml.hpp"
    "$REPO_ROOT/extern/icu/win64/include"
    "$REPO_ROOT/extern/icu/win64/lib64/icuuc.lib"
    "$REPO_ROOT/extern/icu/win64/lib64/icuin.lib"
    "$REPO_ROOT/extern/icu/win64/lib64/icudt.lib"
    "$REPO_ROOT/extern/icu/win64/bin64/icuuc72.dll"
    "$REPO_ROOT/extern/icu/win64/bin64/icuin72.dll"
    "$REPO_ROOT/extern/icu/win64/bin64/icudt72.dll"
    "$REPO_ROOT/extern/llvm/llvm-21.1.8-x86_64/bin/lld-link.exe"
    "$REPO_ROOT/extern/llvm/llvm-21.1.8-x86_64/bin/llvm-ar.exe"
    "$REPO_ROOT/extern/llvm/llvm-21.1.8-x86_64/bin/llvm-lib.exe"
    "$REPO_ROOT/extern/llvm/llvm-21.1.8-x86_64/bin/llvm-as.exe"
    "$REPO_ROOT/extern/llvm/llvm-21.1.8-x86_64/include"
    "$REPO_ROOT/extern/llvm/llvm-21.1.8-x86_64/lib/cmake/llvm"
  )

  local missing=0
  local path
  for path in "${required[@]}"; do
    if [[ ! -e "$path" ]]; then
      missing=1
      break
    fi
  done

  if (( missing == 1 )); then
    if (( SETUP_WINDOWS_EXTERN == 0 )); then
      fail "Windows extern payload is incomplete under extern/"
    fi

    local setup_script_windows
    setup_script_windows="$(to_windows_path "$REPO_ROOT/scripts/setup_extern.ps1")"

    log "Windows extern payload missing; running scripts/setup_extern.ps1"
    run_windows_powershell "& '$(escape_powershell_literal "$setup_script_windows")' -RepoRoot '$(escape_powershell_literal "$(to_windows_path "$REPO_ROOT")")'"
  fi

  for path in "${required[@]}"; do
    [[ -e "$path" ]] || fail "Windows prerequisite still missing after setup: $path"
  done
}

run_linux_build() {
  require_cmd cmake
  ensure_linux_extern

  log "Configuring Linux build with preset: $LINUX_PRESET"
  (
    cd "$SOURCE_DIR"
    cmake --preset "$LINUX_PRESET"
  )

  if (( CONFIGURE_ONLY == 1 )); then
    return
  fi

  log "Building Linux target: $LINUX_TARGET"
  if [[ -n "$JOBS" ]]; then
    (
      cd "$SOURCE_DIR"
      cmake --build --preset "$LINUX_PRESET" --target "$LINUX_TARGET" --parallel "$JOBS"
    )
  else
    (
      cd "$SOURCE_DIR"
      cmake --build --preset "$LINUX_PRESET" --target "$LINUX_TARGET"
    )
  fi
}

run_windows_build() {
  ensure_windows_extern

  local source_dir_windows
  source_dir_windows="$(to_windows_path "$SOURCE_DIR")"

  local windows_script
  windows_script="\$ErrorActionPreference = 'Stop'
\$cmake = Get-Command cmake.exe -ErrorAction SilentlyContinue
if (-not \$cmake) { \$cmake = Get-Command cmake -ErrorAction SilentlyContinue }
if (-not \$cmake) { throw 'cmake was not found in the Windows PATH.' }
Set-Location '$(escape_powershell_literal "$source_dir_windows")'
& \$cmake.Source --preset '$(escape_powershell_literal "$WINDOWS_PRESET")'"

  if (( CONFIGURE_ONLY == 0 )); then
    if [[ -n "$JOBS" ]]; then
      windows_script="${windows_script}
& \$cmake.Source --build --preset '$(escape_powershell_literal "$WINDOWS_PRESET")' --target '$(escape_powershell_literal "$WINDOWS_TARGET")' --parallel '$(escape_powershell_literal "$JOBS")' -- /p:BuildProjectReferences=true"
    else
      windows_script="${windows_script}
& \$cmake.Source --build --preset '$(escape_powershell_literal "$WINDOWS_PRESET")' --target '$(escape_powershell_literal "$WINDOWS_TARGET")' -- /p:BuildProjectReferences=true"
    fi
  fi

  log "Running Windows build with preset: $WINDOWS_PRESET"
  run_windows_powershell "$windows_script"
}

while (($# > 0)); do
  case "$1" in
    --platform)
      shift
      [[ $# -gt 0 ]] || fail "--platform requires a value"
      apply_platform_selection "$1"
      ;;
    --linux-only)
      apply_platform_selection linux
      ;;
    --windows-only)
      apply_platform_selection windows
      ;;
    --skip-linux)
      BUILD_LINUX=0
      ;;
    --skip-windows)
      BUILD_WINDOWS=0
      ;;
    --configure-only)
      CONFIGURE_ONLY=1
      ;;
    --linux-preset)
      shift
      [[ $# -gt 0 ]] || fail "--linux-preset requires a value"
      LINUX_PRESET="$1"
      ;;
    --windows-preset)
      shift
      [[ $# -gt 0 ]] || fail "--windows-preset requires a value"
      WINDOWS_PRESET="$1"
      ;;
    --linux-target)
      shift
      [[ $# -gt 0 ]] || fail "--linux-target requires a value"
      LINUX_TARGET="$1"
      ;;
    --windows-target)
      shift
      [[ $# -gt 0 ]] || fail "--windows-target requires a value"
      WINDOWS_TARGET="$1"
      ;;
    --jobs)
      shift
      [[ $# -gt 0 ]] || fail "--jobs requires a value"
      JOBS="$1"
      ;;
    --no-setup-linux-extern|--no-bootstrap-linux-icu)
      SETUP_LINUX_EXTERN=0
      ;;
    --no-setup-windows-extern)
      SETUP_WINDOWS_EXTERN=0
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      fail "unknown argument: $1"
      ;;
  esac
  shift
done

(( BUILD_LINUX == 1 || BUILD_WINDOWS == 1 )) || fail "nothing to do; both builds are disabled"

if (( BUILD_LINUX == 1 )); then
  run_linux_build
fi

if (( BUILD_WINDOWS == 1 )); then
  run_windows_build
fi

log "Completed."
if (( BUILD_LINUX == 1 )); then
  log "Linux package: $SOURCE_DIR/build/linux/out"
fi
if (( BUILD_WINDOWS == 1 )); then
  log "Windows package: $SOURCE_DIR/build/windows/out"
fi
