#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
EXTERN_DIR="${EXTERN_DIR:-$REPO_ROOT/extern}"
DOWNLOAD_DIR="$EXTERN_DIR/.downloads"

LLVM_TAG="${CURSIVE_LLVM_TAG:-llvmorg-21.1.8}"
LLVM_TARGET_DIR="${EXTERN_DIR}/llvm/llvm-21.1.8-x86_64-sysv"
LLVM_ASSET_REGEX="${CURSIVE_LINUX_LLVM_ASSET_REGEX:-^LLVM-21\\.1\\.8-Linux-X64\\.tar\\.xz$}"
TOML_TAG="${CURSIVE_TOML_TAG:-v3.4.0}"
ICU_TAG="${CURSIVE_ICU_TAG:-release-72-1}"
ICU_ARCHIVE="${DOWNLOAD_DIR}/icu4c-72_1-src.tgz"
ICU_READY_MARKER="${EXTERN_DIR}/icu/linux/lib/libicuuc.so.72"

FORCE=0
NO_CACHE=0

usage() {
  cat <<'EOF'
Usage: setup_extern_linux.sh [--force] [--no-cache]

Downloads and stages the Linux extern dependencies required to build Cursive:
- pinned LLVM headers, CMake config, required LLVM libraries, and required tool binaries
- toml++
- ICU source archive, followed by bootstrap_linux_icu.sh
EOF
}

log() {
  printf '[extern-linux] %s\n' "$*"
}

fail() {
  printf '[extern-linux] %s\n' "$*" >&2
  exit 1
}

download_headers=(
  -H "Accept: application/vnd.github+json"
  -H "User-Agent: Cursive-extern-setup-linux"
)

if [[ -n "${GITHUB_TOKEN:-}" ]]; then
  download_headers+=(-H "Authorization: Bearer ${GITHUB_TOKEN}")
fi

download_file() {
  local url="$1"
  local out_file="$2"
  local tmp_file

  mkdir -p "$(dirname "$out_file")"
  if [[ -f "$out_file" && "$NO_CACHE" -eq 0 ]]; then
    log "Using cached download: $out_file"
    return 0
  fi

  tmp_file="${out_file}.partial"
  rm -f "$out_file" "$tmp_file"
  log "Downloading: $url"
  curl -fL "${download_headers[@]}" "$url" -o "$tmp_file"
  mv "$tmp_file" "$out_file"
}

extract_zip() {
  local archive="$1"
  local destination="$2"
  python3 - "$archive" "$destination" <<'PY'
import pathlib
import shutil
import sys
import zipfile

archive = pathlib.Path(sys.argv[1])
destination = pathlib.Path(sys.argv[2])
if destination.exists():
    shutil.rmtree(destination)
destination.mkdir(parents=True, exist_ok=True)
with zipfile.ZipFile(archive) as zf:
    zf.extractall(destination)
PY
}

copy_tree_contents() {
  local source_dir="$1"
  local target_dir="$2"

  rm -rf "$target_dir"
  mkdir -p "$target_dir"
  cp -R "$source_dir"/. "$target_dir"/
}

copy_resolved_tool() {
  local source_file="$1"
  local target_file="$2"
  local resolved

  [[ -e "$source_file" ]] || fail "Missing LLVM tool: $source_file"
  mkdir -p "$(dirname "$target_file")"
  resolved="$(readlink -f "$source_file" 2>/dev/null || printf '%s' "$source_file")"
  install -m 0755 "$resolved" "$target_file"
}

stage_llvm_subset() {
  local source_root="$1"
  local target_root="$2"
  local target_include_root target_lib_root

  rm -rf "$target_root"
  mkdir -p "$target_root/bin" "$target_root/include" "$target_root/lib/cmake"

  [[ -d "$source_root/include/llvm" ]] || fail "Missing LLVM headers under $source_root/include/llvm"
  [[ -d "$source_root/include/llvm-c" ]] || fail "Missing LLVM C headers under $source_root/include/llvm-c"
  [[ -d "$source_root/lib" ]] || fail "Missing LLVM lib directory under $source_root"
  [[ -d "$source_root/lib/cmake/llvm" ]] || fail "Missing LLVM CMake package under $source_root/lib/cmake/llvm"

  target_include_root="$target_root/include"
  target_lib_root="$target_root/lib"

  copy_tree_contents "$source_root/include/llvm" "$target_include_root/llvm"
  copy_tree_contents "$source_root/include/llvm-c" "$target_include_root/llvm-c"
  copy_tree_contents "$source_root/lib/cmake/llvm" "$target_lib_root/cmake/llvm"

  find "$source_root/lib" -maxdepth 1 -type f \
    \( -name 'libLLVM*.a' -o -name 'libLLVM*.so' -o -name 'libLLVM*.so.*' \) \
    -print0 | while IFS= read -r -d '' lib_file; do
      install -m 0644 "$lib_file" "$target_lib_root/$(basename "$lib_file")"
    done

  [[ -f "$target_lib_root/libLLVMCore.a" || -f "$target_lib_root/libLLVMCore.so" || -f "$target_lib_root/libLLVMCore.so.21.1" || -f "$target_lib_root/libLLVMCore.so.21.1.8" ]] \
    || fail "Failed to stage LLVM libraries into $target_lib_root"

  copy_resolved_tool "$source_root/bin/ld.lld" "$target_root/bin/ld.lld"
  copy_resolved_tool "$source_root/bin/llvm-ar" "$target_root/bin/llvm-ar"
  copy_resolved_tool "$source_root/bin/llvm-as" "$target_root/bin/llvm-as"
}

resolve_release_asset() {
  local api_url="$1"
  local asset_regex="$2"
  python3 - "$api_url" "$asset_regex" <<'PY'
import json
import re
import sys
import urllib.request

url = sys.argv[1]
pattern = re.compile(sys.argv[2])
request = urllib.request.Request(
    url,
    headers={
        "Accept": "application/vnd.github+json",
        "User-Agent": "Cursive-extern-setup-linux",
        **(
            {"Authorization": f"Bearer {token}"}
            if (token := __import__("os").environ.get("GITHUB_TOKEN"))
            else {}
        ),
    },
)
with urllib.request.urlopen(request) as response:
    data = json.load(response)

for asset in data.get("assets", []):
    name = asset.get("name", "")
    if pattern.search(name):
        print(name)
        print(asset["browser_download_url"])
        sys.exit(0)

raise SystemExit(f"No asset matched {pattern.pattern!r} from {url}")
PY
}

find_cached_asset() {
  local asset_regex="$1"
  python3 - "$DOWNLOAD_DIR" "$asset_regex" <<'PY'
import pathlib
import re
import sys

download_dir = pathlib.Path(sys.argv[1])
pattern = re.compile(sys.argv[2])

if not download_dir.exists():
    raise SystemExit(1)

matches = sorted(
    (path for path in download_dir.iterdir() if path.is_file() and pattern.search(path.name)),
    key=lambda path: path.name,
)
if not matches:
    raise SystemExit(1)

print(matches[0].name)
PY
}

patch_llvm_import_checks() {
  local exports_file="$1"

  [[ -f "$exports_file" ]] || fail "Missing LLVM exports file: $exports_file"

  python3 - "$exports_file" <<'PY'
import pathlib
import sys

path = pathlib.Path(sys.argv[1])
text = path.read_text(encoding="utf-8")
patched = text.replace(
    "# Loop over all imported files and verify that they actually exist\nforeach(_cmake_target IN LISTS _cmake_import_check_targets)\n",
    "# Vendored Cursive extern keeps only the build-consumed LLVM subset.\nset(_cmake_import_check_targets)\n# Loop over all imported files and verify that they actually exist\nforeach(_cmake_target IN LISTS _cmake_import_check_targets)\n",
    1,
)
patched = patched.replace(
    "# Loop over all imported files and verify that they actually exist\nforeach(target ${_IMPORT_CHECK_TARGETS} )\n",
    "# Vendored Cursive extern keeps only the build-consumed LLVM subset.\nset(_IMPORT_CHECK_TARGETS)\n# Loop over all imported files and verify that they actually exist\nforeach(target ${_IMPORT_CHECK_TARGETS} )\n",
    1,
)

if patched != text:
    path.write_text(patched, encoding="utf-8")
PY
}

setup_llvm() {
  local archive_path asset_name asset_url

  if [[ -f "${LLVM_TARGET_DIR}/bin/clang++" &&
        -f "${LLVM_TARGET_DIR}/bin/llvm-ar" &&
        "$FORCE" -eq 0 ]]; then
    log "LLVM already present at $LLVM_TARGET_DIR"
    return 0
  fi

  if [[ "$NO_CACHE" -eq 0 ]] && asset_name="$(find_cached_asset "$LLVM_ASSET_REGEX" 2>/dev/null)"; then
    archive_path="${DOWNLOAD_DIR}/${asset_name}"
    log "Using cached LLVM archive: $archive_path"
  else
    mapfile -t llvm_asset < <(
      resolve_release_asset \
        "https://api.github.com/repos/llvm/llvm-project/releases/tags/${LLVM_TAG}" \
        "$LLVM_ASSET_REGEX"
    )
    asset_name="${llvm_asset[0]}"
    asset_url="${llvm_asset[1]}"
    archive_path="${DOWNLOAD_DIR}/${asset_name}"
    download_file "$asset_url" "$archive_path"
  fi
  python3 "${SCRIPT_DIR}/stage_linux_llvm_from_archive.py" \
    --archive "$archive_path" \
    --target "$LLVM_TARGET_DIR"
  log "LLVM installed at $LLVM_TARGET_DIR"
}

setup_toml() {
  local target_dir archive_path tmp_dir root_dir

  target_dir="${EXTERN_DIR}/tomlplusplus/include"
  if [[ -f "${target_dir}/toml++/toml.hpp" && "$FORCE" -eq 0 ]]; then
    log "toml++ already present at $target_dir"
    return 0
  fi

  archive_path="${DOWNLOAD_DIR}/tomlplusplus-${TOML_TAG}.zip"
  tmp_dir="${EXTERN_DIR}/.tmp_toml"

  download_file "https://api.github.com/repos/marzer/tomlplusplus/zipball/${TOML_TAG}" "$archive_path"
  extract_zip "$archive_path" "$tmp_dir"
  root_dir="$(find "$tmp_dir" -path '*/include/toml++/toml.hpp' -print | head -n 1)"
  [[ -n "$root_dir" ]] || fail "Unable to locate toml++ headers in $tmp_dir"
  root_dir="$(dirname "$(dirname "$root_dir")")"

  copy_tree_contents "$root_dir" "$target_dir"
  rm -rf "$tmp_dir"
  log "toml++ installed at $target_dir"
}

setup_icu_source() {
  if [[ -f "$ICU_ARCHIVE" && "$NO_CACHE" -eq 0 ]]; then
    log "Using cached ICU source archive: $ICU_ARCHIVE"
    return 0
  fi

  download_file \
    "https://github.com/unicode-org/icu/releases/download/${ICU_TAG}/icu4c-72_1-src.tgz" \
    "$ICU_ARCHIVE"
}

bootstrap_icu() {
  if [[ -f "$ICU_READY_MARKER" && "$FORCE" -eq 0 ]]; then
    log "ICU Linux payload already present at ${EXTERN_DIR}/icu/linux"
    return 0
  fi

  "${SCRIPT_DIR}/bootstrap_linux_icu.sh"
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --force)
      FORCE=1
      ;;
    --no-cache)
      NO_CACHE=1
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      fail "Unknown argument: $1"
      ;;
  esac
  shift
done

mkdir -p "$DOWNLOAD_DIR" "${EXTERN_DIR}/icu" "${EXTERN_DIR}/llvm" "${EXTERN_DIR}/tomlplusplus"

setup_llvm
setup_toml
setup_icu_source
bootstrap_icu

log "Completed Linux extern setup."
