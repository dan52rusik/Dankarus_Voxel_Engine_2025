# Алгоритмы шума в проекте (из 7 Days To Die)

## Обзор

В проекте интегрированы три алгоритма шума из 7 Days To Die, каждый из которых используется для различных целей в генерации террейна:

1. **OpenSimplex2S** (через `OpenSimplex3D`) - для генерации высоты террейна
2. **PerlinNoise** - для генерации рудных жил (ore generation)
3. **SimplexNoise** - для специальных целей (пока не используется)

## Связь между алгоритмами

### 1. OpenSimplex2S / OpenSimplex3D

**Назначение:** Генерация высоты террейна (terrain height generation)

**Где используется:**
- `ChunkManager::evalSurfaceHeight()` - вычисление высоты поверхности
- `ChunkManager::generateChunk()` - генерация поля плотности для Marching Cubes
- `BiomeDefinition::GetBiomeAt()` - определение биома
- `DecoManager::decorateChunkRandom()` - генерация декораций

**Особенности:**
- Использует улучшенный алгоритм `OpenSimplex2S::Noise3_ImproveXY`
- Обеспечивает плавные переходы и естественный рельеф
- Поддерживает FBM (Fractal Brownian Motion) для многооктавного шума

**Пример использования:**
```cpp
OpenSimplex3D noise(seed);
float height = noise.fbm(x * 0.008f, 0.0f, z * 0.008f, 5, 2.0f, 0.5f);
```

### 2. PerlinNoise

**Назначение:** Генерация рудных жил (ore generation)

**Где используется:**
- `TerrainGeneratorWithBiomeResource` (в оригинале 7DTD) - генерация руд в слоях биома
- `GameUtils::GetOreNoiseAt()` - вычисление значения шума для руды
- `GameUtils::CheckOreNoiseAt()` - проверка наличия руды в точке

**Особенности:**
- Классический алгоритм Perlin шума
- Используется для создания рудных жил с частотой 0.05
- Формула: `(noise(x*0.05, y*0.05, z*0.05) - 0.333) * 3.0`
- Поддерживает 2D, 3D шум и FBM

**Пример использования:**
```cpp
PerlinNoise perlinNoise(seed);
float oreValue = GameUtils::GetOreNoiseAt(perlinNoise, x, y, z);
if (oreValue > 0.0f) {
    // Разместить руду
}
```

**Связь с TerrainGeneratorWithBiomeResource:**
В оригинале 7DTD `TerrainGeneratorWithBiomeResource` использует `PerlinNoise` для генерации руд:
```csharp
if (GameUtils.GetOreNoiseAt(this.perlinNoise, blockWorldPosX, _y2, blockWorldPosZ) > 0.0)
{
    // Разместить ресурс из слоя биома
}
```

### 3. SimplexNoise

**Назначение:** Специальные цели (пока не используется в проекте)

**Особенности:**
- Классический алгоритм Simplex шума (3D)
- Более быстрый, чем PerlinNoise
- Меньше артефактов
- Может использоваться для генерации пещер, подземных структур

**Пример использования:**
```cpp
float value = SimplexNoise::noise(x, y, z);
```

## Рекомендации по использованию

### Для генерации террейна:
- **Используйте OpenSimplex2S** (через `OpenSimplex3D`) - это основной алгоритм для высоты террейна

### Для генерации руд:
- **Используйте PerlinNoise** - как в оригинале 7DTD, это обеспечивает правильное распределение рудных жил

### Для специальных целей:
- **Используйте SimplexNoise** - для пещер, подземных структур, или других детализированных элементов

## Интеграция в проект

### ChunkManager
```cpp
// OpenSimplex3D для террейна
OpenSimplex3D noise(seed);
float height = evalSurfaceHeight(wx, wz); // Использует noise внутри
```

### TerrainGeneratorWithBiomeResource (будущая интеграция)
```cpp
// PerlinNoise для руд
PerlinNoise perlinNoise(seed);
float oreValue = GameUtils::GetOreNoiseAt(perlinNoise, x, y, z);
```

## Примечания

- Все три алгоритма используют один и тот же `seed` для согласованности генерации
- Для разных целей можно использовать разные seed (например, `seed + 1000` для руд)
- В оригинале 7DTD `PerlinNoise` инициализируется с тем же seed, что и основной генератор террейна

