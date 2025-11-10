#include "lighting/Lighting.h"
#include <algorithm>
#include <cmath>

namespace lighting {
    float calculateLight(uint8_t blockLight, uint8_t skyLight) {
        // Нормализуем значения освещения (0-15) в диапазон (0-1)
        float block = blockLight / 15.0f;
        float sky = skyLight / 15.0f;
        
        // Комбинируем освещение (берем максимум)
        float light = std::max(block, sky);
        
        return light;
    }
    
    glm::vec3 calculateLightWithTorch(
        float compressedLight,
        const glm::vec3& position,
        const glm::vec3& cameraPos,
        const glm::vec3& torchlightColor,
        float torchlightDistance,
        const glm::vec3& skyLightColor,
        float gamma
    ) {
        // Распаковываем сжатый свет
        glm::vec4 decomp = decompressLight(compressedLight);
        
        // RGB - блоковое освещение, A - небесное освещение
        glm::vec3 blockLight = glm::vec3(decomp.r, decomp.g, decomp.b);
        float skyLightValue = decomp.a;
        
        // Вычисляем факел (torchlight) от камеры
        float distance = glm::length(cameraPos - position);
        float torchlight = std::max(0.0f, 1.0f - distance / torchlightDistance);
        
        // Комбинируем освещение
        glm::vec3 light = blockLight;
        light += torchlight * torchlightColor;
        
        // Применяем небесный свет как минимум
        glm::vec3 skyLight = skyLightColor * skyLightValue;
        light = glm::max(light, skyLight);
        
        // Гамма-коррекция
        light = glm::pow(glm::clamp(light, 0.0f, 1.0f), glm::vec3(gamma));
        
        return light;
    }
}

