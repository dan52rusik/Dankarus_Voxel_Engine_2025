#ifndef FILES_FILES_H_
#define FILES_FILES_H_

#include <string>

namespace files {
	void write_bytes(const std::string& filename, const char* data, size_t size);
}

#endif /* FILES_FILES_H_ */

