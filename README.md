# FuzzMatryoshka

Radamsa-based fuzzing project for the Matryoshka firmware update parser and protocol implementation.

Upstream target: https://github.com/George-Yanni/Matryoshka

This repository is a fuzzing-focused clone/adaptation of Matryoshka. The main goal is to stress and break parsing and state-machine logic, then use crashes, hangs, and behavioral anomalies to harden the code.

## Main Target

The primary fuzzing targets are:

- Binary firmware parsing path in matryoshka parse mode
- Header parser path in matryoshka header mode
- Protocol/state-machine path in matryoshka protocol mode

The default mutation engine in this project is Radamsa.

## Why This Project

Firmware parsers and update protocol handlers often fail at malformed boundaries. This project keeps the surface area intentionally narrow so fuzzing signal stays high:

- Binary format edge cases
- Length and offset validation
- Invalid transitions in the line-based update protocol
- Corrupted chunk sequencing and malformed payloads

## Build

Build all binaries:

```bash
make
```

Generate a known-valid seed image:

```bash
make seed
```

Expected artifacts:

- matryoshka
- updater
- corpus/valid_fw.bin

## Fuzzing With Radamsa

Install Radamsa (one-time), then use corpus/valid_fw.bin as a base seed.

The examples below are Bash-style (Linux, macOS, WSL, or Git Bash).

### 1) Single mutation sanity check

```bash
radamsa corpus/valid_fw.bin | ./matryoshka parse
```

### 2) Continuous parse fuzz loop

```bash
mkdir -p crashes
for i in $(seq 1 100000); do
  radamsa corpus/valid_fw.bin > /tmp/mut_fw.bin
  ./matryoshka parse < /tmp/mut_fw.bin >/dev/null 2>&1 || {
    echo "crash at iteration $i"
    cp /tmp/mut_fw.bin "crashes/parse_$i.bin"
    continue
  }
done
```

### 3) Header-only fuzz loop

```bash
mkdir -p crashes
for i in $(seq 1 100000); do
  radamsa corpus/valid_fw.bin > /tmp/mut_fw.bin
  ./matryoshka header < /tmp/mut_fw.bin >/dev/null 2>&1 || {
    echo "header crash at iteration $i"
    cp /tmp/mut_fw.bin "crashes/header_$i.bin"
    continue
  }
done
```

### 4) Protocol path fuzzing via updater pipeline

```bash
mkdir -p crashes
for i in $(seq 1 100000); do
  radamsa corpus/valid_fw.bin > /tmp/mut_fw.bin
  ./updater /tmp/mut_fw.bin | ./matryoshka protocol >/dev/null 2>&1 || {
    echo "protocol crash at iteration $i"
    cp /tmp/mut_fw.bin "crashes/protocol_$i.bin"
    continue
  }
done
```

## Triage Guidance

When you observe a crash or hang:

- Save the exact mutated input
- Reproduce with a direct command
- Minimize the sample if possible
- Add the minimized case back into corpus/
- Fix root cause, then keep the test as a regression seed

## Typical Non-Fuzz Usage

```bash
./matryoshka parse < corpus/valid_fw.bin
./matryoshka header < corpus/valid_fw.bin
./updater corpus/valid_fw.bin | ./matryoshka protocol
```

## Interesting path to check [sanitizers branch]
radamsa_ubuntu/ 😇

## Disclaimer

This is a robustness and security testing project. It is intended for defensive testing and hardening of software you own or are authorized to test.
