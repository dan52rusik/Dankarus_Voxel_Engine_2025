#version 330 core

in vec4 a_color;
in vec2 a_texCoord;
in float a_distance;
flat in float a_blockId;
out vec4 f_color;

uniform sampler2D u_texture0;

void main(){
    // Временно отключаем текстуру для проверки - блоки будут одноцветными по blockId
    vec4 texColor = vec4(1.0); // texture(u_texture0, a_texCoord) - выключено для проверки
    
    // Цветовая модификация на основе типа блока
    vec3 blockColor = vec3(1.0);
    int blockId = int(a_blockId);
    
    if (blockId == 1) {
        // Блок 1 - серый камень
        blockColor = vec3(0.6, 0.6, 0.6);
    } else if (blockId == 2) {
        // Блок 2 - лампа (желтый/оранжевый)
        blockColor = vec3(1.0, 0.9, 0.7);
    } else if (blockId == 3) {
        // Блок 3 - коричневый дерево
        blockColor = vec3(0.5, 0.3, 0.2);
    } else if (blockId == 4) {
        // Блок 4 - зеленый
        blockColor = vec3(0.4, 0.7, 0.4);
    }
    
    f_color = a_color * texColor * vec4(blockColor, 1.0);
}

