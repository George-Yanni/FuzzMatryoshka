#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PACK_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
ROOT_DIR="$(cd "$PACK_DIR/.." && pwd)"

ITERATIONS="${ITERATIONS:-20000}"
TIMEOUT_SECONDS="${TIMEOUT_SECONDS:-2}"

require_cmd() {
  local name="$1"
  if ! command -v "$name" >/dev/null 2>&1; then
    echo "Missing required command: $name" >&2
    exit 1
  fi
}

prepare_env() {
  require_cmd radamsa
  require_cmd timeout
  if [[ ! -x "$ROOT_DIR/matryoshka" ]]; then
    echo "Missing executable: $ROOT_DIR/matryoshka" >&2
    echo "Build first from repo root (example): make" >&2
    exit 1
  fi
}

is_sanitizer_or_fatal() {
  local log_file="$1"
  local rc="$2"

  if [[ "$rc" -ge 128 ]]; then
    return 0
  fi

  if grep -Eq "AddressSanitizer|UndefinedBehaviorSanitizer|runtime error:|ERROR: AddressSanitizer" "$log_file"; then
    return 0
  fi

  return 1
}

save_interesting() {
  local campaign="$1"
  local kind="$2"
  local iter="$3"
  local input_file="$4"
  local log_file="$5"

  local out_dir="$PACK_DIR/out/$campaign/$kind"
  mkdir -p "$out_dir"

  cp "$input_file" "$out_dir/input_$iter.bin"
  cp "$log_file" "$out_dir/log_$iter.txt"
}
