#version 330 core

in vec2 a_texCoord;
in vec3 a_worldPos;
in vec3 a_normal;
in float a_alpha;
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
uniform vec3 u_lightDir;
uniform vec3 u_sunColor;

// Плавная функция затухания
float atten(float d, float r) {
    float x = clamp(1.0 - d/r, 0.0, 1.0);
    return x*x*(3.0 - 2.0*x);
}

void main(){
    // Цвет воды (голубой/синий)
    vec3 waterColor = vec3(0.2, 0.5, 0.8); // Голубой цвет воды
    
    // Можно использовать текстуру воды, если есть
    vec4 texColor = vec4(waterColor, 1.0);
    
    // Вычисляем освещение
    vec3 N = normalize(a_normal);
    if (length(N) < 1e-5) N = vec3(0.0, 1.0, 0.0);
    
    // Небесный hemi
    vec3 sky    = vec3(0.6, 0.75, 0.9);
    vec3 ground = vec3(0.25, 0.22, 0.2);
    vec3 hemi   = mix(ground, sky, N.y * 0.5 + 0.5);
    
    // Направленный свет от солнца
    float NdotL = max(dot(N, normalize(u_lightDir)), 0.0);
    vec3 dirLit = texColor.rgb * (0.15 * hemi + 0.85 * u_sunColor * NdotL);
    
    vec3 color = dirLit;
    
    // Суммируем точечные источники света
    const int MAX_LIGHTS = 32;
    for (int i = 0; i < MAX_LIGHTS; i++) {
        if (i >= u_lightCount) break;
        
        vec3 L = u_lights[i].pos - a_worldPos;
        float d = length(L);
        if (d > u_lights[i].radius) continue;
        if (d < 0.01) continue;
        
        vec3 Ldir = normalize(L);
        float ndotl = max(dot(N, Ldir), 0.0);
        
        float a = atten(d, u_lights[i].radius);
        color += texColor.rgb * u_lights[i].color * ndotl * a;
    }
    
    // Гамма-коррекция
    color = pow(clamp(color, 0.0, 1.0), vec3(1.0/2.2));
    
    // Используем альфа-канал для прозрачности
    f_color = vec4(color, a_alpha);
}

