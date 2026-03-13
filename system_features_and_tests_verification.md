Matryoshka - System Features and Tests Verification Guide
Date: 2026-03-13
Purpose: Single command checklist for feature validation and robustness testing.

Notes
- Run all commands from repo root.
- Run commands one per line. If you use a one-liner, separate commands with `&&`.
- Many status messages are printed to stderr. To view everything in one stream, append: `2>&1`
- Expected output below means key lines/keywords, not necessarily exact full logs.

## 0) Setup

**Command:**

```bash
cd ~/Desktop/Matryoshka
sudo apt update
sudo apt install -y clang make
make clean
make
make seed
```

**One-line equivalent (safe separators):**

```bash
cd ~/Desktop/Matryoshka && sudo apt update && sudo apt install -y clang make && make clean && make && make seed
```

**Expected:**

- Build succeeds (exit code 0)
- Files exist: `./matryoshka`, `./updater`, `corpus/valid_fw.bin`

**Optional check:**

```bash
ls -l matryoshka updater corpus/valid_fw.bin
```

## 1) System Feature Verification (Happy Path)

### 1.1 Parse Full Firmware Image

**Command:**

```bash
./matryoshka parse < corpus/valid_fw.bin
echo $?
```

**Expected:**

- Contains: `[parse] OK: image parsed successfully`
- Exit code: 0

### 1.2 Parse Header Only

**Command:**

```bash
./matryoshka header < corpus/valid_fw.bin
echo $?
```

**Expected:**

- Contains: `[header] OK:`
- Shows fields like magic, format_version, fw_version, payload_len
- Exit code: 0

### 1.3 Process Single CHUNK Line

**Command:**

```bash
echo "CHUNK 0 49464d00" | ./matryoshka chunk
echo $?
```

**Expected:**

- Contains: `[chunk] OK: parsed`
- Exit code: 0

### 1.4 Protocol Interactive VERSION Query

**Command:**

```bash
printf "VERSION\n" | ./matryoshka protocol
echo $?
```

**Expected:**

- Contains: `version 1`
- Contains readiness/session messages from `[protocol]`
- Exit code: 0

### 1.5 Updater Stream Generation

**Command:**

```bash
./updater corpus/valid_fw.bin
echo $?
```

**Expected:**

- Stdout contains START, multiple CHUNK lines, COMMIT
- Stderr contains updater info, e.g. `[updater] Loaded ...` and `Stream emitted successfully`
- Exit code: 0

### 1.6 End-to-End Update

**Command:**

```bash
./updater corpus/valid_fw.bin | ./matryoshka protocol
echo $?
```

**Expected:**

- Contains protocol progress messages
- Contains: `[protocol] COMMIT applied successfully`
- Contains: `version 2`
- Exit code: 0

### 1.7 Seed Generation Feature

**Command:**

```bash
make seed
ls -l corpus/valid_fw.bin
```

**Expected:**

- `corpus/valid_fw.bin` exists

## 2) Negative and Robustness Tests (thingsToTest.md)

### 2.0 Prepare temp artifacts

**Command:**

```bash
cp corpus/valid_fw.bin /tmp/valid_fw.bin
```

**Expected:**

- `/tmp/valid_fw.bin` exists

### 2.1 Magic Number Corruption (Parser must reject)

**Command:**

```bash
cp /tmp/valid_fw.bin /tmp/bad_magic.bin
printf '\x00\x00\x00\x00' | dd of=/tmp/bad_magic.bin bs=1 seek=0 conv=notrunc status=none
./matryoshka parse < /tmp/bad_magic.bin
echo $?
```

**Expected:**

- Contains: `[parse] Error:`
- Exit code: non-zero

### 2.2 Truncated File (Parser must reject)

**Command:**

```bash
head -c 20 /tmp/valid_fw.bin > /tmp/truncated.bin
./matryoshka parse < /tmp/truncated.bin
echo $?
```

**Expected:**

- Contains: `[parse] Error:`
- Exit code: non-zero

### 2.3 Digest Mismatch (Payload modified)

**Command:**

```bash
cp /tmp/valid_fw.bin /tmp/bad_digest.bin
printf '\xFF' | dd of=/tmp/bad_digest.bin bs=1 seek=60 conv=notrunc status=none
./matryoshka parse < /tmp/bad_digest.bin
echo $?
```

**Expected:**

- Contains: `[parse] Error: firmware digest verification failed`
- Exit code: non-zero

### 2.4 CHUNK Before START (State violation)

**Command:**

```bash
printf "CHUNK 0 00\nCOMMIT\n" | ./matryoshka protocol
echo $?
```

**Expected:**

- Contains warning that CHUNK is ignored without active START
- Contains COMMIT ignored/failure message
- Ends with version 1
- Exit code: 0

### 2.5 Invalid Hex in CHUNK

**Command:**

```bash
printf "START 4\nCHUNK 0 ZZ\nCOMMIT\n" | ./matryoshka protocol
echo $?
```

**Expected:**

- Contains: `[protocol] Error: CHUNK contains invalid hex data`
- Ends with version 1
- Exit code: 0

### 2.6 Out-of-Bounds CHUNK Offset

**Command:**

```bash
printf "START 4\nCHUNK 10 0000\nCOMMIT\n" | ./matryoshka protocol
echo $?
```

**Expected:**

- Contains warning about bytes ignored beyond declared size
- No crash
- Ends with version 1 (or unchanged version)
- Exit code: 0

### 2.7 Huge START Size (allocation safety)

**Command:**

```bash
printf "START 999999999999\n" | ./matryoshka protocol
echo $?
```

**Expected:**

- Contains START overflow or allocation failure message
- No crash
- Exit code: 0

### 2.8 Double START While Receiving

**Command:**

```bash
printf "START 10\nSTART 8\nABORT\n" | ./matryoshka protocol
echo $?
```

**Expected:**

- Contains warning: START received while already receiving; buffer reset
- ABORT accepted
- Exit code: 0

### 2.9 ABORT Flow

**Command:**

```bash
printf "START 8\nCHUNK 0 00112233\nABORT\nCOMMIT\n" | ./matryoshka protocol
echo $?
```

**Expected:**

- Contains: `ABORT accepted: update session reset to Idle`
- COMMIT should be ignored or do nothing
- Version remains unchanged
- Exit code: 0

### 2.10 Unknown Command Handling

**Command:**

```bash
printf "HELLO\nVERSION\n" | ./matryoshka protocol
echo $?
```

**Expected:**

- Contains warning: unknown command ignored
- Prints version line
- Exit code: 0

### 2.11 Updater Missing File

**Command:**

```bash
./updater /tmp/does_not_exist.bin
echo $?
```

**Expected:**

- Contains: `[updater] Error: cannot open firmware file`
- Exit code: non-zero

### 2.12 Updater Empty File

**Command:**

```bash
: > /tmp/empty.bin
./updater /tmp/empty.bin
echo $?
```

**Expected:**

- Stdout contains START 0 and COMMIT
- Stderr contains empty-file warning
- Exit code: 0

### 2.13 Device Handling of Empty Update Stream

**Command:**

```bash
./updater /tmp/empty.bin | ./matryoshka protocol
echo $?
```

**Expected:**

- No crash
- Version remains 1 (or unchanged)
- Exit code: 0

### 2.14 Partial Stream Integration

**Command:**

```bash
./updater corpus/valid_fw.bin | head -n 3 | ./matryoshka protocol
echo $?
```

**Expected:**

- No crash
- COMMIT not properly applied due to incomplete stream
- Version remains unchanged
- Exit code: 0

## 3) Optional: One-Liner Smoke Check

**Command:**

```bash
./matryoshka parse < corpus/valid_fw.bin && \
./matryoshka header < corpus/valid_fw.bin && \
echo "CHUNK 0 49464d00" | ./matryoshka chunk && \
./updater corpus/valid_fw.bin | ./matryoshka protocol
```

**Expected:**

- Parse/header/chunk pass messages
- Final version line should include version 2

## 4) Pass Criteria Summary
- Happy path tests all pass with exit code 0.
- Parser corruption/truncation/digest tests fail with non-zero exit code.
- Protocol invalid/state-violation tests do not crash and produce explicit warnings/errors.
- End-to-end valid update reaches version 2.
