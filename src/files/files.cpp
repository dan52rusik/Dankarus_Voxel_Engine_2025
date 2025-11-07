#include "files.h"
#include <fstream>
#include <iostream>
#include <sstream>

namespace files {
	std::string read_string(const std::string& filename) {
		std::ifstream file(filename);
		if (!file.is_open()) {
			std::cerr << "could not open file " << filename << std::endl;
			return "";
		}
		std::stringstream buffer;
		buffer << file.rdbuf();
		return buffer.str();
	}

	void write_string(const std::string& filename, const std::string& text) {
		std::ofstream file(filename);
		if (!file.is_open()) {
			std::cerr << "could not open file " << filename << std::endl;
			return;
		}
		file << text;
	}

	void write_bytes(const std::string& filename, const char* data, size_t size) {
		std::ofstream file(filename, std::ios::binary);
		if (!file.is_open()) {
			std::cerr << "Could not open file for writing: " << filename << std::endl;
			return;
		}
		file.write(data, size);
		file.close();
	}
	
	bool file_exists(const std::string& filename) {
		std::ifstream file(filename);
		return file.good();
	}
}

