#include "lighting/LightingSystem.h"
#include "../voxels/ChunkManager.h"
#include "../voxels/MCChunk.h"
#include "../voxels/voxel.h"
#include "lighting/Lighting.h"
#include <algorithm>
#include <cmath>

namespace lighting {

LightingSystem::LightingSystem() : chunkManager(nullptr) {
}

LightingSystem::~LightingSystem() {
}

void LightingSystem::initialize(ChunkManager* cm) {
    chunkManager = cm;
}

void LightingSystem::updateLighting(const glm::vec3& position, int radius) {
    if (chunkManager == nullptr) {
        return;
    }
    
    // Вычисляем освещение для блоков вокруг позиции
    int px = (int)std::floor(position.x);
    int py = (int)std::floor(position.y);
    int pz = (int)std::floor(position.z);
    
    for (int y = py - radius; y <= py + radius; y++) {
        for (int z = pz - radius; z <= pz + radius; z++) {
            for (int x = px - radius; x <= px + radius; x++) {
                calculateLighting(x, y, z);
            }
        }
    }
}

float LightingSystem::getCompressedLight(int x, int y, int z) const {
    uint64_t key = hashCoords(x, y, z);
    auto it = lightMap.find(key);
    if (it != lightMap.end()) {
        return it->second;
    }
    
    // Если освещение не найдено, вычисляем его
    // По умолчанию: небесный свет зависит от высоты
    uint8_t skyLight = 15; // Максимальный небесный свет
    if (y < 64) {
        skyLight = std::max(0, 15 - (64 - y) / 4); // Уменьшаем небесный свет с глубиной
    }
    
    uint8_t blockLight = 0; // Блоковое освещение по умолчанию
    
    return lighting::compressLight(blockLight, blockLight, blockLight, skyLight);
}

float LightingSystem::getLight(int x, int y, int z) const {
    return getCompressedLight(x, y, z);
}

void LightingSystem::setLight(int x, int y, int z, uint8_t blockLight, uint8_t skyLight) {
    uint64_t key = hashCoords(x, y, z);
    float compressed = lighting::compressLight(blockLight, blockLight, blockLight, skyLight);
    lightMap[key] = compressed;
}

void LightingSystem::calculateLighting(int x, int y, int z) {
    if (chunkManager == nullptr) {
        return;
    }
    
    voxel* vox = chunkManager->getVoxel(x, y, z);
    if (vox == nullptr) {
        return;
    }
    
    // Вычисляем небесный свет (зависит от высоты)
    uint8_t skyLight = 15; // Максимальный небесный свет
    if (y < 64) {
        skyLight = std::max(0, 15 - (64 - y) / 4); // Уменьшаем небесный свет с глубиной
    }
    
    // Вычисляем блоковое освещение на основе соседних блоков
    uint8_t blockLight = 0;
    
    // Проверяем текущий блок - если это лампа (id=2), он сам является источником света
    if (vox->id == 2) {
        // Лампа излучает максимальное блоковое освещение
        blockLight = 15;
    } else if (vox->id == 0) {
        // Для прозрачных блоков (воздух) проверяем соседние блоки на наличие источников света
        int directions[6][3] = {
            {0, 1, 0},  // Вверх
            {0, -1, 0}, // Вниз
            {1, 0, 0},  // Вправо
            {-1, 0, 0}, // Влево
            {0, 0, 1},  // Вперед
            {0, 0, -1}  // Назад
        };
        
        for (int i = 0; i < 6; i++) {
            int nx = x + directions[i][0];
            int ny = y + directions[i][1];
            int nz = z + directions[i][2];
            
            voxel* neighbor = chunkManager->getVoxel(nx, ny, nz);
            if (neighbor != nullptr && neighbor->id == 2) {
                // Соседний блок - лампа, распространяет свет
                // Свет уменьшается с расстоянием (пока просто максимальное)
                blockLight = std::max(blockLight, (uint8_t)15);
            }
        }
    }
    // Для непрозрачных блоков (id != 0 и id != 2) блоковое освещение = 0
    
    // Сохраняем освещение
    setLight(x, y, z, blockLight, skyLight);
}

float LightingSystem::getFaceLight(int x, int y, int z, const glm::vec3& faceNormal) const {
    // Получаем базовое освещение для блока
    float compressed = getCompressedLight(x, y, z);
    
    // Распаковываем освещение
    uint32_t comp = *reinterpret_cast<const uint32_t*>(&compressed);
    uint8_t blockLight = (comp >> 24) & 0xFF;
    uint8_t skyLight = comp & 0xFF;
    
    // Нормализуем значения освещения (0-15) в диапазон (0-1)
    float block = blockLight / 15.0f;
    float sky = skyLight / 15.0f;
    
    // Вычисляем направленное освещение от солнца
    // Верхние грани (normal.y > 0) получают больше света
    float sunFactor = glm::max(0.0f, glm::dot(faceNormal, sunDirection));
    // Для верхних граней солнце дает максимальное освещение
    // Для боковых граней - меньше, для нижних - минимальное
    sunFactor = glm::pow(sunFactor, 0.7f); // Смягчаем переход, но не слишком сильно
    
    // Применяем затенение граней (Minecraft-style)
    // Верхняя грань: 1.0, боковые: 0.85, нижняя: 0.5
    float faceShade = 1.0f;
    if (faceNormal.y > 0.5f) {
        // Верхняя грань - самая светлая, получает полное освещение от солнца
        faceShade = 1.0f;
    } else if (faceNormal.y < -0.5f) {
        // Нижняя грань - самая темная
        faceShade = 0.5f;
    } else {
        // Боковые грани - средняя яркость
        faceShade = 0.85f;
    }
    
    // Комбинируем освещение:
    // - Блоковое освещение (равномерное, слабое влияние)
    // - Небесное освещение (основной источник)
    // - Направленное освещение от солнца (для верхних граней)
    // - Затенение граней (Minecraft-style)
    
    // Базовое освещение от неба (ambient)
    float baseLight = sky * 0.4f;
    
    // Добавляем сильное освещение от солнца
    // Для верхних граней (sunFactor близок к 1.0) это дает яркое освещение
    // Для верхних граней: sunFactor = 1.0, для боковых: ~0.0, для нижних: 0.0
    float sunLight = sunFactor * sky * 1.2f; // Очень сильное влияние солнца на верхние грани
    
    // Комбинируем базовое и солнечное освещение
    baseLight = baseLight + sunLight;
    
    // Для верхних граней добавляем дополнительную яркость
    if (faceNormal.y > 0.5f) {
        baseLight += 0.2f; // Дополнительная яркость для верхних граней
    }
    
    // Добавляем блоковое освещение (если есть источники света)
    baseLight = glm::max(baseLight, block * 0.6f);
    
    // Применяем затенение граней (Minecraft-style)
    // Это создает визуальное различие между гранями
    float light = baseLight * faceShade;
    
    // Ограничиваем диапазон и добавляем минимальное освещение
    // Минимум 0.4 для видимости, максимум 1.0
    // Это гарантирует, что блоки всегда видны, даже в тени
    light = glm::clamp(light, 0.4f, 1.0f);
    
    // Возвращаем как сжатый свет (используем light для всех каналов)
    uint8_t lightValue = (uint8_t)(light * 15.0f);
    return lighting::compressLight(lightValue, lightValue, lightValue, skyLight);
}

}

