/**
 * Matryoshka — Firmware update protocol state machine.
 * Commands: START <size>, CHUNK <offset> <hex>, COMMIT, ABORT, VERSION
 */
#ifndef MATRYOSHKA_PROTOCOL_HPP
#define MATRYOSHKA_PROTOCOL_HPP

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>

namespace matryoshka {

enum class UpdateState { Idle, Receiving, Verifying, Committed };

class UpdateProtocol {
public:
	UpdateProtocol();
	/** Process one line of input. Returns false on fatal error. */
	bool process_line(const char* line, size_t line_len);
	UpdateState state() const { return state_; }
	uint32_t current_version() const { return current_version_; }

	/** Process a single CHUNK line into a fixed buffer (standalone entry point). */
	static bool process_single_chunk_line(const char* line, size_t line_len);

private:
	bool cmd_start(const char* rest, size_t len);
	bool cmd_chunk(const char* rest, size_t len);
	bool cmd_commit();
	bool cmd_abort();

	UpdateState state_;
	uint32_t current_version_;
	size_t declared_size_;
	std::vector<uint8_t> buffer_;  // Chunk accumulation
};

} // namespace matryoshka

#endif /* MATRYOSHKA_PROTOCOL_HPP */
