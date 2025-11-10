#include "files.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

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
	
	bool directory_exists(const std::string& path) {
#ifdef _WIN32
		DWORD dwAttrib = GetFileAttributesA(path.c_str());
		return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#else
		struct stat info;
		if (stat(path.c_str(), &info) != 0) return false;
		return (info.st_mode & S_IFDIR) != 0;
#endif
	}
	
	bool create_directory(const std::string& path) {
#ifdef _WIN32
		return CreateDirectoryA(path.c_str(), NULL) != 0 || GetLastError() == ERROR_ALREADY_EXISTS;
#else
		return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
#endif
	}
	
	std::vector<std::string> list_files(const std::string& path, const std::string& extension) {
		std::vector<std::string> files;
#ifdef _WIN32
		// Используем правильный паттерн поиска для Windows
		std::string searchPath = path + "\\*";
		WIN32_FIND_DATAA findData;
		HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				std::string filename = findData.cFileName;
				// Пропускаем . и ..
				if (filename == "." || filename == "..") continue;
				
				// Если extension пустой, возвращаем все (папки и файлы)
				// Если extension указан, возвращаем только файлы с таким расширением
				bool isDirectory = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
				
				if (extension.empty()) {
					// Возвращаем все (папки и файлы)
					files.push_back(filename);
				} else if (!isDirectory) {
					// Возвращаем только файлы с указанным расширением
					if (filename.size() >= extension.size()) {
						if (filename.substr(filename.size() - extension.size()) == extension) {
							files.push_back(filename);
						}
					}
				}
			} while (FindNextFileA(hFind, &findData) != 0);
			FindClose(hFind);
		} else {
			// Если папка не найдена, выводим ошибку
			DWORD error = GetLastError();
			if (error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND) {
				std::cerr << "[FILES] Error listing files in " << path << ": " << error << std::endl;
			}
		}
#else
		DIR* dir = opendir(path.c_str());
		if (dir != nullptr) {
			struct dirent* entry;
			while ((entry = readdir(dir)) != nullptr) {
				std::string filename = entry->d_name;
				if (filename != "." && filename != "..") {
					bool isDirectory = (entry->d_type == DT_DIR);
					
					if (extension.empty()) {
						// Возвращаем все (папки и файлы)
						files.push_back(filename);
					} else if (!isDirectory) {
						// Возвращаем только файлы с указанным расширением
						if (filename.size() >= extension.size()) {
							if (filename.substr(filename.size() - extension.size()) == extension) {
								files.push_back(filename);
							}
						}
					}
				}
			}
			closedir(dir);
		}
#endif
		return files;
	}
}

