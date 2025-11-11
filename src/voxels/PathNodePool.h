#ifndef VOXELS_PATHNODEPOOL_H_
#define VOXELS_PATHNODEPOOL_H_

#include "PathNode.h"
#include <vector>

// Пул узлов для pathfinding (объектный пул для оптимизации)
// Адаптировано из 7 Days To Die PathNodePool
namespace Pathfinding {
    class PathNodePool {
    private:
        std::vector<PathNode*> pool;  // Пул узлов
        size_t used;                   // Количество используемых узлов
        
    public:
        PathNodePool(size_t initialSize = 100000) : used(0) {
            pool.reserve(initialSize);
        }
        
        ~PathNodePool() {
            for (PathNode* node : pool) {
                delete node;
            }
        }
        
        // Выделить узел из пула
        PathNode* Alloc() {
            PathNode* node;
            if (used >= pool.size()) {
                // Создаем новый узел, если пул исчерпан
                node = new PathNode();
                pool.push_back(node);
            } else {
                // Переиспользуем существующий узел
                node = pool[used];
            }
            used++;
            return node;
        }
        
        // Вернуть все узлы в пул (сброс счетчика)
        void ReturnAll() {
            for (size_t i = 0; i < used; i++) {
                pool[i]->Reset();
            }
            used = 0;
        }
        
        // Очистить пул
        void Cleanup() {
            ReturnAll();
            pool.clear();
            pool.reserve(16); // Минимальная емкость
        }
        
        // Получить статистику
        size_t GetCapacity() const { return pool.capacity(); }
        size_t GetAllocated() const { return pool.size(); }
        size_t GetInUse() const { return used; }
    };
}

#endif /* VOXELS_PATHNODEPOOL_H_ */

