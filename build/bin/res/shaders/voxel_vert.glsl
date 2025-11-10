#version 330 core

layout (location = 0) in vec3 v_position;
layout (location = 1) in vec3 v_normal;
layout (location = 2) in vec2 v_texCoord;
layout (location = 3) in float v_light;
layout (location = 4) in float v_blockId;

out vec4 a_color;
out vec2 a_texCoord;
out vec3 a_worldPos;
out vec3 a_normal;
out float a_distance;
flat out float a_blockId;

uniform mat4 model;
uniform mat4 projview;
uniform mat4 u_view;
uniform vec3 u_skyLightColor;
uniform float u_gamma;

// Распаковка сжатого света
// Значения хранятся в диапазоне 0-15 (4 бита на канал)
vec4 decompress_light(float compressed_light) {
    int compressed = floatBitsToInt(compressed_light);
    vec4 result;
    result.r = ((compressed >> 24) & 0xF) / 15.0;
    result.g = ((compressed >> 16) & 0xF) / 15.0;
    result.b = ((compressed >> 8) & 0xF) / 15.0;
    result.a = (compressed & 0xF) / 15.0;
    return result;
}

void main(){
    vec4 modelpos = model * vec4(v_position, 1.0);
    vec4 viewmodelpos = u_view * modelpos;
    
    // Вычисляем мировую позицию и нормаль
    a_worldPos = modelpos.xyz;
    mat3 nmat = mat3(transpose(inverse(model)));
    a_normal = normalize(nmat * v_normal);
    
    // Распаковываем сжатый свет
    // v_light уже содержит правильное освещение, вычисленное в getFaceLight
    // с учетом нормалей граней и направления солнца
    vec4 decomp_light = decompress_light(v_light);
    vec3 light = decomp_light.rgb;
    
    // Применяем небесный свет как минимум (для теней и подземных областей)
    // Это гарантирует, что даже в тени есть минимальное освещение
    float skyLightFactor = decomp_light.a; // уже нормализовано (0-1)
    vec3 ambient = u_skyLightColor.rgb * skyLightFactor * 0.3; // минимальный ambient
    light = max(light, ambient);
    
    // Увеличиваем общую яркость для лучшей видимости
    light = light * 1.2;
    light = clamp(light, 0.0, 1.0); // Ограничиваем до 1.0
    
    // БЕЗ гамма-коррекции (гамма будет в фрагментном шейдере)
    a_color = vec4(clamp(light, 0.0, 1.0), 1.0);
    a_texCoord = v_texCoord;
    a_distance = length(viewmodelpos);
    a_blockId = v_blockId;
    gl_Position = projview * modelpos;
}

