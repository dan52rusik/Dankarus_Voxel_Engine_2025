# Фрактальный шум (vp_Perlin, vp_FractalNoise, vp_SmoothRandom)

## Обзор

Интегрированы три класса из 7 Days To Die для генерации фрактального шума и плавных случайных значений:

1. **vp_Perlin** - классический Perlin шум (альтернативная реализация)
2. **vp_FractalNoise** - фрактальный шум с разными типами
3. **vp_SmoothRandom** - генератор плавных случайных значений

## Назначение

Эти классы используются для:
- **Эффектов камеры** - плавное дрожание, движение
- **Анимаций** - плавные случайные движения объектов
- **Специальных типов генерации** - Ridged Multifractal для острых хребтов
- **Визуальных эффектов** - ветер, волны, и т.д.

## vp_Perlin

Классический алгоритм Perlin шума (отличается от `PerlinNoise` - это другая реализация).

### Использование:

```cpp
#include "noise/vp_Perlin.h"

// Создать генератор
vp_Perlin perlin; // Случайный seed
vp_Perlin perlinSeeded(12345); // С указанным seed

// 1D шум
float value1D = perlin.Noise(x);

// 2D шум
float value2D = perlin.Noise(x, y);

// 3D шум
float value3D = perlin.Noise(x, y, z);
```

## vp_FractalNoise

Фрактальный шум с тремя типами:

### 1. HybridMultifractal
Гибридный мультифрактал - создает плавные, естественные формы.

```cpp
vp_FractalNoise fractal(1.27f, 2.04f, 8.36f); // H, Lacunarity, Octaves
float value = fractal.HybridMultifractal(x, y, 0.58f); // offset
```

### 2. RidgedMultifractal
Гребневой мультифрактал - создает острые хребты и горы.

```cpp
float value = fractal.RidgedMultifractal(x, y, 0.5f, 2.0f); // offset, gain
```

### 3. BrownianMotion
Броуновское движение - классический FBM (Fractal Brownian Motion).

```cpp
float value = fractal.BrownianMotion(x, y);
```

### Параметры конструктора:

- `inH` - параметр Hurst (контролирует шероховатость)
- `inLacunarity` - лакунарность (множитель частоты для каждой октавы)
- `inOctaves` - количество октав (может быть дробным)

## vp_SmoothRandom

Генератор плавных случайных значений для эффектов.

### Использование:

```cpp
#include "noise/vp_SmoothRandom.h"

// Получить Vector3 для плавного движения
glm::vec3 movement = vp_SmoothRandom::GetVector3(1.0f); // speed

// Получить Vector3 с центрированием (для плавных переходов)
glm::vec3 centered = vp_SmoothRandom::GetVector3Centered(1.0f);

// С указанным временем
glm::vec3 centeredAtTime = vp_SmoothRandom::GetVector3Centered(time, 1.0f);

// Получить одно значение
float value = vp_SmoothRandom::Get(1.0f);

// Получить экземпляр для прямого использования
vp_FractalNoise* noise = vp_SmoothRandom::Get();
float custom = noise->HybridMultifractal(x, y, offset);
```

### Применение:

**Для эффекта камеры:**
```cpp
// В update loop
glm::vec3 shake = vp_SmoothRandom::GetVector3Centered(currentTime, shakeSpeed);
camera->position += shake * shakeIntensity;
```

**Для анимации объектов:**
```cpp
glm::vec3 wind = vp_SmoothRandom::GetVector3(windSpeed);
tree->applyWind(wind);
```

## Сравнение с другими алгоритмами

| Алгоритм | Назначение | Использование |
|----------|-----------|---------------|
| **OpenSimplex2S** | Генерация террейна | Основной алгоритм для высоты |
| **PerlinNoise** | Генерация руд | Размещение рудных жил |
| **SimplexNoise** | Специальные цели | Пещеры, структуры |
| **vp_Perlin** | Фрактальный шум | Основа для vp_FractalNoise |
| **vp_FractalNoise** | Эффекты, анимации | Камера, ветер, визуальные эффекты |
| **vp_SmoothRandom** | Плавные значения | Удобный API для эффектов |

## Примеры интеграции

### Эффект дрожания камеры:

```cpp
class Camera {
    float shakeTime = 0.0f;
    float shakeIntensity = 0.1f;
    
    void update(float deltaTime) {
        shakeTime += deltaTime;
        glm::vec3 shake = vp_SmoothRandom::GetVector3Centered(shakeTime, 5.0f);
        position += shake * shakeIntensity;
    }
};
```

### Генерация острых гор:

```cpp
vp_FractalNoise ridged(0.5f, 2.0f, 6.0f); // H=0.5 для острых форм
float height = ridged.RidgedMultifractal(x, z, 0.5f, 2.0f);
// Использовать для генерации горных хребтов
```

### Плавное движение объектов:

```cpp
glm::vec3 offset = vp_SmoothRandom::GetVector3(0.5f);
object->position = basePosition + offset * movementRange;
```

## Примечания

- `vp_SmoothRandom` использует статический экземпляр `vp_FractalNoise` (thread-safe)
- Параметры по умолчанию: `H=1.27f`, `Lacunarity=2.04f`, `Octaves=8.36f`
- Для детерминированности можно создать собственный `vp_FractalNoise` с указанным seed
- `vp_Perlin` отличается от `PerlinNoise` - это разные реализации одного алгоритма

