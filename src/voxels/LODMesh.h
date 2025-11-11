#ifndef VOXELS_LODMESH_H_
#define VOXELS_LODMESH_H_

#include <vector>
#include <glm/glm.hpp>
#include <cstdint>

// Forward declaration
class Mesh;

// Система LOD (Level of Detail) для дальних чанков
// Адаптировано из 7 Days To Die DChunkSquareMesh
namespace LODMesh {
	// ==================== Константы ====================
	
	// Максимальное количество уровней детализации
	static constexpr int MaxLODLevels = 4;
	
	// Типичные разрешения для разных LOD уровней
	// LOD 0 = максимальная детализация, LOD 3 = минимальная
	static constexpr int LODResolution[] = {32, 16, 8, 4};
	
	// Разрешение коллайдера (обычно меньше визуального)
	static constexpr int DefaultColliderResolution = 8;
	
	// ==================== Структуры данных ====================
	
	// Данные меша для дальнего чанка с LOD
	// Адаптировано из 7 Days To Die DChunkSquareMesh
	struct ChunkSquareMesh {
		// Визуальные данные поверхности (Marching Cubes)
		std::vector<glm::vec3> vertices;
		std::vector<glm::vec3> normals;
		std::vector<glm::vec4> tangents;
		std::vector<glm::vec3> edgeCornerNormals; // Нормали углов для сглаживания краев (Resolution * 4)
		std::vector<glm::vec4> colors;            // Цвета вершин (RGBA)
		std::vector<int> textureIds;              // ID текстур для каждой вершины
		std::vector<uint32_t> indices;           // Индексы треугольников
		
		// Флаги вершин
		std::vector<bool> isWater;                // Флаг воды для каждой вершины
		
		// Воксельный меш (для кубических блоков, отдельно от поверхности)
		class Mesh* voxelMesh;                    // Указатель на меш воксельных блоков (аналог VoxelMeshTerrain)
		
		// Коллайдер (низкое разрешение для физики)
		std::vector<glm::vec3> colliderVertices;
		std::vector<float> colliderVerticesHeight;
		
		// Метаданные
		int lodLevel;                            // Уровень детализации (0 = максимальный)
		int resolution;                          // Разрешение меша (количество вершин по одной стороне)
		int colliderResolution;                  // Разрешение коллайдера
		glm::vec3 chunkPosition;                 // Позиция чанка в мире
		glm::vec3 boundsMin;                     // Минимальные границы
		glm::vec3 boundsMax;                     // Максимальные границы
		int waterPlaneBlockId;                   // ID блока воды (если есть)
		
		// Конструктор
		ChunkSquareMesh() 
			: lodLevel(0), resolution(32), colliderResolution(DefaultColliderResolution),
			  waterPlaneBlockId(0), voxelMesh(nullptr) {
		}
		
		// Деструктор (не удаляет voxelMesh, т.к. он управляется извне)
		~ChunkSquareMesh() {
			// voxelMesh не удаляем здесь, т.к. он может быть общим
		}
		
		// Инициализация с заданными параметрами
		void init(int nbVertices, int resLevel, int meshResolution, 
		          int colliderRes, int nbTriangles) {
			lodLevel = resLevel;
			resolution = meshResolution;
			colliderResolution = colliderRes;
			
			// Выделяем память
			vertices.resize(nbVertices);
			normals.resize(nbVertices);
			tangents.resize(nbVertices);
			colors.resize(nbVertices);
			textureIds.resize(nbVertices);
			isWater.resize(nbVertices);
			indices.resize(nbTriangles * 3);
			
			// Нормали углов (4 угла * разрешение)
			edgeCornerNormals.resize(resolution * 4);
			
			// Коллайдер
			int colliderSize = colliderRes * colliderRes;
			colliderVertices.resize(colliderSize);
			colliderVerticesHeight.resize(colliderSize);
		}
		
		// Вычислить границы меша (аналог Bounds в Unity/C#)
		// Использует MeshBounds::getBoundsFromVerts для универсальности
		void calculateBounds() {
			if (vertices.empty()) {
				boundsMin = boundsMax = glm::vec3(0.0f);
				return;
			}
			
			// Используем универсальную функцию из MeshBounds
			// (требует включения MeshBounds.h в .cpp файле)
			boundsMin = boundsMax = vertices[0];
			for (const auto& v : vertices) {
				boundsMin = glm::min(boundsMin, v);
				boundsMax = glm::max(boundsMax, v);
			}
		}
		
		// Получить центр границ (для совместимости с Bounds.center)
		glm::vec3 getBoundsCenter() const {
			return (boundsMin + boundsMax) * 0.5f;
		}
		
		// Получить размер границ (для совместимости с Bounds.size)
		glm::vec3 getBoundsSize() const {
			return boundsMax - boundsMin;
		}
		
		// Очистить все данные
		void clear() {
			vertices.clear();
			normals.clear();
			tangents.clear();
			edgeCornerNormals.clear();
			colors.clear();
			textureIds.clear();
			isWater.clear();
			indices.clear();
			colliderVertices.clear();
			colliderVerticesHeight.clear();
			voxelMesh = nullptr; // Не удаляем, только сбрасываем указатель
		}
		
		// Получить количество треугольников
		size_t getTriangleCount() const {
			return indices.size() / 3;
		}
		
		// Получить размер данных в байтах (приблизительно)
		size_t getMemorySize() const {
			size_t size = vertices.size() * sizeof(glm::vec3) +
			              normals.size() * sizeof(glm::vec3) +
			              tangents.size() * sizeof(glm::vec4) +
			              edgeCornerNormals.size() * sizeof(glm::vec3) +
			              colors.size() * sizeof(glm::vec4) +
			              textureIds.size() * sizeof(int) +
			              isWater.size() * sizeof(bool) +
			              indices.size() * sizeof(uint32_t) +
			              colliderVertices.size() * sizeof(glm::vec3) +
			              colliderVerticesHeight.size() * sizeof(float);
			// Примечание: voxelMesh не учитывается, т.к. он управляется отдельно
			return size;
		}
	};
	
	// ==================== Утилиты ====================
	
	// Получить разрешение для LOD уровня
	inline int getLODResolution(int lodLevel) {
		if (lodLevel >= 0 && lodLevel < MaxLODLevels) {
			return LODResolution[lodLevel];
		}
		return LODResolution[MaxLODLevels - 1]; // Минимальное разрешение
	}
	
	// Вычислить количество вершин для квадратного меша
	inline int calculateVertexCount(int resolution) {
		return (resolution + 1) * (resolution + 1);
	}
	
	// Вычислить количество треугольников для квадратного меша
	inline int calculateTriangleCount(int resolution) {
		return resolution * resolution * 2; // 2 треугольника на квадрат
	}
	
	// Определить LOD уровень на основе расстояния
	// distance - расстояние от камеры до чанка
	// lodDistances - пороги расстояний для каждого LOD уровня
	inline int determineLODLevel(float distance, const float* lodDistances, int maxLevels) {
		for (int i = 0; i < maxLevels; i++) {
			if (distance <= lodDistances[i]) {
				return i;
			}
		}
		return maxLevels - 1; // Максимальный LOD
	}
	
	// Вычислить упрощенный меш для LOD (downsampling)
	// source - исходный меш с высоким разрешением
	// target - целевой меш с низким разрешением
	// targetLOD - целевой уровень детализации
	void generateLODMesh(const ChunkSquareMesh& source, ChunkSquareMesh& target, int targetLOD);
	
	// Генерировать коллайдер из визуального меша
	// mesh - исходный визуальный меш
	// target - целевой меш, в который будет записан коллайдер
	// colliderRes - разрешение коллайдера
	void generateCollider(const ChunkSquareMesh& mesh, ChunkSquareMesh& target, int colliderRes);
}

#endif /* VOXELS_LODMESH_H_ */

