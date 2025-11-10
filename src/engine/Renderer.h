#pragma once

#include <glm/glm.hpp>

class Shader;
class Batch2D;

class Renderer {
public:
    static void drawBlockOutline(Shader* linesShader, const glm::mat4& projview, 
                                 int blockX, int blockY, int blockZ, 
                                 const glm::vec3& faceNormal);
    
    static void drawCrosshair(Batch2D* batch, Shader* uiShader, 
                              int windowWidth, int windowHeight);
};

