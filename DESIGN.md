# Matryoshka — Design: Image Format and Protocol

Specification of the firmware image format and the update protocol used between the updater and the device.

---

## 1. Architecture

- **Device** (matryoshka): parses firmware images and runs the update protocol (START, CHUNK, COMMIT). It keeps a stored firmware version and applies updates when the stream commits.
- **Updater**: reads a firmware image file and sends it to the device by writing protocol commands to stdout. Piping updater into the device runs a full update.

---

## 2. Binary Firmware Image Format

Little-endian.

| Offset | Size | Field          | Description |
|--------|------|----------------|-------------|
| 0x00   | 4    | magic          | 0x495F5746 ("I_FW" LE) |
| 0x04   | 2    | version        | Image format version (e.g. 1) |
| 0x06   | 2    | header_len     | Total header size in bytes |
| 0x08   | 4    | payload_len    | Total payload size (all segments) |
| 0x0C   | 4    | fw_version     | Firmware version (used for rollback check) |
| 0x10   | 2    | num_segments   | Number of segment descriptors |
| 0x12   | 2    | reserved       | Reserved |
| 0x14   | 16   | digest         | Digest (e.g. truncated hash) |
| 0x24   | …    | segments[]     | Segment descriptors |
| …      | …    | payload        | Raw segment data (concatenated) |

**Segment descriptor** (12 bytes each):

| Offset | Size | Field   | Description |
|--------|------|---------|-------------|
| 0      | 4    | addr    | Load address (e.g. flash offset) |
| 4      | 4    | length  | Segment length in bytes |
| 8      | 4    | offset  | Offset into payload for this segment |

---

## 3. Update Protocol

Line-based. The updater sends commands; the device reads them from stdin in `protocol` mode.

| Command | Format                     | Description |
|---------|----------------------------|-------------|
| START   | `START <size>\n`           | Begin update; `size` = image size in bytes |
| CHUNK   | `CHUNK <offset> <hexdata>\n` | Send chunk at byte offset (hex = two hex digits per byte) |
| COMMIT  | `COMMIT\n`                 | Verify and apply update |
| ABORT   | `ABORT\n`                  | Cancel update |
| VERSION | `VERSION\n`                | Query current version (device prints it) |

State flow: Idle → Receiving (after START) → Committed (after COMMIT). ABORT returns to Idle. On COMMIT, the device parses the accumulated image, verifies digest and version, and updates the stored version on success.
