#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

bash "$SCRIPT_DIR/fuzz_parse.sh"
bash "$SCRIPT_DIR/fuzz_protocol_text.sh"
bash "$SCRIPT_DIR/fuzz_chunk.sh"
bash "$SCRIPT_DIR/fuzz_header.sh"
bash "$SCRIPT_DIR/fuzz_protocol_pipeline.sh"

echo "[all] all fuzz campaigns completed"
