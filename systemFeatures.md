# System Features

Command tip: run each command on its own line. If you prefer one line, use `&&` between commands.

## 1. Parse Full Firmware Image

**Command:**

```bash
./matryoshka parse < corpus/valid_fw.bin
```

- **Description:** Reads a binary file from stdin, parses the header and segments, and validates the digest.
- **Expected Output:** Exit code 0 on success, non-zero on failure. (No text output on success).

## 2. Parse Header Only

**Command:**

```bash
./matryoshka header < corpus/valid_fw.bin
```

- **Description:** Reads only the first few bytes (header size) to validate magic bytes and version.
- **Expected Output:** Exit code 0 on success.

## 3. Process Single Chunk Line

**Command:**

```bash
echo "CHUNK 0 49464d00" | ./matryoshka chunk
```

- **Description:** Parses a single line of text in the format `CHUNK <offset> <hex_data>`. Used to test the hex parser and offset logic in isolation.
- **Expected Output:** Exit code 0 if the line is valid.

## 4. Run Device Protocol (Interactive Mode)

**Command:**

```bash
./matryoshka protocol
```

- **Description:** Starts the device in a state machine mode where it waits for text commands (`START`, `CHUNK`, `COMMIT`, `ABORT`, `VERSION`) on stdin.
- **Expected Output:** Prints the current version (e.g., "version 1") upon exit or whenever the loop finishes processing input.

## 5. Generate Update Stream (Updater Tool)

**Command:**

```bash
./updater corpus/valid_fw.bin
```

- **Description:** Reads a binary firmware file and converts it into a stream of text commands (`START`, `CHUNK`, `COMMIT`) printed to stdout.
- **Expected Output:** A long list of text commands representing the firmware update.

## 6. Perform Full Update (End-to-End)

**Command:**

```bash
./updater corpus/valid_fw.bin | ./matryoshka protocol
```

- **Description:** Pipes the output of the updater directly into the device. The device should receive the image, verify it, and "install" it (update its internal version).
- **Expected Output:** "version 2" (assuming the seed image is version 2 and the device starts at version 1).

## 7. Generate Seed Image

**Command:**

```bash
make seed
```

(or run `./tools/mkseed_fw` directly if compiled)

- **Description:** Creates a valid sample firmware binary (`corpus/valid_fw.bin`) with known good headers and checksums.
- **Expected Output:** A file named `corpus/valid_fw.bin` is created.
