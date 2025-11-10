#version 330 core

in vec3 a_normal;
in vec3 a_worldPos;
out vec4 f_color;

uniform vec3 u_lightDir;           // передавай нормализованным из CPU
uniform float u_baseHeight;
uniform float u_heightVariation;

// Улучшенный хэш без sin() - быстрый и без полос
uint h2u(vec2 p) {
    uvec2 q = floatBitsToUint(p) * uvec2(1597334673U, 3812015801U) + uvec2(12345U);
    q ^= (q.yx >> 16U);
    return q.x * q.y;
}

float hash(vec2 p) {
    return float(h2u(p) >> 9U) * (1.0 / 8388608.0);
}

float vnoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

// Поворот для снятия сеточности FBM
vec2 rot(vec2 p) {
    return mat2(0.866, -0.5, 0.5, 0.866) * p; // ~30°
}

float fbm(vec2 p) {
    // константная развёртка вместо динамического i<octaves
    const int O = 4;
    float value = 0.0;
    float amp = 0.5;
    float freq = 1.0;
    
    for (int i = 0; i < O; i++) {
        value += amp * vnoise(p * freq);
        freq *= 2.0;
        amp *= 0.5;
    }
    
    return value;
}

// биом
vec3 biomeColor(float normalizedHeight, float slope, vec3 normal) {
    vec3 water = vec3(0.10, 0.20, 0.40);
    vec3 sand  = vec3(0.76, 0.70, 0.50);
    vec3 grass = vec3(0.15, 0.35, 0.10);
    vec3 dirt  = vec3(0.45, 0.30, 0.20);
    vec3 rock  = vec3(0.35, 0.35, 0.36);
    vec3 snow  = vec3(0.85, 0.86, 0.88);
    vec3 wetDirt = vec3(0.28, 0.24, 0.18); // Влажная земля у воды
    
    vec3 col;
    
    if (normalizedHeight < 0.1) {
        col = mix(water, sand, smoothstep(0.0, 0.1, normalizedHeight));
    } else if (normalizedHeight < 0.3) {
        col = mix(sand, grass, smoothstep(0.1, 0.3, normalizedHeight));
    } else if (normalizedHeight < 0.6) {
        col = grass;
    } else if (normalizedHeight < 0.8) {
        col = mix(grass, rock, smoothstep(0.6, 0.8, normalizedHeight));
    } else {
        col = rock; // Базовая порода на высоте
    }
    
    // Крутые склоны - больше камня
    float rockInfluence = smoothstep(0.3, 0.9, slope);
    col = mix(col, rock, rockInfluence * 0.6);
    
    // Дополнительная земля на склонах
    if (slope > 0.4 && normalizedHeight < 0.7) {
        float dirtInfluence = smoothstep(0.4, 0.8, slope);
        col = mix(col, dirt, dirtInfluence * 0.3);
    }
    
    // Влажная земля у воды (тёмная кайма рядом с берегом)
    float shore = smoothstep(0.08, 0.12, normalizedHeight) * (1.0 - smoothstep(0.12, 0.16, normalizedHeight));
    col = mix(col, wetDirt, shore * 0.35);
    
    // Снег по нормали+высоте (лежит на плоскостях, но не на стенах)
    float snowMask = smoothstep(0.75, 0.9, normalizedHeight) * smoothstep(0.2, 0.6, normal.y);
    col = mix(col, snow, snowMask);
    
    return col;
}

void main() {
    vec3 N = normalize(a_normal);
    float slope = clamp(1.0 - abs(N.y), 0.0, 1.0);
    
    float minH = u_baseHeight - u_heightVariation;
    float maxH = u_baseHeight + u_heightVariation;
    float h = clamp((a_worldPos.y - minH) / max(0.1, maxH - minH), 0.0, 1.0);
    
    vec3 base = biomeColor(h, slope, N);
    
    // Снимаем сеточность FBM через поворот домена
    float detail = fbm(rot(a_worldPos.xz) * 0.15);
    detail = detail * 0.3 + 0.7;
    vec3 col = base * mix(0.85, 1.15, detail);
    
    // Hemispheric ambient вместо константы (небо/земля)
    vec3 sky = vec3(0.6, 0.75, 0.9);
    vec3 ground = vec3(0.25, 0.22, 0.2);
    vec3 hemi = mix(ground, sky, N.y * 0.5 + 0.5);
    
    float NdotL = max(dot(N, normalize(u_lightDir)), 0.0);
    vec3 colLit = col * (0.15 * hemi + 0.85 * NdotL);
    col = colLit;
    
    // Фейковая AO от склона - тени подчеркивают рельеф
    col *= mix(1.0, 0.85, slope);
    
    // Гамма-коррекция (если не используешь sRGB фреймбуфер)
    // Если используешь sRGB - закомментируй эту строку и включи GL_FRAMEBUFFER_SRGB
    col = pow(clamp(col, 0.0, 1.0), vec3(1.0 / 2.2));
    
    // Dither после гаммы для борьбы с бэндингом на 8-битном sRGB выходе
    float dither = (hash(gl_FragCoord.xy * 0.5) - 0.5) / 255.0;
    col += dither;
    col = clamp(col, 0.0, 1.0);
    
    f_color = vec4(col, 1.0);
}
