# Ubuntu Radamsa Fuzz Pack

This directory is a focused fuzzing workspace for the Matryoshka codebase using Radamsa on Ubuntu.

No installation instructions are included here by design. This pack assumes Radamsa is already available in your Ubuntu environment.

## What This Pack Gives You

- Explicit fuzz target map tied to actual source behavior
- A strategy document prioritizing high-risk attack surfaces
- Bash scripts for repeatable fuzz loops and artifact capture
- Separate output folders for crashes, hangs, and sanitizer findings

## Quick Start

Run from repository root:

```bash
make clean
make CXXFLAGS='-std=c++17 -Wall -Wextra -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc'
make seed
```

Then run the campaign:

```bash
bash radamsa_ubuntu/scripts/fuzz_all.sh
```

By default this runs all target families and writes artifacts under:

- radamsa_ubuntu/out/parse
- radamsa_ubuntu/out/header
- radamsa_ubuntu/out/chunk
- radamsa_ubuntu/out/protocol_text
- radamsa_ubuntu/out/protocol_pipeline

## Tunables

Environment variables:

- ITERATIONS (default: 20000)
- TIMEOUT_SECONDS (default: 2)

Example:

```bash
ITERATIONS=100000 TIMEOUT_SECONDS=3 bash radamsa_ubuntu/scripts/fuzz_all.sh
```

## Recommended Workflow

1. Build with sanitizers.
2. Run fuzz_all.sh for broad coverage.
3. Reproduce with the saved artifact and captured log.
4. Minimize and convert to regression test seeds.

## Notes

- Non-zero exit from parse/header alone is often expected for malformed input.
- Real findings are identified via sanitizer output, fatal signals, or hangs.
- The strategy and target rationale are in TARGETS_AND_STRATEGY.md.
