# TerrainGeneratorWithBiomeResource - Объяснение

## Что это?

`TerrainGeneratorWithBiomeResource` - это **абстрактный класс** из 7 Days To Die, который отвечает за генерацию террейна с учетом:

1. **Биомов** (BiomeDefinition) - определяет тип местности
2. **Слоев биома** (BiomeLayer) - разные материалы на разной глубине (трава, земля, камень)
3. **Ресурсов** (руды) - размещение рудных жил через PerlinNoise

## Как он работает?

### Основной процесс генерации:

```
1. Для каждого блока в чанке (16x16):
   ├── Определить биом (BiomeDefinition)
   ├── Получить высоту террейна (GetTerrainHeightByteAt)
   ├── Определить подбиом (subbiome) если нужно
   │
   └── Заполнить слои биома (сверху вниз):
       ├── Слой 0 (верхний): трава/песок/снег
       ├── Слой 1: земля/глина
       ├── Слой 2: камень
       └── ...
       
       Для каждого слоя:
       ├── Проверить, есть ли руда (PerlinNoise через GameUtils.GetOreNoiseAt)
       │   └── Если да → разместить ресурс из layer.m_Resources
       └── Если нет руды → разместить обычный блок из layer.m_Block
```

### Ключевые компоненты:

#### 1. PerlinNoise для руд
```csharp
// В оригинале 7DTD:
if (GameUtils.GetOreNoiseAt(this.perlinNoise, blockWorldPosX, _y2, blockWorldPosZ) > 0.0)
{
    // Разместить руду из layer.m_Resources
}
```

#### 2. Слои биома (BiomeLayer)
- Каждый биом имеет список слоев (`bd.m_Layers`)
- Каждый слой имеет:
  - `m_Depth` - глубина слоя (или -1 для "до дна")
  - `m_Block` - обычный блок слоя
  - `m_Resources` - список ресурсов (руд) с вероятностями
  - `SumResourceProbs` - накопленные вероятности для выбора ресурса

#### 3. Плотность для Marching Cubes
```csharp
sbyte density = _bv.Block.shape.IsTerrain() 
    ? MarchingCubes.DensityTerrain 
    : MarchingCubes.DensityAir;
_chunk.SetDensity(_x, _y, _z, density);
```

## Связь с текущей системой

### Что уже есть в проекте:

✅ **OpenSimplex3D** - для генерации высоты террейна  
✅ **BiomeDefinition** - определение биомов  
✅ **PerlinNoise** - для генерации руд (только что интегрирован)  
✅ **GameUtils::GetOreNoiseAt** - функция для проверки руд  
✅ **MarchingCubes** - для генерации поверхности  

### Чего не хватает:

❌ **BiomeLayer** - структура для слоев биома  
❌ **Система ресурсов** - список руд с вероятностями  
❌ **Заполнение слоев** - логика размещения блоков по слоям  
❌ **SubBiome** - подбиомы (например, "пустыня с оазисом")  

## Как интегрировать?

### Вариант 1: Полная интеграция (сложно)

Создать полную систему слоев биома:

```cpp
// BiomeLayer.h
struct BiomeLayer {
    int depth;                    // Глубина слоя (-1 = до дна)
    std::vector<uint8_t> blocks;  // Обычные блоки слоя
    struct Resource {
        uint8_t blockId;
        float probability;
    };
    std::vector<Resource> resources; // Ресурсы (руды)
    std::vector<float> sumResourceProbs; // Накопленные вероятности
};

// BiomeDefinition.h - добавить
struct BiomeDefinition {
    // ... существующие поля ...
    std::vector<BiomeLayer> layers;
    int totalLayerDepth;
};
```

### Вариант 2: Упрощенная интеграция (проще)

Использовать текущую систему, но добавить размещение руд:

```cpp
// В ChunkManager::generateChunk или MCChunk::generate
// После генерации densityField, но перед генерацией воды:

// 1. Определить биом
BiomeDefinition::BiomeType biome = chunkManager->getBiomeAt(wx, wz);

// 2. Для каждого блока ниже поверхности:
for (int y = surfaceHeight; y >= 0; y--) {
    // 3. Проверить руду
    if (GameUtils::CheckOreNoiseAt(perlinNoise, wx, y, wz)) {
        // 4. Разместить руду (например, камень с рудой)
        chunk->setVoxel(lx, ly, lz, ORE_BLOCK_ID);
    } else {
        // 5. Разместить обычный блок (камень, земля, трава)
        uint8_t blockId = getBlockForBiomeAndDepth(biome, surfaceHeight - y);
        chunk->setVoxel(lx, ly, lz, blockId);
    }
}
```

## Рекомендация

**Начать с упрощенной интеграции:**

1. ✅ Добавить `PerlinNoise` в `ChunkManager` (уже сделано)
2. ✅ Использовать `GameUtils::GetOreNoiseAt` для проверки руд
3. ⏳ Добавить простую функцию `getBlockForBiomeAndDepth()` для выбора блока
4. ⏳ Интегрировать размещение руд в `MCChunk::generate` или `ChunkManager::generateChunk`

**Позже можно добавить:**
- Полную систему слоев биома
- SubBiome систему
- Разные типы руд с вероятностями

## Пример использования (упрощенный)

```cpp
// В ChunkManager или MCChunk
PerlinNoise perlinNoise(seed);

// После генерации поверхности
for (int z = 0; z < CHUNK_SIZE_Z; z++) {
    for (int x = 0; x < CHUNK_SIZE_X; x++) {
        int wx = chunkX * CHUNK_SIZE_X + x;
        int wz = chunkZ * CHUNK_SIZE_Z + z;
        
        float surfaceHeight = evalSurfaceHeight(wx, wz);
        BiomeDefinition::BiomeType biome = getBiomeAt(wx, wz);
        
        // Заполняем блоки от поверхности вниз
        for (int y = (int)surfaceHeight; y >= 0; y--) {
            int ly = y - (chunkY * CHUNK_SIZE_Y);
            if (ly < 0 || ly >= CHUNK_SIZE_Y) continue;
            
            // Проверяем руду
            if (GameUtils::CheckOreNoiseAt(perlinNoise, wx, y, wz)) {
                chunk->setVoxel(x, ly, z, ORE_STONE_ID);
            } else {
                // Обычный блок в зависимости от глубины
                uint8_t blockId = getBlockForDepth(biome, surfaceHeight - y);
                chunk->setVoxel(x, ly, z, blockId);
            }
        }
    }
}
```

## Вывод

`TerrainGeneratorWithBiomeResource` - это **продвинутая система генерации**, которая объединяет:
- Биомы
- Слои материалов
- Размещение ресурсов (руд)

В текущем проекте уже есть **основа** (биомы, PerlinNoise, GameUtils), но нужно добавить **логику размещения блоков по слоям** и **интеграцию руд**.

