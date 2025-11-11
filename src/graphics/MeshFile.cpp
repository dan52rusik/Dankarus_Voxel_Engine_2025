#include "MeshFile.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstdint>

namespace MeshFile {
	
	// Магические байты для формата файла
	static const char MAGIC[] = "MESH";
	static const int VERSION = 1;
	
	// Вспомогательные функции для записи/чтения
	namespace {
		bool writeInt32(std::ofstream& file, int32_t value) {
			file.write(reinterpret_cast<const char*>(&value), sizeof(int32_t));
			return file.good();
		}
		
		bool writeUInt32(std::ofstream& file, uint32_t value) {
			file.write(reinterpret_cast<const char*>(&value), sizeof(uint32_t));
			return file.good();
		}
		
		bool writeFloat(std::ofstream& file, float value) {
			file.write(reinterpret_cast<const char*>(&value), sizeof(float));
			return file.good();
		}
		
		bool readInt32(std::ifstream& file, int32_t& value) {
			file.read(reinterpret_cast<char*>(&value), sizeof(int32_t));
			return file.good();
		}
		
		bool readUInt32(std::ifstream& file, uint32_t& value) {
			file.read(reinterpret_cast<char*>(&value), sizeof(uint32_t));
			return file.good();
		}
		
		bool readFloat(std::ifstream& file, float& value) {
			file.read(reinterpret_cast<char*>(&value), sizeof(float));
			return file.good();
		}
	}
	
	bool saveMesh(const std::string& filepath, const MeshData& meshData) {
		if (!meshData.isValid()) {
			std::cerr << "[MESHFILE] Invalid mesh data, cannot save" << std::endl;
			return false;
		}
		
		std::ofstream file(filepath, std::ios::binary);
		if (!file.is_open()) {
			std::cerr << "[MESHFILE] Failed to open file for writing: " << filepath << std::endl;
			return false;
		}
		
		// Магические байты
		file.write(MAGIC, 4);
		
		// Версия формата
		if (!writeInt32(file, VERSION)) {
			file.close();
			return false;
		}
		
		// Метаданные
		if (!writeInt32(file, meshData.worldPosition.x) ||
		    !writeInt32(file, meshData.worldPosition.y) ||
		    !writeInt32(file, meshData.worldPosition.z) ||
		    !writeInt32(file, meshData.updateTime)) {
			file.close();
			return false;
		}
		
		// Количество вершин
		uint32_t vertexCount = static_cast<uint32_t>(meshData.vertices.size());
		if (!writeUInt32(file, vertexCount)) {
			file.close();
			return false;
		}
		
		// Записываем вершины (координаты сжаты в int16 для экономии места, как в 7DTD)
		for (size_t i = 0; i < meshData.vertices.size(); i++) {
			int16_t x = static_cast<int16_t>(meshData.vertices[i].x * 100.0f);
			int16_t y = static_cast<int16_t>(meshData.vertices[i].y * 100.0f);
			int16_t z = static_cast<int16_t>(meshData.vertices[i].z * 100.0f);
			file.write(reinterpret_cast<const char*>(&x), sizeof(int16_t));
			file.write(reinterpret_cast<const char*>(&y), sizeof(int16_t));
			file.write(reinterpret_cast<const char*>(&z), sizeof(int16_t));
		}
		
		// Нормали (если есть)
		uint32_t normalCount = static_cast<uint32_t>(meshData.normals.size());
		writeUInt32(file, normalCount);
		if (normalCount > 0) {
			for (size_t i = 0; i < meshData.normals.size(); i++) {
				writeFloat(file, meshData.normals[i].x);
				writeFloat(file, meshData.normals[i].y);
				writeFloat(file, meshData.normals[i].z);
			}
		}
		
		// UV координаты (сжаты в uint16, как в 7DTD)
		uint32_t uvCount = static_cast<uint32_t>(meshData.uvs.size());
		writeUInt32(file, uvCount);
		for (size_t i = 0; i < meshData.uvs.size(); i++) {
			uint16_t u = static_cast<uint16_t>(meshData.uvs[i].x * 10000.0f);
			uint16_t v = static_cast<uint16_t>(meshData.uvs[i].y * 10000.0f);
			file.write(reinterpret_cast<const char*>(&u), sizeof(uint16_t));
			file.write(reinterpret_cast<const char*>(&v), sizeof(uint16_t));
		}
		
		// Цвета (RGBA, сжаты в uint16, как в 7DTD)
		uint32_t colorCount = static_cast<uint32_t>(meshData.colors.size());
		writeUInt32(file, colorCount);
		for (size_t i = 0; i < meshData.colors.size(); i++) {
			uint16_t r = static_cast<uint16_t>(meshData.colors[i].r * 10000.0f);
			uint16_t g = static_cast<uint16_t>(meshData.colors[i].g * 10000.0f);
			uint16_t b = static_cast<uint16_t>(meshData.colors[i].b * 10000.0f);
			uint16_t a = static_cast<uint16_t>(meshData.colors[i].a * 10000.0f);
			file.write(reinterpret_cast<const char*>(&r), sizeof(uint16_t));
			file.write(reinterpret_cast<const char*>(&g), sizeof(uint16_t));
			file.write(reinterpret_cast<const char*>(&b), sizeof(uint16_t));
			file.write(reinterpret_cast<const char*>(&a), sizeof(uint16_t));
		}
		
		// Индексы
		uint32_t indexCount = static_cast<uint32_t>(meshData.indices.size());
		writeUInt32(file, indexCount);
		
		// Используем uint16 для индексов, если возможно (экономия места)
		bool use16Bit = indexCount <= 65535;
		file.write(reinterpret_cast<const char*>(&use16Bit), sizeof(bool));
		
		for (size_t i = 0; i < meshData.indices.size(); i++) {
			if (use16Bit) {
				uint16_t idx = static_cast<uint16_t>(meshData.indices[i]);
				file.write(reinterpret_cast<const char*>(&idx), sizeof(uint16_t));
			} else {
				file.write(reinterpret_cast<const char*>(&meshData.indices[i]), sizeof(uint32_t));
			}
		}
		
		file.close();
		return true;
	}
	
	LoadResult loadMesh(const std::string& filepath, MeshData& meshData) {
		std::ifstream file(filepath, std::ios::binary);
		if (!file.is_open()) {
			std::cerr << "[MESHFILE] Failed to open file for reading: " << filepath << std::endl;
			return LoadResult::Error;
		}
		
		meshData.clear();
		
		// Получаем размер файла для проверок
		file.seekg(0, std::ios::end);
		std::streampos fileSize = file.tellg();
		file.seekg(0, std::ios::beg);
		
		// Проверяем магические байты
		char magic[4];
		file.read(magic, 4);
		if (magic[0] != 'M' || magic[1] != 'E' || magic[2] != 'S' || magic[3] != 'H') {
			std::cerr << "[MESHFILE] Invalid file format: " << filepath << std::endl;
			file.close();
			return LoadResult::Error;
		}
		
		// Версия
		int32_t version;
		if (!readInt32(file, version)) {
			file.close();
			return LoadResult::Error;
		}
		
		// Метаданные
		if (!readInt32(file, meshData.worldPosition.x) ||
		    !readInt32(file, meshData.worldPosition.y) ||
		    !readInt32(file, meshData.worldPosition.z) ||
		    !readInt32(file, meshData.updateTime)) {
			file.close();
			return LoadResult::Error;
		}
		
		// Количество вершин
		uint32_t vertexCount;
		if (!readUInt32(file, vertexCount)) {
			file.close();
			return LoadResult::Error;
		}
		
		// Проверка размера файла для вершин (как в 7DTD ReadVoxelMesh)
		if (file.tellg() + static_cast<std::streampos>(vertexCount * 6) > fileSize) {
			file.close();
			std::cerr << "[MESHFILE] File too small for vertices: " << filepath << std::endl;
			return LoadResult::WrongSize;
		}
		
		// Читаем вершины
		meshData.vertices.reserve(vertexCount);
		meshData.vertices.resize(vertexCount);
		for (uint32_t i = 0; i < vertexCount; i++) {
			int16_t x, y, z;
			file.read(reinterpret_cast<char*>(&x), sizeof(int16_t));
			file.read(reinterpret_cast<char*>(&y), sizeof(int16_t));
			file.read(reinterpret_cast<char*>(&z), sizeof(int16_t));
			if (!file.good()) {
				file.close();
				return LoadResult::Error;
			}
			meshData.vertices[i] = glm::vec3(
				static_cast<float>(x) / 100.0f,
				static_cast<float>(y) / 100.0f,
				static_cast<float>(z) / 100.0f
			);
		}
		
		// Нормали (сжатые в int16, как в 7DTD)
		uint32_t normalCount;
		if (!readUInt32(file, normalCount)) {
			file.close();
			return LoadResult::Error;
		}
		
		if (normalCount > 0) {
			if (file.tellg() + static_cast<std::streampos>(normalCount * 6) > fileSize) {
				file.close();
				return LoadResult::WrongSize;
			}
			
			meshData.normals.reserve(normalCount);
			meshData.normals.resize(normalCount);
			for (uint32_t i = 0; i < normalCount; i++) {
				int16_t nx, ny, nz;
				file.read(reinterpret_cast<char*>(&nx), sizeof(int16_t));
				file.read(reinterpret_cast<char*>(&ny), sizeof(int16_t));
				file.read(reinterpret_cast<char*>(&nz), sizeof(int16_t));
				if (!file.good()) {
					file.close();
					return LoadResult::Error;
				}
				meshData.normals[i] = glm::vec3(
					static_cast<float>(nx) / 100.0f,
					static_cast<float>(ny) / 100.0f,
					static_cast<float>(nz) / 100.0f
				);
			}
		}
		
		// UV координаты
		uint32_t uvCount;
		if (!readUInt32(file, uvCount)) {
			file.close();
			return LoadResult::Error;
		}
		
		if (uvCount > 0) {
			if (file.tellg() + static_cast<std::streampos>(uvCount * 4) > fileSize) {
				file.close();
				return LoadResult::WrongSize;
			}
			
			meshData.uvs.reserve(uvCount);
			meshData.uvs.resize(uvCount);
			for (uint32_t i = 0; i < uvCount; i++) {
				uint16_t u, v;
				file.read(reinterpret_cast<char*>(&u), sizeof(uint16_t));
				file.read(reinterpret_cast<char*>(&v), sizeof(uint16_t));
				if (!file.good()) {
					file.close();
					return LoadResult::Error;
				}
				meshData.uvs[i] = glm::vec2(
					static_cast<float>(u) / 10000.0f,
					static_cast<float>(v) / 10000.0f
				);
			}
		}
		
		// Цвета
		uint32_t colorCount;
		if (!readUInt32(file, colorCount)) {
			file.close();
			return LoadResult::Error;
		}
		
		if (colorCount > 0) {
			if (file.tellg() + static_cast<std::streampos>(colorCount * 8) > fileSize) {
				file.close();
				return LoadResult::WrongSize;
			}
			
			meshData.colors.reserve(colorCount);
			meshData.colors.resize(colorCount);
			for (uint32_t i = 0; i < colorCount; i++) {
				uint16_t r, g, b, a;
				file.read(reinterpret_cast<char*>(&r), sizeof(uint16_t));
				file.read(reinterpret_cast<char*>(&g), sizeof(uint16_t));
				file.read(reinterpret_cast<char*>(&b), sizeof(uint16_t));
				file.read(reinterpret_cast<char*>(&a), sizeof(uint16_t));
				if (!file.good()) {
					file.close();
					return LoadResult::Error;
				}
				meshData.colors[i] = glm::vec4(
					static_cast<float>(r) / 10000.0f,
					static_cast<float>(g) / 10000.0f,
					static_cast<float>(b) / 10000.0f,
					static_cast<float>(a) / 10000.0f
				);
			}
		}
		
		// Индексы
		uint32_t indexCount;
		if (!readUInt32(file, indexCount)) {
			file.close();
			return LoadResult::Error;
		}
		
		if (indexCount > 0) {
			bool use16Bit;
			file.read(reinterpret_cast<char*>(&use16Bit), sizeof(bool));
			
			size_t indexSize = use16Bit ? 2 : 4;
			if (file.tellg() + static_cast<std::streampos>(indexCount * indexSize) > fileSize) {
				file.close();
				return LoadResult::WrongSize;
			}
			
			meshData.indices.reserve(indexCount);
			meshData.indices.resize(indexCount);
			for (uint32_t i = 0; i < indexCount; i++) {
				if (use16Bit) {
					uint16_t idx;
					file.read(reinterpret_cast<char*>(&idx), sizeof(uint16_t));
					if (!file.good()) {
						file.close();
						return LoadResult::Error;
					}
					meshData.indices[i] = static_cast<uint32_t>(idx);
				} else {
					file.read(reinterpret_cast<char*>(&meshData.indices[i]), sizeof(uint32_t));
					if (!file.good()) {
						file.close();
						return LoadResult::Error;
					}
				}
			}
		}
		
		file.close();
		return LoadResult::Ok;
	}
	
	bool loadMeshBool(const std::string& filepath, MeshData& meshData) {
		return loadMesh(filepath, meshData) == LoadResult::Ok;
	}
	
	bool saveMeshes(const std::string& filepath, const std::vector<MeshData>& meshes) {
		std::ofstream file(filepath, std::ios::binary);
		if (!file.is_open()) {
			std::cerr << "[MESHFILE] Failed to open file for writing: " << filepath << std::endl;
			return false;
		}
		
		// Магические байты
		file.write(MAGIC, 4);
		
		// Версия
		writeInt32(file, VERSION);
		
		// Количество мешей
		uint32_t meshCount = static_cast<uint32_t>(meshes.size());
		writeUInt32(file, meshCount);
		
		// Сохраняем каждый меш в поток
		for (const auto& meshData : meshes) {
			if (!meshData.isValid()) {
				continue;
			}
			
			// Метаданные меша
			writeInt32(file, meshData.worldPosition.x);
			writeInt32(file, meshData.worldPosition.y);
			writeInt32(file, meshData.worldPosition.z);
			writeInt32(file, meshData.updateTime);
			
			// Количество вершин
			uint32_t vertexCount = static_cast<uint32_t>(meshData.vertices.size());
			writeUInt32(file, vertexCount);
			
			// Вершины
			for (size_t i = 0; i < meshData.vertices.size(); i++) {
				int16_t x = static_cast<int16_t>(meshData.vertices[i].x * 100.0f);
				int16_t y = static_cast<int16_t>(meshData.vertices[i].y * 100.0f);
				int16_t z = static_cast<int16_t>(meshData.vertices[i].z * 100.0f);
				file.write(reinterpret_cast<const char*>(&x), sizeof(int16_t));
				file.write(reinterpret_cast<const char*>(&y), sizeof(int16_t));
				file.write(reinterpret_cast<const char*>(&z), sizeof(int16_t));
			}
			
		// Нормали (сжатые в int16)
		uint32_t normalCount = static_cast<uint32_t>(meshData.normals.size());
		writeUInt32(file, normalCount);
		for (size_t i = 0; i < meshData.normals.size(); i++) {
			int16_t nx = static_cast<int16_t>(meshData.normals[i].x * 100.0f);
			int16_t ny = static_cast<int16_t>(meshData.normals[i].y * 100.0f);
			int16_t nz = static_cast<int16_t>(meshData.normals[i].z * 100.0f);
			file.write(reinterpret_cast<const char*>(&nx), sizeof(int16_t));
			file.write(reinterpret_cast<const char*>(&ny), sizeof(int16_t));
			file.write(reinterpret_cast<const char*>(&nz), sizeof(int16_t));
		}
		
		// Тангенсы (если есть) - сжатые в int16
		uint32_t tangentCount = static_cast<uint32_t>(meshData.tangents.size());
		writeUInt32(file, tangentCount);
		for (size_t i = 0; i < meshData.tangents.size(); i++) {
			int16_t tx = static_cast<int16_t>(meshData.tangents[i].x * 100.0f);
			int16_t ty = static_cast<int16_t>(meshData.tangents[i].y * 100.0f);
			int16_t tz = static_cast<int16_t>(meshData.tangents[i].z * 100.0f);
			int16_t tw = static_cast<int16_t>(meshData.tangents[i].w * 100.0f);
			file.write(reinterpret_cast<const char*>(&tx), sizeof(int16_t));
			file.write(reinterpret_cast<const char*>(&ty), sizeof(int16_t));
			file.write(reinterpret_cast<const char*>(&tz), sizeof(int16_t));
			file.write(reinterpret_cast<const char*>(&tw), sizeof(int16_t));
		}
		
		// Дополнительные UV каналы (если есть)
		uint32_t uv2Count = static_cast<uint32_t>(meshData.uvs2.size());
		writeUInt32(file, uv2Count);
		for (size_t i = 0; i < meshData.uvs2.size(); i++) {
			uint16_t u = static_cast<uint16_t>(meshData.uvs2[i].x * 10000.0f);
			uint16_t v = static_cast<uint16_t>(meshData.uvs2[i].y * 10000.0f);
			file.write(reinterpret_cast<const char*>(&u), sizeof(uint16_t));
			file.write(reinterpret_cast<const char*>(&v), sizeof(uint16_t));
		}
			
			// UV
			uint32_t uvCount = static_cast<uint32_t>(meshData.uvs.size());
			writeUInt32(file, uvCount);
			for (size_t i = 0; i < meshData.uvs.size(); i++) {
				uint16_t u = static_cast<uint16_t>(meshData.uvs[i].x * 10000.0f);
				uint16_t v = static_cast<uint16_t>(meshData.uvs[i].y * 10000.0f);
				file.write(reinterpret_cast<const char*>(&u), sizeof(uint16_t));
				file.write(reinterpret_cast<const char*>(&v), sizeof(uint16_t));
			}
			
			// Цвета
			uint32_t colorCount = static_cast<uint32_t>(meshData.colors.size());
			writeUInt32(file, colorCount);
			for (size_t i = 0; i < meshData.colors.size(); i++) {
				uint16_t r = static_cast<uint16_t>(meshData.colors[i].r * 10000.0f);
				uint16_t g = static_cast<uint16_t>(meshData.colors[i].g * 10000.0f);
				uint16_t b = static_cast<uint16_t>(meshData.colors[i].b * 10000.0f);
				uint16_t a = static_cast<uint16_t>(meshData.colors[i].a * 10000.0f);
				file.write(reinterpret_cast<const char*>(&r), sizeof(uint16_t));
				file.write(reinterpret_cast<const char*>(&g), sizeof(uint16_t));
				file.write(reinterpret_cast<const char*>(&b), sizeof(uint16_t));
				file.write(reinterpret_cast<const char*>(&a), sizeof(uint16_t));
			}
			
			// Индексы
			uint32_t indexCount = static_cast<uint32_t>(meshData.indices.size());
			writeUInt32(file, indexCount);
			bool use16Bit = indexCount <= 65535;
			file.write(reinterpret_cast<const char*>(&use16Bit), sizeof(bool));
			for (size_t i = 0; i < meshData.indices.size(); i++) {
				if (use16Bit) {
					uint16_t idx = static_cast<uint16_t>(meshData.indices[i]);
					file.write(reinterpret_cast<const char*>(&idx), sizeof(uint16_t));
				} else {
					file.write(reinterpret_cast<const char*>(&meshData.indices[i]), sizeof(uint32_t));
				}
			}
		}
		
		file.close();
		return true;
	}
	
	bool loadMeshes(const std::string& filepath, std::vector<MeshData>& meshes) {
		std::ifstream file(filepath, std::ios::binary);
		if (!file.is_open()) {
			std::cerr << "[MESHFILE] Failed to open file for reading: " << filepath << std::endl;
			return false;
		}
		
		meshes.clear();
		
		// Проверяем магические байты
		char magic[4];
		file.read(magic, 4);
		if (magic[0] != 'M' || magic[1] != 'E' || magic[2] != 'S' || magic[3] != 'H') {
			std::cerr << "[MESHFILE] Invalid file format: " << filepath << std::endl;
			file.close();
			return false;
		}
		
		// Версия
		int32_t version;
		if (!readInt32(file, version)) {
			file.close();
			return false;
		}
		
		// Количество мешей
		uint32_t meshCount;
		if (!readUInt32(file, meshCount)) {
			file.close();
			return false;
		}
		
		meshes.resize(meshCount);
		
		// Читаем каждый меш
		for (uint32_t m = 0; m < meshCount; m++) {
			MeshData& meshData = meshes[m];
			
			// Метаданные
			if (!readInt32(file, meshData.worldPosition.x) ||
			    !readInt32(file, meshData.worldPosition.y) ||
			    !readInt32(file, meshData.worldPosition.z) ||
			    !readInt32(file, meshData.updateTime)) {
				file.close();
				return false;
			}
			
			// Вершины
			uint32_t vertexCount;
			if (!readUInt32(file, vertexCount)) {
				file.close();
				return false;
			}
			meshData.vertices.resize(vertexCount);
			for (uint32_t i = 0; i < vertexCount; i++) {
				int16_t x, y, z;
				file.read(reinterpret_cast<char*>(&x), sizeof(int16_t));
				file.read(reinterpret_cast<char*>(&y), sizeof(int16_t));
				file.read(reinterpret_cast<char*>(&z), sizeof(int16_t));
				meshData.vertices[i] = glm::vec3(
					static_cast<float>(x) / 100.0f,
					static_cast<float>(y) / 100.0f,
					static_cast<float>(z) / 100.0f
				);
			}
			
			// Нормали
			uint32_t normalCount;
			readUInt32(file, normalCount);
			meshData.normals.resize(normalCount);
			for (uint32_t i = 0; i < normalCount; i++) {
				readFloat(file, meshData.normals[i].x);
				readFloat(file, meshData.normals[i].y);
				readFloat(file, meshData.normals[i].z);
			}
			
			// UV
			uint32_t uvCount;
			if (!readUInt32(file, uvCount)) {
				file.close();
				return false;
			}
			meshData.uvs.resize(uvCount);
			for (uint32_t i = 0; i < uvCount; i++) {
				uint16_t u, v;
				file.read(reinterpret_cast<char*>(&u), sizeof(uint16_t));
				file.read(reinterpret_cast<char*>(&v), sizeof(uint16_t));
				meshData.uvs[i] = glm::vec2(
					static_cast<float>(u) / 10000.0f,
					static_cast<float>(v) / 10000.0f
				);
			}
			
			// Цвета
			uint32_t colorCount;
			if (!readUInt32(file, colorCount)) {
				file.close();
				return false;
			}
			meshData.colors.resize(colorCount);
			for (uint32_t i = 0; i < colorCount; i++) {
				uint16_t r, g, b, a;
				file.read(reinterpret_cast<char*>(&r), sizeof(uint16_t));
				file.read(reinterpret_cast<char*>(&g), sizeof(uint16_t));
				file.read(reinterpret_cast<char*>(&b), sizeof(uint16_t));
				file.read(reinterpret_cast<char*>(&a), sizeof(uint16_t));
				meshData.colors[i] = glm::vec4(
					static_cast<float>(r) / 10000.0f,
					static_cast<float>(g) / 10000.0f,
					static_cast<float>(b) / 10000.0f,
					static_cast<float>(a) / 10000.0f
				);
			}
			
			// Индексы
			uint32_t indexCount;
			if (!readUInt32(file, indexCount)) {
				file.close();
				return false;
			}
			bool use16Bit;
			file.read(reinterpret_cast<char*>(&use16Bit), sizeof(bool));
			meshData.indices.resize(indexCount);
			for (uint32_t i = 0; i < indexCount; i++) {
				if (use16Bit) {
					uint16_t idx;
					file.read(reinterpret_cast<char*>(&idx), sizeof(uint16_t));
					meshData.indices[i] = idx;
				} else {
					file.read(reinterpret_cast<char*>(&meshData.indices[i]), sizeof(uint32_t));
				}
			}
		}
		
		file.close();
		return true;
	}
	
	void prepareMeshData(const MeshData& data, std::vector<float>& vertexBuffer, std::vector<uint32_t>& indexBuffer) {
		vertexBuffer.clear();
		indexBuffer = data.indices;
		
		// Формируем буфер вершин: position(3) + normal(3) + uv(2) + color(4) = 12 floats на вершину
		const int floatsPerVertex = 12;
		vertexBuffer.reserve(data.vertices.size() * floatsPerVertex);
		
		for (size_t i = 0; i < data.vertices.size(); i++) {
			// Position
			vertexBuffer.push_back(data.vertices[i].x);
			vertexBuffer.push_back(data.vertices[i].y);
			vertexBuffer.push_back(data.vertices[i].z);
			
			// Normal
			if (i < data.normals.size()) {
				vertexBuffer.push_back(data.normals[i].x);
				vertexBuffer.push_back(data.normals[i].y);
				vertexBuffer.push_back(data.normals[i].z);
			} else {
				vertexBuffer.push_back(0.0f);
				vertexBuffer.push_back(1.0f);
				vertexBuffer.push_back(0.0f);
			}
			
			// UV
			if (i < data.uvs.size()) {
				vertexBuffer.push_back(data.uvs[i].x);
				vertexBuffer.push_back(data.uvs[i].y);
			} else {
				vertexBuffer.push_back(0.0f);
				vertexBuffer.push_back(0.0f);
			}
			
			// Color
			if (i < data.colors.size()) {
				vertexBuffer.push_back(data.colors[i].r);
				vertexBuffer.push_back(data.colors[i].g);
				vertexBuffer.push_back(data.colors[i].b);
				vertexBuffer.push_back(data.colors[i].a);
			} else {
				vertexBuffer.push_back(1.0f);
				vertexBuffer.push_back(1.0f);
				vertexBuffer.push_back(1.0f);
				vertexBuffer.push_back(1.0f);
			}
		}
	}
	
} // namespace MeshFile

