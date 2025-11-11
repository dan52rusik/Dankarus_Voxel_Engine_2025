#ifndef GRAPHICS_MESHFILE_H_
#define GRAPHICS_MESHFILE_H_

#include "Mesh.h"
#include "MeshBounds.h"
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <cstdint>

// Система сохранения/загрузки мешей на диск
// Адаптировано из 7 Days To Die DynamicMeshFile
namespace MeshFile {
	
	// Результат загрузки меша
	enum class LoadResult {
		Ok,           // Успешно загружено
		WrongSize,    // Неверный размер файла
		Error,        // Ошибка чтения
		Empty         // Пустой меш
	};
	
	// Структура для данных меша (для сохранения)
	struct MeshData {
		std::vector<glm::vec3> vertices;
		std::vector<glm::vec3> normals;
		std::vector<glm::vec2> uvs;
		std::vector<glm::vec2> uvs2; // Дополнительный UV канал (для трещин/детализации)
		std::vector<glm::vec2> uvs3; // Дополнительный UV канал
		std::vector<glm::vec2> uvs4; // Дополнительный UV канал
		std::vector<glm::vec4> tangents; // Тангенсы (для нормального маппинга)
		std::vector<glm::vec4> colors; // RGBA
		std::vector<uint32_t> indices; // Индексы треугольников
		
		// Метаданные
		glm::ivec3 worldPosition; // Позиция чанка в мире
		int updateTime; // Время последнего обновления
		
		// Очистить все данные
		void clear() {
			vertices.clear();
			normals.clear();
			uvs.clear();
			uvs2.clear();
			uvs3.clear();
			uvs4.clear();
			tangents.clear();
			colors.clear();
			indices.clear();
		}
		
		// Проверить валидность данных
		bool isValid() const {
			return !vertices.empty() && 
			       vertices.size() == uvs.size() &&
			       vertices.size() == colors.size() &&
			       indices.size() % 3 == 0; // Должно быть кратно 3 (треугольники)
		}
		
		// Вычислить размер меша в байтах для сохранения на диск
		// Адаптировано из 7 Days To Die DynamicMeshVoxel::GetByteLength
		size_t getByteLength() const {
			if (vertices.empty()) {
				return 0;
			}
			
			const size_t MAX_16BIT = 65535;
			bool use32BitIndices = indices.size() > MAX_16BIT;
			size_t indexSize = use32BitIndices ? 4 : 2;
			
			// Формат: магические байты(4) + версия(4) + метаданные(16) + 
			//         vertexCount(4) + vertices(vertexCount * 6) +
			//         normalCount(4) + normals(normalCount * 12) +
			//         uvCount(4) + uvs(uvCount * 4) +
			//         colorCount(4) + colors(colorCount * 8) +
			//         indexCount(4) + use16Bit(1) + indices(indexCount * indexSize)
			
			size_t size = 4 + 4 + 16; // Магические байты + версия + метаданные
			size += 4 + vertices.size() * 6; // Вершины (int16 * 3)
			size += 4 + normals.size() * 12; // Нормали (float * 3)
			size += 4 + uvs.size() * 4; // UV (uint16 * 2)
			size += 4 + colors.size() * 8; // Цвета (uint16 * 4)
			size += 4 + 1 + indices.size() * indexSize; // Индексы
			
			return size;
		}
	};
	
	// Сохранить меш в бинарный файл
	// Формат: версия(4), количество вершин(4), vertices, normals, uvs, colors, indices
	bool saveMesh(const std::string& filepath, const MeshData& meshData);
	
	// Загрузить меш из бинарного файла
	// Возвращает LoadResult для детальной информации об ошибках
	LoadResult loadMesh(const std::string& filepath, MeshData& meshData);
	
	// Загрузить меш из бинарного файла (старый API, возвращает bool)
	bool loadMeshBool(const std::string& filepath, MeshData& meshData);
	
	// Сохранить несколько мешей в один файл (для региона)
	bool saveMeshes(const std::string& filepath, const std::vector<MeshData>& meshes);
	
	// Загрузить несколько мешей из файла
	bool loadMeshes(const std::string& filepath, std::vector<MeshData>& meshes);
	
	// Конвертировать MeshData в Mesh (для отрисовки)
	// Примечание: требует создания Mesh через конструктор с параметрами
	// Эта функция только подготавливает данные
	void prepareMeshData(const MeshData& data, std::vector<float>& vertexBuffer, std::vector<uint32_t>& indexBuffer);
	
} // namespace MeshFile

#endif /* GRAPHICS_MESHFILE_H_ */

