#pragma once

#include <cstdint>
#include <unordered_map>
#include <glm/glm.hpp>

class ChunkManager;
class Camera;

namespace lighting {
    // Система освещения для вокселей
    class LightingSystem {
    public:
        LightingSystem();
        ~LightingSystem();
        
        // Инициализация системы освещения
        void initialize(ChunkManager* chunkManager);
        
        // Обновление освещения вокруг позиции
        void updateLighting(const glm::vec3& position, int radius);
        
        // Получить освещение для блока
        float getLight(int x, int y, int z) const;
        
        // Получить сжатый свет для блока (публичный метод)
        float getCompressedLight(int x, int y, int z) const;
        
        // Получить освещение для грани блока с учетом нормали
        float getFaceLight(int x, int y, int z, const glm::vec3& faceNormal) const;
        
        // Установить освещение для блока
        void setLight(int x, int y, int z, uint8_t blockLight, uint8_t skyLight);
        
        // Вычисление освещения на основе соседних блоков
        void calculateLighting(int x, int y, int z);
        
        // Параметры освещения
        void setSkyLightColor(const glm::vec3& color) { skyLightColor = color; }
        void setSunColor(const glm::vec3& color) { sunColor = color; }
        void setGamma(float g) { gamma = g; }
        void setSunDirection(const glm::vec3& dir) { sunDirection = glm::normalize(dir); }
        
        const glm::vec3& getSkyLightColor() const { return skyLightColor; }
        const glm::vec3& getSunColor() const { return sunColor; }
        float getGamma() const { return gamma; }
        const glm::vec3& getSunDirection() const { return sunDirection; }
        
    private:
        ChunkManager* chunkManager;
        
        // Хранилище освещения (x, y, z) -> compressed light
        std::unordered_map<uint64_t, float> lightMap;
        
        // Параметры освещения
        glm::vec3 skyLightColor = glm::vec3(0.6f, 0.75f, 0.9f);  // Небесный свет (ambient)
        glm::vec3 sunColor = glm::vec3(1.0f, 0.95f, 0.85f);      // Цвет солнца
        float gamma = 2.2f;  // Гамма-коррекция
        glm::vec3 sunDirection = glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f)); // Направление солнца (прямо вверх)
        
        // Хеш-функция для координат
        uint64_t hashCoords(int x, int y, int z) const {
            return ((uint64_t)(x & 0xFFFF) << 32) | ((uint64_t)(y & 0xFFFF) << 16) | (uint64_t)(z & 0xFFFF);
        }
    };
}
