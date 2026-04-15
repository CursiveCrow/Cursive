#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

DOWNLOAD_DIR="$REPO_ROOT/extern/.downloads"
ICU_ARCHIVE="$DOWNLOAD_DIR/icu4c-72_1-src.tgz"
ICU_SOURCE_URL="${CURSIVE_ICU_SOURCE_URL:-https://github.com/unicode-org/icu/releases/download/release-72-1/icu4c-72_1-src.tgz}"
ICU_RUNTIME_DEB_URL="${CURSIVE_ICU_RUNTIME_DEB_URL:-https://deb.debian.org/debian/pool/main/i/icu/libicu72_72.1-3+deb12u1_amd64.deb}"
ICU_DEV_DEB_URL="${CURSIVE_ICU_DEV_DEB_URL:-https://deb.debian.org/debian/pool/main/i/icu/libicu-dev_72.1-3+deb12u1_amd64.deb}"
ICU_RUNTIME_DEB="$DOWNLOAD_DIR/$(basename "$ICU_RUNTIME_DEB_URL")"
ICU_DEV_DEB="$DOWNLOAD_DIR/$(basename "$ICU_DEV_DEB_URL")"
ICU_DEST_ROOT="$REPO_ROOT/extern/icu/linux"
ICU_DEST_INCLUDE="$ICU_DEST_ROOT/include"
ICU_DEST_LIB="$ICU_DEST_ROOT/lib"
ICU_BUILD_ROOT="${TMPDIR:-/tmp}/cursive-icu72-bootstrap"
ICU_SOURCE_ROOT="$ICU_BUILD_ROOT/src"
ICU_SOURCE_DIR="$ICU_SOURCE_ROOT/icu/source"
ICU_RUNTIME_ROOT="$ICU_BUILD_ROOT/runtime"
ICU_DEV_ROOT="$ICU_BUILD_ROOT/dev"
ICU_RUNTIME_LIB="$ICU_RUNTIME_ROOT/usr/lib/x86_64-linux-gnu"
ICU_DEV_INCLUDE="$ICU_DEV_ROOT/usr/include"
ICU_DATA_FILE="$ICU_SOURCE_DIR/data/in/icudt72l.dat"

download_file() {
  local url="$1"
  local out_file="$2"
  local tmp_file

  mkdir -p "$(dirname "$out_file")"
  tmp_file="${out_file}.partial"
  rm -f "$tmp_file"
  curl -fL "$url" -o "$tmp_file"
  mv "$tmp_file" "$out_file"
}

require_cmd() {
  local name="$1"
  command -v "$name" >/dev/null 2>&1 || {
    printf 'missing required command: %s\n' "$name" >&2
    exit 1
  }
}

require_cmd curl
require_cmd tar
require_cmd dpkg-deb

rm -rf "$ICU_BUILD_ROOT"
mkdir -p "$DOWNLOAD_DIR" "$ICU_BUILD_ROOT" "$ICU_SOURCE_ROOT" "$ICU_RUNTIME_ROOT" "$ICU_DEV_ROOT" "$ICU_DEST_INCLUDE" "$ICU_DEST_LIB"

if [[ ! -f "$ICU_ARCHIVE" ]]; then
  download_file "$ICU_SOURCE_URL" "$ICU_ARCHIVE"
fi

if [[ ! -f "$ICU_RUNTIME_DEB" ]]; then
  download_file "$ICU_RUNTIME_DEB_URL" "$ICU_RUNTIME_DEB"
fi

if [[ ! -f "$ICU_DEV_DEB" ]]; then
  download_file "$ICU_DEV_DEB_URL" "$ICU_DEV_DEB"
fi

tar xfz "$ICU_ARCHIVE" -C "$ICU_SOURCE_ROOT"
dpkg-deb -x "$ICU_RUNTIME_DEB" "$ICU_RUNTIME_ROOT"
dpkg-deb -x "$ICU_DEV_DEB" "$ICU_DEV_ROOT"

rm -rf "$ICU_DEST_INCLUDE"
mkdir -p "$ICU_DEST_INCLUDE" "$ICU_DEST_LIB"
cp -R "$ICU_DEV_INCLUDE/." "$ICU_DEST_INCLUDE/"
cp -f "$ICU_RUNTIME_LIB/libicui18n.so.72.1" "$ICU_DEST_LIB/libicui18n.so.72"
cp -f "$ICU_RUNTIME_LIB/libicuuc.so.72.1" "$ICU_DEST_LIB/libicuuc.so.72"
cp -f "$ICU_RUNTIME_LIB/libicudata.so.72.1" "$ICU_DEST_LIB/libicudata.so.72"
cp -f "$ICU_DATA_FILE" "$ICU_DEST_LIB/icudt72l.dat"

printf 'bootstrapped prebuilt Linux ICU payload under %s\n' "$ICU_DEST_ROOT"
