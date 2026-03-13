# Fuzz Targets and Strategy

## Code-Comprehension Summary

Core binaries and modes:

- matryoshka parse: full firmware parse + digest + rollback check
- matryoshka header: header-only parse path
- matryoshka chunk: standalone CHUNK parser in a static 1 MiB buffer
- matryoshka protocol: line-based state machine (START, CHUNK, COMMIT, ABORT, VERSION)
- updater: emits protocol stream from binary firmware

## High-Value Targets

1. Parser segment table handling in src/fw_parser.cpp

- parse_firmware_image copies segment descriptors using num_segments from the input.
- seg_count is not bounded against FW_MAX_SEGMENTS before writing out.segments[i].
- This is a high-priority memory corruption target when num_segments is large.

1. Protocol CHUNK parsing in src/protocol.cpp

- cmd_chunk parses offset and hex pairs and writes into a vector sized by START.
- Risk classes: malformed offsets, odd/invalid hex, trailing garbage, boundary behavior.

1. Standalone CHUNK mode in src/protocol.cpp

- process_single_chunk_line writes into a fixed-size static buffer.
- Good target for malformed command text and large offsets.

1. START size handling in src/protocol.cpp

- START drives dynamic allocation of buffer_.
- Good for memory pressure and parser state stress.

1. Updater to protocol pipeline

- Mutate binary image, transform to protocol stream using updater, then feed protocol parser.
- This checks integration semantics and parser assumptions under transformed input.

## Strategy

1. Build with ASan + UBSan.

- This makes silent memory bugs observable.

1. Split campaigns by input grammar.

- Binary campaign: mutate corpus/valid_fw.bin for parse/header.
- Text campaign: mutate valid protocol and CHUNK seed lines.
- Pipeline campaign: mutate binary then route through updater into protocol.

1. Use strict oracles.

- Hangs: timeout exit 124.
- Memory/UB findings: sanitizer signature lines in logs.
- Fatal process exits: signal-style return codes (>= 128).

1. Save every interesting input + log.

- Keep reproducer and stderr/stdout log together.

1. Feed minimized repros back into corpus.

- Grow a stable regression seed set over time.

## Campaign Priority

1. parse campaign (highest priority)
2. protocol_text campaign
3. chunk campaign
4. header campaign
5. protocol_pipeline campaign

## Suggested Next Enhancements

- Add a small grammar-aware mutator for protocol commands.
- Add deterministic Radamsa seeds for reproducibility.
- Add CI nightly sanitizer fuzz smoke run with bounded iterations.
