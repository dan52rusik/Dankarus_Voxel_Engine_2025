#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace lighting {
    // Сжатие света в один float (RGBA по 4 бита каждый)
    // R = block light (0-15), G = block light (0-15), B = block light (0-15), A = sky light (0-15)
    inline float compressLight(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        // Ограничиваем значения до 4 бит (0-15)
        r = r & 0xF;
        g = g & 0xF;
        b = b & 0xF;
        a = a & 0xF;
        uint32_t compressed = (r << 24) | (g << 16) | (b << 8) | a;
        return *reinterpret_cast<float*>(&compressed);
    }
    
    // Распаковка света из float
    inline glm::vec4 decompressLight(float compressed) {
        uint32_t comp = *reinterpret_cast<uint32_t*>(&compressed);
        return glm::vec4(
            ((comp >> 24) & 0xF) / 15.0f,
            ((comp >> 16) & 0xF) / 15.0f,
            ((comp >> 8) & 0xF) / 15.0f,
            (comp & 0xF) / 15.0f
        );
    }
    
    // Вычисление освещения для вокселя на основе соседних блоков
    // blockLight: освещение от блоков (0-15)
    // skyLight: небесное освещение (0-15)
    float calculateLight(uint8_t blockLight, uint8_t skyLight);
    
    // Вычисление освещения с учетом факела от камеры
    glm::vec3 calculateLightWithTorch(
        float compressedLight,
        const glm::vec3& position,
        const glm::vec3& cameraPos,
        const glm::vec3& torchlightColor,
        float torchlightDistance,
        const glm::vec3& skyLightColor,
        float gamma
    );
}

