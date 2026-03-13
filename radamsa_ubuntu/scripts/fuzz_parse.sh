#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
source "$SCRIPT_DIR/common.sh"

prepare_env
cd "$ROOT_DIR"

SEED="$ROOT_DIR/corpus/valid_fw.bin"
if [[ ! -f "$SEED" ]]; then
  echo "Missing seed file: $SEED" >&2
  exit 1
fi

echo "[parse] iterations=$ITERATIONS timeout=${TIMEOUT_SECONDS}s"

for ((i=1; i<=ITERATIONS; i++)); do
  mut_file="$(mktemp)"
  log_file="$(mktemp)"

  radamsa "$SEED" > "$mut_file"

  set +e
  timeout "$TIMEOUT_SECONDS" ./matryoshka parse < "$mut_file" > "$log_file" 2>&1
  rc=$?
  set -e

  if [[ "$rc" -eq 124 ]]; then
    save_interesting "parse" "hangs" "$i" "$mut_file" "$log_file"
    echo "[parse] hang at iteration $i"
  elif is_sanitizer_or_fatal "$log_file" "$rc"; then
    save_interesting "parse" "crashes" "$i" "$mut_file" "$log_file"
    echo "[parse] sanitizer/fatal finding at iteration $i"
  fi

  rm -f "$mut_file" "$log_file"
done

echo "[parse] done"
