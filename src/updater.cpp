/**
 * Matryoshka — Update client: reads a firmware image file and sends it
 * to the device via the update protocol on stdout.
 * Usage: ./updater <firmware.bin>
 *        ./updater firmware.bin | ./matryoshka protocol
 */
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <iomanip>

static const size_t CHUNK_BYTES = 32;

int main(int argc, char** argv) {
	if (argc < 2) {
		std::cerr << "Usage: " << (argv[0] ? argv[0] : "updater") << " <firmware.bin>\n";
		return 1;
	}
	std::ifstream f(argv[1], std::ios::binary | std::ios::ate);
	if (!f) {
		std::cerr << "[updater] Error: cannot open firmware file: " << argv[1] << "\n";
		return 1;
	}
	std::streamsize size = f.tellg();
	if (size < 0) {
		std::cerr << "[updater] Error: failed to determine file size for: " << argv[1] << "\n";
		return 1;
	}
	f.seekg(0);
	std::vector<uint8_t> buf(static_cast<size_t>(size));
	if (!f.read(reinterpret_cast<char*>(buf.data()), size)) {
		std::cerr << "[updater] Error: failed to read file data from: " << argv[1] << "\n";
		return 1;
	}
	size_t n = static_cast<size_t>(size);
	std::cerr << "[updater] Loaded " << n << " byte(s) from " << argv[1] << ".\n";
	if (n == 0) {
		std::cerr << "[updater] Warning: firmware file is empty; output stream will contain START 0 and COMMIT only.\n";
	}

	std::cout << "START " << n << "\n";
	size_t chunk_count = 0;
	for (size_t offset = 0; offset < n; offset += CHUNK_BYTES) {
		size_t len = (offset + CHUNK_BYTES <= n) ? CHUNK_BYTES : (n - offset);
		std::cout << std::dec;
		std::cout << "CHUNK " << offset << " ";
		for (size_t i = 0; i < len; ++i) {
			std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned>(buf[offset + i]);
		}
		std::cout << std::dec;
		std::cout << "\n";
		chunk_count++;
	}
	std::cout << "COMMIT\n";
	std::cerr << "[updater] Stream emitted successfully: START + " << chunk_count << " CHUNK command(s) + COMMIT.\n";
	return 0;
}
