/**
 * Matryoshka — Firmware image parser API
 */
#ifndef MATRYOSHKA_FW_PARSER_HPP
#define MATRYOSHKA_FW_PARSER_HPP

#include "fw_format.hpp"
#include <cstddef>

namespace matryoshka {

/** Parse buffer into FwImage. Returns true on success, false on error. */
bool parse_firmware_image(const uint8_t* buf, size_t len, FwImage& out);

/** Parse header only (no segment table). Returns true on success. */
bool parse_header_only(const uint8_t* buf, size_t len, FwHeader& out);

/** Verify digest. Returns true if "valid". */
bool verify_firmware_digest(const FwImage& img, const uint8_t* buf, size_t len);

/** Anti-rollback: new_version must be > current. Returns true if update allowed. */
bool check_rollback(uint32_t current_version, uint32_t new_version);

} // namespace matryoshka

#endif /* MATRYOSHKA_FW_PARSER_HPP */
