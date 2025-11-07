#ifndef FILES_FILES_H_
#define FILES_FILES_H_

#include <string>

namespace files {
	std::string read_string(const std::string& filename);
	void write_string(const std::string& filename, const std::string& text);
	void write_bytes(const std::string& filename, const char* bytes, size_t size);
	bool file_exists(const std::string& filename);
}

#endif /* FILES_FILES_H_ */

