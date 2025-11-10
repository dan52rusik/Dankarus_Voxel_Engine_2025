#ifndef FILES_FILES_H_
#define FILES_FILES_H_

#include <string>
#include <vector>

namespace files {
	std::string read_string(const std::string& filename);
	void write_string(const std::string& filename, const std::string& text);
	void write_bytes(const std::string& filename, const char* bytes, size_t size);
	bool file_exists(const std::string& filename);
	
	// Работа с директориями
	bool directory_exists(const std::string& path);
	bool create_directory(const std::string& path);
	std::vector<std::string> list_files(const std::string& path, const std::string& extension = "");
}

#endif /* FILES_FILES_H_ */

