/**
 * Matryoshka — Protocol state machine implementation
 */
#include "protocol.hpp"
#include "fw_parser.hpp"
#include "fw_format.hpp"
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <limits>
#include <new>

namespace matryoshka {

namespace {

bool parse_hex_byte(const char* s, uint8_t& out) {
	if (!s[0] || !s[1]) return false;
	int a = std::isxdigit(static_cast<unsigned char>(s[0])) ? (s[0] <= '9' ? s[0] - '0' : (s[0] & 0x0F) + 9) : -1;
	int b = std::isxdigit(static_cast<unsigned char>(s[1])) ? (s[1] <= '9' ? s[1] - '0' : (s[1] & 0x0F) + 9) : -1;
	if (a < 0 || b < 0) return false;
	out = static_cast<uint8_t>((a << 4) | b);
	return true;
}

constexpr size_t CHUNK_STATIC_BUF_SIZE = 1024 * 1024;

} // namespace

bool UpdateProtocol::process_single_chunk_line(const char* line, size_t line_len) {
	while (line_len && (line[line_len - 1] == '\n' || line[line_len - 1] == '\r'))
		--line_len;
	if (line_len < 6 || std::memcmp(line, "CHUNK", 5) != 0 || (line[5] != ' ' && line[5] != '\t')) {
		std::cerr << "[chunk] Error: expected format 'CHUNK <offset> <hex_data>'.\n";
		return false;
	}
	const char* rest = line + 6;
	size_t len = line_len - 6;

	unsigned long offset = 0;
	const char* p = rest;
	while (len && *p == ' ') { ++p; --len; }
	bool has_offset_digits = false;
	for (; len && *p >= '0' && *p <= '9'; ++p, --len)
	{
		has_offset_digits = true;
		offset = offset * 10 + static_cast<unsigned long>(*p - '0');
	}
	if (!has_offset_digits) {
		std::cerr << "[chunk] Error: missing numeric offset.\n";
		return false;
	}
	while (len && (*p == ' ' || *p == '\t')) { ++p; --len; }
	if (len == 0) {
		std::cerr << "[chunk] Error: missing hex payload after offset " << offset << ".\n";
		return false;
	}

	static uint8_t chunk_buf[CHUNK_STATIC_BUF_SIZE];
	size_t off = static_cast<size_t>(offset);
	size_t parsed_bytes = 0;
	bool syntax_error = false;
	for (; len >= 2 && p[0] && p[1] && off < CHUNK_STATIC_BUF_SIZE; p += 2, len -= 2, off++) {
		uint8_t byte;
		if (!parse_hex_byte(p, byte)) {
			syntax_error = true;
			break;
		}
		chunk_buf[off] = byte;
		parsed_bytes++;
	}
	if (off >= CHUNK_STATIC_BUF_SIZE && len >= 2) {
		std::cerr << "[chunk] Error: write exceeded standalone chunk buffer limit (" << CHUNK_STATIC_BUF_SIZE << " bytes).\n";
		return false;
	}
	while (len && std::isspace(static_cast<unsigned char>(*p))) { ++p; --len; }
	if (len != 0 || syntax_error) {
		std::cerr << "[chunk] Error: invalid hex payload encoding near: '";
		for (size_t i = 0; i < std::min<size_t>(len, 12); ++i) std::cerr << p[i];
		std::cerr << "'.\n";
		return false;
	}
	std::cerr << "[chunk] OK: parsed " << parsed_bytes << " byte(s) at offset " << offset << ".\n";
	return true;
}

UpdateProtocol::UpdateProtocol()
	: state_(UpdateState::Idle)
	, current_version_(1)
	, declared_size_(0) {}

bool UpdateProtocol::process_line(const char* line, size_t line_len) {
	// Trim trailing newline
	while (line_len && (line[line_len - 1] == '\n' || line[line_len - 1] == '\r'))
		--line_len;
	if (line_len == 0) return true;

	if (line_len >= 5 && std::memcmp(line, "START", 5) == 0 && (line_len == 5 || line[5] == ' ' || line[5] == '\t')) {
		return cmd_start(line + 5, line_len - 5);
	}
	if (line_len >= 5 && std::memcmp(line, "CHUNK", 5) == 0 && (line_len == 5 || line[5] == ' ' || line[5] == '\t')) {
		return cmd_chunk(line + 5, line_len - 5);
	}
	if (line_len == 6 && std::memcmp(line, "COMMIT", 6) == 0) {
		return cmd_commit();
	}
	if (line_len == 5 && std::memcmp(line, "ABORT", 5) == 0) {
		return cmd_abort();
	}
	if (line_len == 7 && std::memcmp(line, "VERSION", 7) == 0) {
		std::cout << "version " << current_version_ << "\n";
		return true;
	}
	std::cerr << "[protocol] Warning: unknown command ignored: '";
	for (size_t i = 0; i < std::min<size_t>(line_len, 64); ++i) std::cerr << line[i];
	if (line_len > 64) std::cerr << "...";
	std::cerr << "'\n";
	return true;
}

bool UpdateProtocol::cmd_start(const char* rest, size_t len) {
	if (state_ == UpdateState::Receiving) {
		std::cerr << "[protocol] Warning: START received while already receiving; previous buffer was reset.\n";
	}

	size_t val = 0;
	const char* p = rest;
	while (len && std::isspace(static_cast<unsigned char>(*p))) { ++p; --len; }
	bool has_digits = false;
	for (; len && *p >= '0' && *p <= '9'; ++p, --len)
	{
		has_digits = true;
		if (val > (std::numeric_limits<size_t>::max() - 9) / 10) {
			std::cerr << "[protocol] Error: START size overflow.\n";
			return true;
		}
		val = val * 10 + static_cast<size_t>(*p - '0');
	}
	while (len && std::isspace(static_cast<unsigned char>(*p))) { ++p; --len; }
	if (!has_digits || len != 0) {
		std::cerr << "[protocol] Error: invalid START syntax. Use: START <size>.\n";
		return true;
	}
	declared_size_ = val;
	buffer_.clear();
	try {
		buffer_.resize(declared_size_, 0);
	} catch (const std::bad_alloc&) {
		std::cerr << "[protocol] Error: unable to allocate buffer for START size=" << declared_size_ << ".\n";
		declared_size_ = 0;
		state_ = UpdateState::Idle;
		return true;
	}
	state_ = UpdateState::Receiving;
	std::cerr << "[protocol] START accepted: expecting " << declared_size_ << " bytes.\n";
	return true;
}

bool UpdateProtocol::cmd_chunk(const char* rest, size_t len) {
	if (state_ != UpdateState::Receiving) {
		std::cerr << "[protocol] Warning: CHUNK ignored because no active START session exists.\n";
		return true;
	}

	unsigned long offset = 0;
	const char* p = rest;
	while (len && std::isspace(static_cast<unsigned char>(*p))) { ++p; --len; }
	bool has_offset_digits = false;
	for (; len && *p >= '0' && *p <= '9'; ++p, --len)
	{
		has_offset_digits = true;
		offset = offset * 10 + static_cast<unsigned long>(*p - '0');
	}
	while (len && std::isspace(static_cast<unsigned char>(*p))) { ++p; --len; }
	if (!has_offset_digits) {
		std::cerr << "[protocol] Error: CHUNK missing numeric offset.\n";
		return true;
	}
	if (len == 0) {
		std::cerr << "[protocol] Error: CHUNK missing hex payload.\n";
		return true;
	}

	size_t off = static_cast<size_t>(offset);
	size_t written = 0;
	bool had_invalid_hex = false;
	bool had_oob = false;
	for (; len >= 2 && p[0] && p[1]; p += 2, len -= 2, off++) {
		uint8_t byte;
		if (!parse_hex_byte(p, byte)) {
			had_invalid_hex = true;
			break;
		}
		if (off < buffer_.size()) {
			buffer_[off] = byte;
			written++;
		} else {
			had_oob = true;
		}
	}
	while (len && std::isspace(static_cast<unsigned char>(*p))) { ++p; --len; }
	if (len != 0 || had_invalid_hex) {
		std::cerr << "[protocol] Error: CHUNK contains invalid hex data near: '";
		for (size_t i = 0; i < std::min<size_t>(len, 12); ++i) std::cerr << p[i];
		std::cerr << "'.\n";
		return true;
	}
	if (had_oob) {
		std::cerr << "[protocol] Warning: CHUNK wrote beyond declared START size; extra bytes were ignored.\n";
	}
	std::cerr << "[protocol] CHUNK accepted: offset=" << offset << ", bytes_written=" << written << ".\n";
	return true;
}

bool UpdateProtocol::cmd_commit() {
	if (state_ == UpdateState::Idle) {
		std::cerr << "[protocol] Warning: COMMIT ignored because no update session is active.\n";
		return true;
	}
	if (state_ == UpdateState::Committed) {
		std::cerr << "[protocol] Warning: COMMIT ignored because session is already committed.\n";
		return true;
	}

	if (state_ == UpdateState::Receiving && !buffer_.empty()) {
		FwImage img;
		bool ok = parse_firmware_image(buffer_.data(), buffer_.size(), img);
		if (!ok) {
			std::cerr << "[protocol] COMMIT failed: buffered image is not a valid firmware structure.\n";
		} else if (!verify_firmware_digest(img, buffer_.data(), buffer_.size())) {
			std::cerr << "[protocol] COMMIT failed: digest verification failed.\n";
		} else if (!check_rollback(current_version_, img.header.fw_version)) {
			std::cerr << "[protocol] COMMIT rejected: rollback protection blocked version "
				<< img.header.fw_version << " (current=" << current_version_ << ").\n";
		} else {
				current_version_ = img.header.fw_version;
			std::cerr << "[protocol] COMMIT applied successfully. Current version is now " << current_version_ << ".\n";
		}
	} else {
		std::cerr << "[protocol] COMMIT received with empty payload buffer; nothing was applied.\n";
	}
	state_ = UpdateState::Committed;
	return true;
}

bool UpdateProtocol::cmd_abort() {
	buffer_.clear();
	declared_size_ = 0;
	state_ = UpdateState::Idle;
	std::cerr << "[protocol] ABORT accepted: update session reset to Idle.\n";
	return true;
}

} // namespace matryoshka
