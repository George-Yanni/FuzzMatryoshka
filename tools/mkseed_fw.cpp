/**
 * Matryoshka — Generate a valid firmware image (corpus/valid_fw.bin).
 * Run from project root or from corpus/ so valid_fw.bin is created in the right place.
 */
#include "fw_format.hpp"
#include <cstdio>
#include <cstring>
#include <vector>
#include <cstdint>

int main() {
	using namespace matryoshka;

	std::vector<uint8_t> out;
	out.resize(FW_HEADER_MIN + 2 * SEGMENT_DESC_SIZE + 64, 0);  // header + 2 segments + small payload

	FwHeader* h = reinterpret_cast<FwHeader*>(out.data());
	h->magic = FW_MAGIC;
	h->version = FW_HEADER_VERSION;
	h->header_len = static_cast<uint16_t>(FW_HEADER_MIN + 2 * SEGMENT_DESC_SIZE);
	h->payload_len = 64;
	h->fw_version = 2;
	h->num_segments = 2;
	h->reserved = 0;
	std::memset(h->digest, 0xAB, DIGEST_SIZE);

	FwSegment* segs = reinterpret_cast<FwSegment*>(out.data() + sizeof(FwHeader));
	segs[0].addr = 0x08000000;
	segs[0].length = 32;
	segs[0].offset = 0;
	segs[1].addr = 0x08001000;
	segs[1].length = 32;
	segs[1].offset = 32;

	// Payload bytes
	for (size_t i = 0; i < 64; ++i)
		out[FW_HEADER_MIN + 2 * SEGMENT_DESC_SIZE + i] = static_cast<uint8_t>(i & 0xFF);

	FILE* f = std::fopen("valid_fw.bin", "wb");
	if (!f) return 1;
	std::fwrite(out.data(), 1, out.size(), f);
	std::fclose(f);
	return 0;
}
