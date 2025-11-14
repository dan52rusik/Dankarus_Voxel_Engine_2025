#pragma once

#include "Mesh.h"
#include <glm/glm.hpp>

class Shader;

class FarWaterRenderer {
public:
    FarWaterRenderer();
    ~FarWaterRenderer();
    
    // Вызывать каждый кадр с текущим уровнем воды и шейдером
    // Шейдер должен быть уже активирован (use()) и иметь установленные view/projection
    void render(float waterLevel, Shader* waterShader);
    
private:
    Mesh* mesh;
};

