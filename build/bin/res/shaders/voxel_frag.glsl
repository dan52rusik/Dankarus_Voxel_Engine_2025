#version 330 core

in vec4 a_color;
in vec2 a_texCoord;
in vec3 a_worldPos;
in vec3 a_normal;
in float a_distance;
flat in float a_blockId;
out vec4 f_color;

uniform sampler2D u_texture0;

struct Light {
    vec3 pos;
    vec3 color;
    float radius;
};

uniform int u_lightCount;
uniform Light u_lights[32];

uniform vec3 u_ambient = vec3(0.05);
uniform vec3 u_lightDir;     // нормализованный (как в MC)
uniform vec3 u_sunColor;     // цвет солнца

// Плавная функция затухания
float atten(float d, float r) {
    float x = clamp(1.0 - d/r, 0.0, 1.0);
    return x*x*(3.0 - 2.0*x); // smoothstep-подобное
}

void main(){
    // Временно отключаем текстуру для проверки - блоки будут одноцветными по blockId
    vec4 texColor = vec4(1.0); // texture(u_texture0, a_texCoord) - выключено для проверки
    
    // Цветовая модификация на основе типа блока
    vec3 blockColor = vec3(1.0);
    vec3 emission = vec3(0.0); // Эмиссия (свечение) для источников света
    int blockId = int(a_blockId);
    
    if (blockId == 1) {
        // Блок 1 - серый камень
        blockColor = vec3(0.6, 0.6, 0.6);
    } else if (blockId == 2) {
        // Блок 2 - лампа (ярко светится как источник света)
        blockColor = vec3(1.0, 0.95, 0.85); // Базовый цвет лампы
        emission = vec3(1.0, 0.9, 0.7) * 2.5; // Яркое свечение (эмиссия)
    } else if (blockId == 3) {
        // Блок 3 - коричневый дерево
        blockColor = vec3(0.5, 0.3, 0.2);
    } else if (blockId == 4) {
        // Блок 4 - зеленый
        blockColor = vec3(0.4, 0.7, 0.4);
    }
    
    // Базовый цвет с учетом освещения
    vec3 baseColor = texColor.rgb * blockColor;
    vec3 N = normalize(a_normal);
    // Sanity-чек на нормали (на всякий случай)
    if (length(N) < 1e-5) N = vec3(0.0, 1.0, 0.0);
    
    // Небесный hemi (как в mc_frag)
    vec3 sky    = vec3(0.6, 0.75, 0.9);
    vec3 ground = vec3(0.25, 0.22, 0.2);
    vec3 hemi   = mix(ground, sky, N.y * 0.5 + 0.5);
    
    // Направленный свет от солнца
    float NdotL = max(dot(N, normalize(u_lightDir)), 0.0);
    vec3 dirLit = baseColor * (0.15 * hemi + 0.85 * u_sunColor * NdotL);
    
    vec3 color = dirLit;
    
    // Суммируем точечные источники света
    const int MAX_LIGHTS = 32;
    for (int i = 0; i < MAX_LIGHTS; i++) {
        if (i >= u_lightCount) break;
        
        vec3 L = u_lights[i].pos - a_worldPos;
        float d = length(L);
        if (d > u_lights[i].radius) continue;
        if (d < 0.01) continue; // Избегаем деления на ноль
        
        vec3 Ldir = normalize(L);
        float ndotl = max(dot(N, Ldir), 0.0);
        
        float a = atten(d, u_lights[i].radius);
        color += baseColor * u_lights[i].color * ndotl * a;
    }
    
    // Добавляем эмиссию самой лампы
    if (blockId == 2) {
        color += emission;
    }
    
    // ВАЖНО: baked a_color применять как *модуляцию*, а не слабый mix
    // (он у вас уже «освещение» граней; пусть умножает диффуз)
    color *= a_color.rgb;
    
    // Гамма-коррекция (вариант А: гамма только в конце фрагментов)
    color = pow(clamp(color, 0.0, 1.0), vec3(1.0/2.2));
    
    f_color = vec4(color, 1.0);
}

