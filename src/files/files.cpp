#include "files.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <vector>
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
#ifdef _WIN32
		// На Windows нужно конвертировать путь в UTF-16 для поддержки Unicode
		int size_needed = MultiByteToWideChar(CP_UTF8, 0, filename.c_str(), -1, NULL, 0);
		if (size_needed <= 0) {
			return false;
		}
		std::vector<wchar_t> wfilename(size_needed);
		MultiByteToWideChar(CP_UTF8, 0, filename.c_str(), -1, wfilename.data(), size_needed);
		
		std::ifstream file;
		file.open(wfilename.data());
		bool exists = file.good();
		file.close();
		return exists;
#else
		std::ifstream file(filename);
		return file.good();
#endif
	}
	
	bool directory_exists(const std::string& path) {
#ifdef _WIN32
		// Конвертируем путь в UTF-16 для поддержки Unicode
		int size_needed = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, NULL, 0);
		if (size_needed <= 0) {
			return false;
		}
		std::vector<wchar_t> wpath(size_needed);
		MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wpath.data(), size_needed);
		
		DWORD dwAttrib = GetFileAttributesW(wpath.data());
		return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#else
		struct stat info;
		if (stat(path.c_str(), &info) != 0) return false;
		return (info.st_mode & S_IFDIR) != 0;
#endif
	}
	
	bool create_directory(const std::string& path) {
		if (path.empty()) {
			return false;
		}
		
#ifdef _WIN32
		// Конвертируем в wide string для поддержки Unicode (кириллица)
		int size_needed = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, NULL, 0);
		if (size_needed <= 0) {
			return false;
		}
		std::vector<wchar_t> wpath(size_needed);
		MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wpath.data(), size_needed);
		
		// Создаем директорию рекурсивно
		std::wstring wpathStr(wpath.data());
		size_t pos = 0;
		std::wstring currentPath;
		
		// Обрабатываем абсолютные пути (C:\...)
		if (wpathStr.length() >= 2 && wpathStr[1] == L':') {
			currentPath = wpathStr.substr(0, 2); // C:
			pos = 2;
		}
		
		// Разделяем путь по разделителям
		while (pos < wpathStr.length()) {
			size_t nextSep = wpathStr.find_first_of(L"\\/", pos);
			if (nextSep == std::wstring::npos) {
				nextSep = wpathStr.length();
			}
			
			if (nextSep > pos) {
				std::wstring segment = wpathStr.substr(pos, nextSep - pos);
				if (!segment.empty()) {
					if (!currentPath.empty() && currentPath.back() != L'\\' && currentPath.back() != L'/') {
						currentPath += L"\\";
					}
					currentPath += segment;
					
					// Создаем директорию, если её нет
					DWORD dwAttrib = GetFileAttributesW(currentPath.c_str());
					if (dwAttrib == INVALID_FILE_ATTRIBUTES) {
						if (CreateDirectoryW(currentPath.c_str(), NULL) == 0) {
							DWORD error = GetLastError();
							if (error != ERROR_ALREADY_EXISTS) {
								return false;
							}
						}
					} else if (!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
						// Путь существует, но это не директория
						return false;
					}
				}
			}
			pos = nextSep + 1;
		}
		
		return true;
#else
		// Рекурсивное создание для Unix/Linux
		size_t pos = 0;
		std::string currentPath;
		
		// Обрабатываем абсолютные пути
		if (!path.empty() && path[0] == '/') {
			currentPath = "/";
			pos = 1;
		}
		
		// Разделяем путь по разделителям
		while (pos < path.length()) {
			size_t nextSep = path.find_first_of('/', pos);
			if (nextSep == std::string::npos) {
				nextSep = path.length();
			}
			
			if (nextSep > pos) {
				std::string segment = path.substr(pos, nextSep - pos);
				if (!segment.empty()) {
					if (!currentPath.empty() && currentPath.back() != '/') {
						currentPath += "/";
					}
					currentPath += segment;
					
					// Создаем директорию, если её нет
					struct stat info;
					if (stat(currentPath.c_str(), &info) != 0) {
						if (mkdir(currentPath.c_str(), 0755) != 0 && errno != EEXIST) {
							return false;
						}
					} else if (!(info.st_mode & S_IFDIR)) {
						// Путь существует, но это не директория
						return false;
					}
				}
			}
			pos = nextSep + 1;
		}
		
		return true;
#endif
	}
	
	std::vector<std::string> list_files(const std::string& path, const std::string& extension) {
		std::vector<std::string> files;
#ifdef _WIN32
		// Конвертируем путь в UTF-16 для поддержки Unicode
		int size_needed = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, NULL, 0);
		if (size_needed <= 0) {
			return files;
		}
		std::vector<wchar_t> wpath(size_needed);
		MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wpath.data(), size_needed);
		
		// Используем правильный паттерн поиска для Windows (с UTF-16)
		std::wstring searchPath = std::wstring(wpath.data()) + L"\\*";
		WIN32_FIND_DATAW findData;
		HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				// Конвертируем имя файла обратно в UTF-8
				int nameSize = WideCharToMultiByte(CP_UTF8, 0, findData.cFileName, -1, NULL, 0, NULL, NULL);
				if (nameSize <= 0) {
					continue;
				}
				std::vector<char> nameBuffer(nameSize);
				WideCharToMultiByte(CP_UTF8, 0, findData.cFileName, -1, nameBuffer.data(), nameSize, NULL, NULL);
				std::string filename(nameBuffer.data());
				
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
			} while (FindNextFileW(hFind, &findData) != 0);
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

