#include "MeshPool.h"
#include <iostream>

namespace MeshPool {
	
	// Глобальный пул мешей (thread-safe)
	static MeshPool* g_meshPool = nullptr;
	static std::mutex g_poolMutex;
	
	MeshPool& getMeshPool() {
		std::lock_guard<std::mutex> lock(g_poolMutex);
		if (g_meshPool == nullptr) {
			g_meshPool = new MeshPool(100); // Максимум 100 мешей в пуле
		}
		return *g_meshPool;
	}
	
	Mesh* MeshPool::acquire() {
		std::lock_guard<std::mutex> lock(mutex);
		
		if (!availableMeshes.empty()) {
			Mesh* mesh = availableMeshes.front();
			availableMeshes.pop();
			return mesh;
		}
		
		// Если пул пуст, возвращаем nullptr
		// Вызывающий код должен создать меш самостоятельно через конструктор Mesh
		return nullptr;
	}
	
	void MeshPool::release(Mesh* mesh) {
		if (mesh == nullptr) {
			return;
		}
		
		std::lock_guard<std::mutex> lock(mutex);
		
		// Проверяем, не превышен ли лимит пула
		if (availableMeshes.size() >= maxPoolSize) {
			// Удаляем старый меш, если пул переполнен
			delete mesh;
			return;
		}
		
		// Возвращаем меш в пул
		availableMeshes.push(mesh);
	}
	
	void MeshPool::clear() {
		std::lock_guard<std::mutex> lock(mutex);
		while (!availableMeshes.empty()) {
			Mesh* mesh = availableMeshes.front();
			availableMeshes.pop();
			delete mesh;
		}
	}
	
	// Утилиты для работы с глобальным пулом
	Mesh* getMeshFromPool() {
		return getMeshPool().acquire();
	}
	
	void returnMeshToPool(Mesh* mesh) {
		getMeshPool().release(mesh);
	}
	
	void clearMeshPool() {
		std::lock_guard<std::mutex> lock(g_poolMutex);
		if (g_meshPool != nullptr) {
			g_meshPool->clear();
		}
	}
	
} // namespace MeshPool

