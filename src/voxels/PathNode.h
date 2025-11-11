#ifndef VOXELS_PATHNODE_H_
#define VOXELS_PATHNODE_H_

#include <glm/glm.hpp>

// Узел для A* pathfinding
// Адаптировано из 7 Days To Die PathNode
namespace Pathfinding {
    struct PathNode {
        glm::ivec2 position;  // Позиция узла в сетке pathfinding
        float pathCost;       // Стоимость пути до этого узла
        PathNode* next;       // Следующий узел в пути (для реконструкции пути)
        PathNode* nextListElem; // Следующий элемент в списке пула (для PathNodePool)
        
        PathNode() : position(0), pathCost(0.0f), next(nullptr), nextListElem(nullptr) {}
        
        PathNode(const glm::ivec2& pos, float cost, PathNode* n)
            : position(pos), pathCost(cost), next(n), nextListElem(nullptr) {}
        
        void Set(const glm::ivec2& pos, float cost, PathNode* n) {
            position = pos;
            pathCost = cost;
            next = n;
        }
        
        void Reset() {
            next = nullptr;
            nextListElem = nullptr;
        }
    };
}

#endif /* VOXELS_PATHNODE_H_ */

