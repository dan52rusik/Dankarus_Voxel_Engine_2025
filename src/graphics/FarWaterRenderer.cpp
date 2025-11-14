#include "FarWaterRenderer.h"
#include "Mesh.h"
#include "Shader.h"
#include "../voxels/MCChunk.h"   // ради CHUNK_SIZE, если нужно
#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace glm;

// Те же атрибуты, что и у воды (из WaterRenderer.cpp)
extern int water_attrs[];

FarWaterRenderer::FarWaterRenderer()
    : mesh(nullptr)
{
    // Огромный квадрат, центр в (0, 0, 0), размер ~ 40кх40к метров
    // По Y будем двигать в render()
    const float SIZE = 20000.0f;
    
    // position(3) + normal(3) + tex(2) + alpha(1) = 9
    std::vector<float> buf;
    buf.reserve(6 * 9);
    
    auto pushVert = [&buf](float x, float y, float z, float u, float v) {
        // позиция
        buf.push_back(x);
        buf.push_back(y);
        buf.push_back(z);
        // нормаль вверх
        buf.push_back(0.0f);
        buf.push_back(1.0f);
        buf.push_back(0.0f);
        // текстура по XZ
        buf.push_back(u);
        buf.push_back(v);
        // альфа (чуть прозрачная)
        buf.push_back(0.8f);
    };
    
    // Два треугольника для создания квадрата
    //   (-SIZE, 0, -SIZE) ---- ( SIZE, 0, -SIZE)
    //          |               /
    //          |             /
    //   (-SIZE, 0,  SIZE) ---- ( SIZE, 0,  SIZE)
    pushVert(-SIZE, 0.0f, -SIZE, 0.0f, 0.0f);
    pushVert( SIZE, 0.0f, -SIZE, 1.0f, 0.0f);
    pushVert( SIZE, 0.0f,  SIZE, 1.0f, 1.0f);
    pushVert(-SIZE, 0.0f, -SIZE, 0.0f, 0.0f);
    pushVert( SIZE, 0.0f,  SIZE, 1.0f, 1.0f);
    pushVert(-SIZE, 0.0f,  SIZE, 0.0f, 1.0f);
    
    size_t vertexCount = buf.size() / 9;
    mesh = new Mesh(buf.data(), vertexCount, water_attrs);
}

FarWaterRenderer::~FarWaterRenderer() {
    delete mesh;
}

void FarWaterRenderer::render(float waterLevel, Shader* waterShader) {
    if (!mesh || !waterShader) return;
    
    // ИСПРАВЛЕНО: устанавливаем модель-матрицу внутри render
    // Поднимаем плоскость на уровень воды
    mat4 model(1.0f);
    model = translate(model, vec3(0.0f, waterLevel, 0.0f));
    waterShader->uniformMatrix("model", model);
    
    // Рендерим меш
    mesh->draw(GL_TRIANGLES);
}

