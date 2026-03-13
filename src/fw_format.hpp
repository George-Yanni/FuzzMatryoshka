/**
 * Matryoshka — Firmware image format definitions
 */
#ifndef MATRYOSHKA_FW_FORMAT_HPP
#define MATRYOSHKA_FW_FORMAT_HPP

#include <cstdint>
#include <cstddef>

namespace matryoshka {

constexpr uint32_t FW_MAGIC          = 0x495F5746u;  // "I_FW" LE
constexpr uint16_t FW_HEADER_VERSION = 1;
constexpr size_t   FW_MAX_SEGMENTS   = 32;
constexpr size_t   SEGMENT_DESC_SIZE = 12;
constexpr size_t   DIGEST_SIZE       = 16;
constexpr size_t   FW_HEADER_MIN     = 0x24;

#pragma pack(push, 1)
struct FwHeader {
	uint32_t magic;
	uint16_t version;
	uint16_t header_len;
	uint32_t payload_len;
	uint32_t fw_version;
	uint16_t num_segments;
	uint16_t reserved;
	uint8_t  digest[DIGEST_SIZE];
};

struct FwSegment {
	uint32_t addr;
	uint32_t length;
	uint32_t offset;
};
#pragma pack(pop)

struct FwImage {
	FwHeader  header;
	FwSegment segments[FW_MAX_SEGMENTS];
	const uint8_t* payload = nullptr;
	size_t         payload_size = 0;
};

} // namespace matryoshka

#endif /* MATRYOSHKA_FW_FORMAT_HPP */
