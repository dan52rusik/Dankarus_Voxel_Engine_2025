#include "engine/Renderer.h"

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

#include "graphics/Shader.h"
#include "graphics/Mesh.h"
#include "graphics/Batch2D.h"
#include "window/Window.h"

using namespace glm;

void Renderer::drawBlockOutline(Shader* linesShader, const mat4& projview, 
                                 int blockX, int blockY, int blockZ, 
                                 const vec3& faceNormal) {
    // Цвет контура (белый с хорошей видимостью)
    const vec4 outlineColor(1.0f, 1.0f, 1.0f, 1.0f);
    
    // Воксельные блоки рендерятся с центром в целых координатах (blockX, blockY, blockZ)
    // Грани блока идут от (center - 0.5) до (center + 0.5)
    // Контур должен точно совпадать с гранями блока
    float x0 = (float)blockX - 0.5f;
    float x1 = (float)blockX + 0.5f;
    float y0 = (float)blockY - 0.5f;
    float y1 = (float)blockY + 0.5f;
    float z0 = (float)blockZ - 0.5f;
    float z1 = (float)blockZ + 0.5f;
    
    // Небольшое смещение наружу для избежания z-fighting
    const float epsilon = 0.001f;
    vec3 center((float)blockX, (float)blockY, (float)blockZ);
    
    // Функция для смещения вершины наружу от центра
    auto offsetVertex = [&center, epsilon](float x, float y, float z) -> vec3 {
        vec3 vertex(x, y, z);
        vec3 dir = normalize(vertex - center);
        return vertex + dir * epsilon;
    };
    
    // Вектор вершин для всех 12 ребер куба (wireframe)
    std::vector<float> vertices;
    vertices.reserve(24 * 7); // 24 вершины (12 ребер * 2 точки) * 7 компонентов
    
    auto addLine = [&vertices, &outlineColor, &offsetVertex](float x1, float y1, float z1, float x2, float y2, float z2) {
        // Смещаем вершины наружу от центра для избежания z-fighting
        vec3 v1 = offsetVertex(x1, y1, z1);
        vec3 v2 = offsetVertex(x2, y2, z2);
        
        // Первая точка линии
        vertices.push_back(v1.x);
        vertices.push_back(v1.y);
        vertices.push_back(v1.z);
        vertices.push_back(outlineColor.r);
        vertices.push_back(outlineColor.g);
        vertices.push_back(outlineColor.b);
        vertices.push_back(outlineColor.a);
        // Вторая точка линии
        vertices.push_back(v2.x);
        vertices.push_back(v2.y);
        vertices.push_back(v2.z);
        vertices.push_back(outlineColor.r);
        vertices.push_back(outlineColor.g);
        vertices.push_back(outlineColor.b);
        vertices.push_back(outlineColor.a);
    };
    
    // Рисуем все 12 ребер куба (wireframe)
    // Y - вертикальная ось (вверх)
    // Нижняя грань (y = y0)
    addLine(x0, y0, z0, x1, y0, z0); // переднее нижнее ребро (по X)
    addLine(x1, y0, z0, x1, y0, z1); // правое нижнее ребро (по Z)
    addLine(x1, y0, z1, x0, y0, z1); // заднее нижнее ребро (по X)
    addLine(x0, y0, z1, x0, y0, z0); // левое нижнее ребро (по Z)
    
    // Верхняя грань (y = y1)
    addLine(x0, y1, z0, x1, y1, z0); // переднее верхнее ребро (по X)
    addLine(x1, y1, z0, x1, y1, z1); // правое верхнее ребро (по Z)
    addLine(x1, y1, z1, x0, y1, z1); // заднее верхнее ребро (по X)
    addLine(x0, y1, z1, x0, y1, z0); // левое верхнее ребро (по Z)
    
    // Вертикальные ребра (по Y)
    addLine(x0, y0, z0, x0, y1, z0); // переднее левое
    addLine(x1, y0, z0, x1, y1, z0); // переднее правое
    addLine(x1, y0, z1, x1, y1, z1); // заднее правое
    addLine(x0, y0, z1, x0, y1, z1); // заднее левое
    
    if (vertices.empty()) {
        return; // Нечего рисовать
    }
    
    // Создаем временный Mesh для линий
    const int lineAttrs[] = {3, 4, 0}; // позиция (3), цвет (4)
    Mesh lineMesh(vertices.data(), vertices.size() / 7, lineAttrs);
    
    // Сохраняем текущее состояние OpenGL
    GLboolean oldCullFace;
    GLboolean oldDepthTest;
    GLboolean oldDepthMask;
    GLboolean oldBlend;
    GLint oldDepthFunc;
    GLfloat oldLineWidth;
    
    glGetBooleanv(GL_CULL_FACE, &oldCullFace);
    glGetBooleanv(GL_DEPTH_TEST, &oldDepthTest);
    // GL_DEPTH_WRITEMASK возвращает GLboolean через glGetBooleanv
    GLboolean depthWriteMask;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWriteMask);
    oldDepthMask = depthWriteMask;
    glGetBooleanv(GL_BLEND, &oldBlend);
    glGetIntegerv(GL_DEPTH_FUNC, &oldDepthFunc);
    glGetFloatv(GL_LINE_WIDTH, &oldLineWidth);
    
    // Настраиваем состояние для отрисовки линий
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE); // Не пишем в буфер глубины
    glDepthFunc(GL_LEQUAL); // Контур виден даже если он точно на грани
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(2.5f); // Толщина линии (немного толще для лучшей видимости)
    
    // Используем шейдер для линий
    linesShader->use();
    linesShader->uniformMatrix("u_projview", projview);
    
    // Рисуем линии
    lineMesh.draw(GL_LINES);
    
    // Восстанавливаем предыдущее состояние OpenGL
    glLineWidth(oldLineWidth);
    glDepthMask(oldDepthMask ? GL_TRUE : GL_FALSE);
    glDepthFunc(oldDepthFunc);
    if (oldBlend) glEnable(GL_BLEND);
    else glDisable(GL_BLEND);
    if (oldCullFace) glEnable(GL_CULL_FACE);
    else glDisable(GL_CULL_FACE);
    if (!oldDepthTest) glDisable(GL_DEPTH_TEST);
}

void Renderer::drawCrosshair(Batch2D* batch, Shader* uiShader, 
                              int windowWidth, int windowHeight) {
    // Настраиваем UI состояние
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    
    // Используем UI шейдер
    glActiveTexture(GL_TEXTURE0);
    uiShader->use();
    uiShader->uniform1i("u_texture", 0);
    
    // Устанавливаем ортографическую проекцию для UI
    // Используем framebuffer размеры для HiDPI
    int fbWidth = Window::fbWidth > 0 ? Window::fbWidth : windowWidth;
    int fbHeight = Window::fbHeight > 0 ? Window::fbHeight : windowHeight;
    mat4 uiProj = glm::ortho(0.0f, (float)fbWidth, (float)fbHeight, 0.0f);
    uiShader->uniformMatrix("u_projview", uiProj);
    
    // Начинаем батч
    batch->begin();
    
    // Размеры прицела
    float crosshairSize = 20.0f;  // Длина линий
    float crosshairThickness = 2.0f;  // Толщина линий
    float crosshairGap = 4.0f;  // Разрыв в центре
    
    // Цвет прицела (белый с небольшой прозрачностью для лучшей видимости)
    batch->color = vec4(1.0f, 1.0f, 1.0f, 0.8f);
    
    // Центр экрана (используем framebuffer размеры)
    float centerX = fbWidth / 2.0f;
    float centerY = fbHeight / 2.0f;
    
    // Вертикальная линия (верхняя часть)
    batch->rect(centerX - crosshairThickness / 2.0f, 
               centerY - crosshairSize / 2.0f - crosshairGap / 2.0f,
               crosshairThickness, 
               crosshairSize / 2.0f - crosshairGap / 2.0f);
    
    // Вертикальная линия (нижняя часть)
    batch->rect(centerX - crosshairThickness / 2.0f, 
               centerY + crosshairGap / 2.0f,
               crosshairThickness, 
               crosshairSize / 2.0f - crosshairGap / 2.0f);
    
    // Горизонтальная линия (левая часть)
    batch->rect(centerX - crosshairSize / 2.0f - crosshairGap / 2.0f,
               centerY - crosshairThickness / 2.0f,
               crosshairSize / 2.0f - crosshairGap / 2.0f,
               crosshairThickness);
    
    // Горизонтальная линия (правая часть)
    batch->rect(centerX + crosshairGap / 2.0f,
               centerY - crosshairThickness / 2.0f,
               crosshairSize / 2.0f - crosshairGap / 2.0f,
               crosshairThickness);
    
    // Рендерим батч
    batch->render();
    
    // Восстанавливаем состояние
    batch->color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

void Renderer::drawHotbar(Batch2D* batch, Shader* uiShader, 
                           int windowWidth, int windowHeight,
                           const int* hotbarSlots, int selectedSlot) {
    // Настраиваем UI состояние
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    
    // Используем UI шейдер
    glActiveTexture(GL_TEXTURE0);
    uiShader->use();
    uiShader->uniform1i("u_texture", 0);
    
    // Устанавливаем ортографическую проекцию для UI
    // Используем framebuffer размеры для HiDPI
    int fbWidth = Window::fbWidth > 0 ? Window::fbWidth : windowWidth;
    int fbHeight = Window::fbHeight > 0 ? Window::fbHeight : windowHeight;
    mat4 uiProj = glm::ortho(0.0f, (float)fbWidth, (float)fbHeight, 0.0f);
    uiShader->uniformMatrix("u_projview", uiProj);
    
    // Начинаем батч
    batch->begin();
    
    // Параметры hotbar
    const int HOTBAR_SIZE = 9;
    const float slotSize = 40.0f; // Размер одной ячейки
    const float slotGap = 4.0f; // Расстояние между ячейками
    const float hotbarWidth = HOTBAR_SIZE * slotSize + (HOTBAR_SIZE - 1) * slotGap;
    const float hotbarHeight = slotSize;
    const float hotbarX = (fbWidth - hotbarWidth) / 2.0f; // Центрируем по горизонтали
    const float hotbarY = fbHeight - hotbarHeight - 20.0f; // Отступ снизу
    
    // Цвета для ячеек
    vec4 slotBgColor = vec4(0.2f, 0.2f, 0.2f, 0.8f); // Темно-серая ячейка
    vec4 slotSelectedBgColor = vec4(0.4f, 0.4f, 0.4f, 0.9f); // Более светлая для выбранной
    vec4 slotBorderColor = vec4(0.5f, 0.5f, 0.5f, 1.0f); // Серая рамка
    vec4 slotSelectedBorderColor = vec4(1.0f, 1.0f, 1.0f, 1.0f); // Белая рамка для выбранной
    vec4 slotEmptyColor = vec4(0.15f, 0.15f, 0.15f, 0.6f); // Еще более темная для пустой
    
    // Рисуем все ячейки hotbar
    for (int i = 0; i < HOTBAR_SIZE; i++) {
        float x = hotbarX + i * (slotSize + slotGap);
        float y = hotbarY;
        bool isSelected = (i == selectedSlot);
        int blockId = hotbarSlots[i];
        bool isEmpty = (blockId == 0);
        
        // Цвет фона ячейки
        vec4 bgColor = isEmpty ? slotEmptyColor : (isSelected ? slotSelectedBgColor : slotBgColor);
        vec4 borderColor = isSelected ? slotSelectedBorderColor : slotBorderColor;
        float borderWidth = isSelected ? 3.0f : 2.0f;
        
        // Рисуем фон ячейки
        batch->color = bgColor;
        batch->rect(x, y, slotSize, slotSize);
        
        // Рисуем рамку ячейки (четыре прямоугольника)
        batch->color = borderColor;
        // Верхняя граница
        batch->rect(x, y, slotSize, borderWidth);
        // Нижняя граница
        batch->rect(x, y + slotSize - borderWidth, slotSize, borderWidth);
        // Левая граница (только средняя часть, чтобы не перекрывать углы)
        batch->rect(x, y + borderWidth, borderWidth, slotSize - borderWidth * 2);
        // Правая граница (только средняя часть, чтобы не перекрывать углы)
        batch->rect(x + slotSize - borderWidth, y + borderWidth, borderWidth, slotSize - borderWidth * 2);
        
        // Если ячейка не пустая, рисуем цвет блока внутри
        if (!isEmpty) {
            // Цвета блоков (такие же, как в voxel_frag.glsl)
            vec4 blockColor;
            switch (blockId) {
                case 1: // Серый камень
                    blockColor = vec4(0.6f, 0.6f, 0.6f, 1.0f);
                    break;
                case 2: // Лампа (желтый/оранжевый)
                    blockColor = vec4(1.0f, 0.95f, 0.85f, 1.0f);
                    break;
                case 3: // Коричневое дерево
                    blockColor = vec4(0.5f, 0.3f, 0.2f, 1.0f);
                    break;
                case 4: // Зеленый
                    blockColor = vec4(0.4f, 0.7f, 0.4f, 1.0f);
                    break;
                default:
                    blockColor = vec4(0.8f, 0.8f, 0.8f, 1.0f);
                    break;
            }
            
            // Рисуем цветной квадрат внутри ячейки (с отступами)
            float padding = 4.0f;
            batch->color = blockColor;
            batch->rect(x + padding, y + padding, slotSize - padding * 2, slotSize - padding * 2);
        }
    }
    
    // Рендерим батч
    batch->render();
    
    // Восстанавливаем состояние
    batch->color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

