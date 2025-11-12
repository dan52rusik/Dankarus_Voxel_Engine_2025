#version 330 core

in vec3 a_normal;
in vec3 a_worldPos;
out vec4 f_color;

uniform vec3 u_lightDir;           // передавай нормализованным из CPU
uniform float u_baseHeight;
uniform float u_heightVariation;
uniform float u_cellSize;          // размер клетки MC в «воксельных» единицах (обычно 1, 2, 4...)

struct Light {
    vec3 pos;
    vec3 color;
    float radius;
};

uniform int u_lightCount;
uniform Light u_lights[32];

uniform vec3 u_ambient = vec3(0.05);

// Плавная функция затухания
float atten(float d, float r) {
    float x = clamp(1.0 - d/r, 0.0, 1.0);
    return x*x*(3.0 - 2.0*x); // smoothstep-подобное
}

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

// Лёгкий поворот домена для земли (отдельная функция для разнообразия)
vec2 rot2(vec2 p) {
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

// --- вспомогательная палитра земли (5 тонов, тёплых) ---
const vec3 D0 = vec3(0.40, 0.29, 0.20);
const vec3 D1 = vec3(0.37, 0.27, 0.17);
const vec3 D2 = vec3(0.33, 0.25, 0.16);
const vec3 D3 = vec3(0.46, 0.34, 0.22);
const vec3 D4 = vec3(0.30, 0.22, 0.15);

// линейная интерполяция между соседними цветами палитры
vec3 dirtPalette(float t) {
    t = clamp(t, 0.0, 1.0);
    float x = t * 4.0;
    float f = fract(x);
    int i = int(floor(x));
    vec3 c0 = (i == 0) ? D0 : (i == 1) ? D1 : (i == 2) ? D2 : D3;
    vec3 c1 = (i == 0) ? D1 : (i == 1) ? D2 : (i == 2) ? D3 : D4;
    return mix(c0, c1, f);
}

// генератор оттенка и маски земли
struct DirtTintOut {
    vec3 color;
    float mask;
};

DirtTintOut makeDirtTint(vec3 N, float h, vec2 wp) {
    DirtTintOut o;
    
    // макро- и микро-вариации тона
    float macro = fbm(rot2(wp) * 0.05);    // крупные пятна
    float micro = vnoise(wp * 0.75);       // зерно
    float randCh = hash(floor(wp * 0.25)); // «клоки»/пятна на тайлах
    
    // индекс палитры (0..1) — теплота/насыщенность земли
    float shadeT = clamp(0.6 * macro + 0.3 * micro + 0.2 * randCh, 0.0, 1.0);
    vec3 dirtCol = dirtPalette(shadeT);
    
    // «влажность»: ниже по высоте темнее, плюс вариации
    float moisture = clamp(1.0 - h + 0.15 * macro, 0.0, 1.0);
    dirtCol *= mix(1.0, 0.82, moisture); // влажная земля темнее и чуть менее насыщенная
    
    // маска, где земля проявляется сильнее:
    // — на склонах (грунт просвечивает),
    // — на средних высотах,
    // — с шумовым варьированием по краям
    float slope = clamp(1.0 - abs(N.y), 0.0, 1.0);
    float slopeMask = smoothstep(0.35, 0.85, slope);
    float heightMask = 1.0 - smoothstep(0.65, 0.85, h); // меньше сверху
    float edgeNoise = 0.7 + 0.3 * vnoise(wp * 1.5);
    o.mask = clamp(slopeMask * heightMask * edgeNoise, 0.0, 1.0);
    o.color = dirtCol;
    
    return o;
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
    // Sanity-чек на нормали (на всякий случай)
    if (length(N) < 1e-5) N = vec3(0.0, 1.0, 0.0);
    float slope = 1.0 - clamp(dot(normalize(N), vec3(0.0, 1.0, 0.0)), 0.0, 1.0);
    
    float minH = u_baseHeight - u_heightVariation;
    float maxH = u_baseHeight + u_heightVariation;
    float h = clamp((a_worldPos.y - minH) / max(0.1, maxH - minH), 0.0, 1.0);
    
    // Простой slope tint для выразительности формы (трава на склонах, камень на крутых)
    vec3 grass = vec3(0.10, 0.30, 0.10);
    vec3 rock  = vec3(0.25, 0.25, 0.22);
    vec3 slopeTint = mix(grass, rock, smoothstep(0.35, 0.75, slope));
    
    vec3 base = biomeColor(h, slope, N);
    
    // Применяем slope tint для усиления контраста склонов (40% влияния на крутых склонах)
    base = mix(base, slopeTint, smoothstep(0.3, 0.7, slope) * 0.4);
    
    // Оттенки земли как в 7DTD
    DirtTintOut dt = makeDirtTint(N, h, a_worldPos.xz);
    
    // Аккуратно «подкрашиваем» базовый цвет:
    // чуть тёплый крен и локальные пятна, без убийства биома
    vec3 tinted = mix(base, dt.color, 0.35); // сила локального тона
    base = mix(base, tinted, dt.mask);       // где маска сильнее — виднее земля
    
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
    
    // Добавляем освещение от точечных источников света (ламп)
    vec3 pointLightColor = vec3(0.0);
    const int MAX_LIGHTS = 32;
    for (int i = 0; i < MAX_LIGHTS; i++) {
        if (i >= u_lightCount) break;
        
        vec3 L = u_lights[i].pos - a_worldPos;
        float d = length(L);
        
        // Приводим расстояние к «воксельным» единицам (MC-сетка может быть крупнее)
        float d_voxel = d * (1.0 / u_cellSize);
        
        // Проверяем расстояние до источника света
        if (d_voxel > u_lights[i].radius) continue;
        if (d < 0.01) continue; // Избегаем деления на ноль
        
        vec3 Ldir = normalize(L);
        float ndotl = max(dot(N, Ldir), 0.0);
        
        // Используем расстояние в воксельных единицах для затухания
        float a = atten(d_voxel, u_lights[i].radius);
        pointLightColor += col * u_lights[i].color * ndotl * a;
    }
    col += pointLightColor;
    
    // DEBUG: подсветим, если есть хоть один свет
    if (u_lightCount > 0) col += vec3(0.0, 0.02, 0.0);
    
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
