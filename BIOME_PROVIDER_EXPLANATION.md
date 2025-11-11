# BiomeProviderFromImage - Объяснение

## Что это?

`WorldBiomeProviderFromImage` и `WorldDecoratorBlocksFromBiome` - это две связанные системы из 7 Days To Die:

1. **WorldBiomeProviderFromImage** - загружает биомную карту из изображения (PNG/TGA) и определяет биомы по координатам
2. **WorldDecoratorBlocksFromBiome** - размещает декорации (префабы и отдельные блоки) на основе биомов

## WorldBiomeProviderFromImage

### Назначение:

Загружает биомную карту из изображения и использует её для определения биомов в мире.

### Как работает:

1. **Загрузка биомной карты** (`biomes.png` или `biomes.tga`):
   - Каждый пиксель изображения соответствует области в мире
   - Цвет пикселя определяет тип биома
   - Масштаб: `biomesScaleDiv = worldSize / biomeMapWidth`

2. **Определение биома**:
   ```cpp
   int _x = x / biomesScaleDiv + biomesMapWidthHalf;
   int _y = z / biomesScaleDiv + biomesMapHeightHalf;
   byte biomeId = biomeMap[_y * biomeMapWidth + _x];
   ```

3. **Подбиомы (SubBiomes)**:
   - Используют `PerlinNoise::FBM` для определения подбиома
   - Например: "пустыня" может иметь подбиомы "пустыня с оазисом", "песчаные дюны"

4. **Радиационная карта** (`radiation.png`):
   - Опциональная карта радиации
   - 0 = нет радиации, 1 = низкая, 2 = средняя, 3 = высокая
   - Определяется по цвету: G=1, B=2, R=3

5. **Splat Maps** (`splat1.png`, `splat2.png`, `splat3.png`):
   - Карты текстур поверхности
   - Определяют, какой тип блока размещать на поверхности (трава, камень, песок и т.д.)

## WorldDecoratorBlocksFromBiome

### Назначение:

Размещает декорации (префабы и отдельные блоки) на основе биомов.

### Как работает:

1. **Группировка по биомам**:
   - Для каждого блока в чанке определяется биом
   - Блоки группируются по биомам для эффективной обработки

2. **Размещение префабов** (`decoratePrefabs`):
   - Для каждого биома проверяются `m_DecoPrefabs`
   - Префаб размещается с вероятностью `decoPrefab.prob`
   - Проверяется `GameUtils.CheckOreNoiseAt` для размещения на рудных жилах
   - Проверяется наклон поверхности (slope)

3. **Размещение отдельных блоков** (`decorateSingleBlocks`):
   - Для каждого блока проверяются `m_DecoBlocks`
   - Блок размещается с вероятностью `deco.prob`
   - Проверяется `GameUtils.CheckOreNoiseAt` для размещения на рудных жилах
   - Проверяется наклон поверхности

## Интеграция в проект

### Текущая реализация:

✅ **BiomeProviderFromImage** - базовая версия:
- Загрузка биомной карты из PNG
- Определение биома по координатам
- Поддержка радиационной карты (упрощенная)
- Интеграция в `ChunkManager::getBiomeAt()`

❌ **WorldDecoratorBlocksFromBiome** - пока не реализован:
- Требует систему префабов
- Требует `BiomeDefinition` с `m_DecoPrefabs` и `m_DecoBlocks`
- Требует `EnumDecoAllowed` для проверки размещения

### Использование:

```cpp
// Создать провайдер биомов
BiomeProviderFromImage* biomeProvider = new BiomeProviderFromImage("MyWorld", 4096);

// Загрузить биомную карту
if (biomeProvider->initData("worlds/MyWorld")) {
    // Установить в ChunkManager
    chunkManager->setBiomeProviderFromImage(biomeProvider);
    
    // Теперь getBiomeAt() будет использовать карту вместо шума
    BiomeDefinition::BiomeType biome = chunkManager->getBiomeAt(x, z);
}
```

### Формат биомной карты:

- **Файл**: `worlds/MyWorld/biomes.png` или `biomes.tga`
- **Формат**: PNG/TGA, RGB/RGBA
- **Размер**: Рекомендуется 512x512 или 1024x1024
- **Цвета**:
  - Красный (R) = Пустыня
  - Зеленый (G) = Лес
  - Синий (B) = Вода
  - Белый/Черный = Пустой биом (Any)

### Формат радиационной карты:

- **Файл**: `worlds/MyWorld/radiation.png` или `radiation.tga`
- **Формат**: PNG/TGA, RGB/RGBA
- **Цвета**:
  - Зеленый (G) = Низкая радиация (1)
  - Синий (B) = Средняя радиация (2)
  - Красный (R) = Высокая радиация (3)
  - Черный = Нет радиации (0)

## Будущие улучшения

1. **Подбиомы**: Реализовать `GetSubBiomeIdxAt` с использованием `PerlinNoise::FBM`
2. **Splat Maps**: Добавить поддержку `splat1.png`, `splat2.png`, `splat3.png`
3. **WorldDecoratorBlocksFromBiome**: Интегрировать систему декораций
4. **BiomeImageLoader**: Улучшить конвертацию цветов в биомы (сейчас упрощенная версия)
5. **TGA поддержка**: Добавить загрузку TGA файлов

## Примечания

- В оригинале 7DTD используется `GridCompressedData<byte>` для сжатия биомной карты
- В текущей реализации используется простой `std::vector<uint8_t>`
- Для больших миров можно добавить сжатие или тайлинг

