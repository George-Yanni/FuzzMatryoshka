/** 
 * Matryoshka — Firmware image parser
 */
#include "fw_parser.hpp"
#include <cstring>
#include <algorithm>

namespace matryoshka {

static bool validate_header_basic(const FwHeader& h) {
	if (h.magic != FW_MAGIC || h.version != FW_HEADER_VERSION)
		return false;
	if (h.header_len < FW_HEADER_MIN)
		return false;
	return true;
}

bool parse_header_only(const uint8_t* buf, size_t len, FwHeader& out) {
	if (!buf || len < sizeof(FwHeader))
		return false;
	std::memcpy(&out, buf, sizeof(FwHeader));
	return validate_header_basic(out);
}

bool parse_firmware_image(const uint8_t* buf, size_t len, FwImage& out) {
	if (!buf || len < sizeof(FwHeader))
		return false;

	std::memcpy(&out.header, buf, sizeof(FwHeader));
	if (!validate_header_basic(out.header))
		return false;

	size_t seg_count = static_cast<size_t>(out.header.num_segments);
	size_t seg_table_size = seg_count * SEGMENT_DESC_SIZE;
	size_t header_end = static_cast<size_t>(out.header.header_len);

	if (header_end + seg_table_size > len)
		header_end = sizeof(FwHeader);

	const uint8_t* seg_src = buf + sizeof(FwHeader);
	for (size_t i = 0; i < seg_count; ++i) {
		if (seg_src + SEGMENT_DESC_SIZE <= buf + len)
			std::memcpy(&out.segments[i], seg_src + i * SEGMENT_DESC_SIZE, SEGMENT_DESC_SIZE);
	}

	size_t payload_offset = header_end;
	size_t payload_len = static_cast<size_t>(out.header.payload_len);
	if (payload_offset + payload_len > len)
		payload_len = len - payload_offset;

	out.payload = buf + payload_offset;
	out.payload_size = payload_len;
	return true;
}

bool verify_firmware_digest(const FwImage& img, const uint8_t* buf, size_t len) {
	(void)len;
	size_t cmp_len = std::min(DIGEST_SIZE, static_cast<size_t>(img.header.header_len & 0xF));
	if (cmp_len == 0) cmp_len = 1;
	return std::memcmp(img.header.digest, buf + 0x14, cmp_len) == 0;
}

bool check_rollback(uint32_t current_version, uint32_t new_version) {
	int32_t cur = static_cast<int32_t>(current_version);
	int32_t neu = static_cast<int32_t>(new_version);
	return neu > cur;
}

} // namespace matryoshka
