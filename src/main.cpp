/**
 * Matryoshka — Device: firmware image parser and update protocol handler.
 * Usage:
 *   matryoshka parse   < file.bin   — parse full firmware image from stdin
 *   matryoshka header  < file.bin   — parse header only from stdin
 *   matryoshka protocol             — read protocol stream from stdin; prints version on exit
 *   matryoshka chunk   < file.txt   — process single CHUNK line from stdin
 */
#include "fw_parser.hpp"
#include "fw_format.hpp"
#include "protocol.hpp"
#include <iostream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <string>
#include <iomanip>

namespace {

std::vector<uint8_t> read_stdin() {
	std::vector<uint8_t> buf;
	char block[4096];
	while (std::cin.read(block, sizeof(block)) || std::cin.gcount() > 0) {
		size_t n = static_cast<size_t>(std::cin.gcount());
		buf.insert(buf.end(), block, block + n);
	}
	return buf;
}

} // namespace

int main(int argc, char** argv) {
	const char* mode = (argc >= 2) ? argv[1] : "parse";

	if (std::strcmp(mode, "protocol") == 0) {
		matryoshka::UpdateProtocol proto;
		std::string line;
		std::cerr << "[protocol] Ready: accepted commands are START, CHUNK, COMMIT, ABORT, VERSION.\n";
		while (std::getline(std::cin, line)) {
			proto.process_line(line.c_str(), line.size());
		}
		std::cout << "version " << proto.current_version() << "\n";
		std::cerr << "[protocol] Session complete. Final version=" << proto.current_version() << "\n";
		return 0;
	}

	if (std::strcmp(mode, "chunk") == 0) {
		std::vector<uint8_t> raw = read_stdin();
		if (raw.empty()) {
			std::cerr << "[chunk] Error: no input line received on stdin.\n";
			return 1;
		}
		const char* p = reinterpret_cast<const char*>(raw.data());
		size_t len = raw.size();
		while (len && (p[len - 1] == '\n' || p[len - 1] == '\r')) len--;
		bool ok = matryoshka::UpdateProtocol::process_single_chunk_line(p, len);
		if (!ok) {
			std::cerr << "[chunk] Failed to parse CHUNK input line.\n";
			return 1;
		}
		std::cerr << "[chunk] CHUNK line parsed successfully.\n";
		return 0;
	}

	if (std::strcmp(mode, "header") == 0) {
		std::vector<uint8_t> buf = read_stdin();
		if (buf.size() < sizeof(matryoshka::FwHeader)) {
			std::cerr << "[header] Error: input too small for header. Need at least "
				<< sizeof(matryoshka::FwHeader) << " bytes, got " << buf.size() << ".\n";
			return 1;
		}
		matryoshka::FwHeader hdr;
		if (!matryoshka::parse_header_only(buf.data(), buf.size(), hdr)) {
			std::cerr << "[header] Error: invalid firmware header (magic/version/header_len).\n";
			return 1;
		}
		std::cerr << "[header] OK: magic=0x" << std::hex << std::setw(8) << std::setfill('0') << hdr.magic
			<< std::dec << ", format_version=" << hdr.version
			<< ", fw_version=" << hdr.fw_version
			<< ", header_len=" << hdr.header_len
			<< ", payload_len=" << hdr.payload_len
			<< ", segments=" << hdr.num_segments << "\n";
		return 0;
	}

	// parse: full firmware image
	std::vector<uint8_t> buf = read_stdin();
	if (buf.empty()) {
		std::cerr << "[parse] Error: no input bytes received on stdin.\n";
		return 1;
	}

	matryoshka::FwImage img;
	if (!matryoshka::parse_firmware_image(buf.data(), buf.size(), img)) {
		std::cerr << "[parse] Error: firmware image parse failed (invalid/truncated header or structure).\n";
		return 1;
	}

	bool digest_ok = matryoshka::verify_firmware_digest(img, buf.data(), buf.size());
	if (!digest_ok) {
		std::cerr << "[parse] Error: firmware digest verification failed.\n";
		return 1;
	}

	bool rollback_ok = matryoshka::check_rollback(0, img.header.fw_version);
	if (!rollback_ok) {
		std::cerr << "[parse] Error: rollback check failed (new version must be greater than current baseline).\n";
		return 1;
	}

	std::cerr << "[parse] OK: image parsed successfully. fw_version=" << img.header.fw_version
		<< ", payload_size=" << img.payload_size
		<< ", num_segments=" << img.header.num_segments << "\n";
	return 0;
}
