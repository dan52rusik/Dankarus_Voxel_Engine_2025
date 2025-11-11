#ifndef GRAPHICS_MESHPOOL_H_
#define GRAPHICS_MESHPOOL_H_

#include "Mesh.h"
#include <queue>
#include <mutex>
#include <memory>

// Система пула объектов для переиспользования мешей
// Адаптировано из 7 Days To Die DynamicMeshFile
namespace MeshPool {
	// Пул для обычных мешей
	class MeshPool {
	private:
		std::queue<Mesh*> availableMeshes;
		mutable std::mutex mutex; // mutable для использования в const методах
		size_t maxPoolSize;
		
	public:
		MeshPool(size_t maxSize = 100) : maxPoolSize(maxSize) {}
		~MeshPool() {
			clear();
		}
		
		// Получить меш из пула или создать новый
		Mesh* acquire();
		
		// Вернуть меш в пул
		void release(Mesh* mesh);
		
		// Очистить пул
		void clear();
		
		// Получить размер пула
		size_t size() const {
			std::lock_guard<std::mutex> lock(mutex);
			return availableMeshes.size();
		}
	};
	
	// Глобальные пулы (thread-safe)
	MeshPool& getMeshPool();
	
	// Утилиты для работы с пулом
	Mesh* getMeshFromPool();
	void returnMeshToPool(Mesh* mesh);
	void clearMeshPool();
}

#endif /* GRAPHICS_MESHPOOL_H_ */

