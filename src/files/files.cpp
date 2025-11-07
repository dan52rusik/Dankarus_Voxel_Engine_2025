#include "files.h"
#include <fstream>
#include <iostream>

namespace files {
	void write_bytes(const std::string& filename, const char* data, size_t size) {
		std::ofstream file(filename, std::ios::binary);
		if (!file.is_open()) {
			std::cerr << "Could not open file for writing: " << filename << std::endl;
			return;
		}
		file.write(data, size);
		file.close();
	}
}

