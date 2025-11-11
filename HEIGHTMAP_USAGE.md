# Использование высотных карт в VoxelNoxel

## Обзор

Система работы с высотными картами позволяет использовать готовые карты высот вместо процедурной генерации террейна. Это полезно для:
- Импорта готовых ландшафтов
- Предгенерированных миров
- Создания специфических форм рельефа

## Поддерживаемые форматы

### PNG
- Формат: 8-bit grayscale, RGB или RGBA
- Значение высоты:
  - Grayscale (1 канал): берется напрямую из пикселя
  - RGB/RGBA: берется из красного канала (R)
- Диапазон: 0-255
- По умолчанию изображение переворачивается по Y (flip=true) для совместимости с большинством DTM редакторов

### RAW (16-bit)
- Формат: 16-bit unsigned short, little-endian
- Размер карты определяется автоматически: `sqrt(fileSize / 2)`
- Диапазон: 0-65535 (конвертируется в 0-255)

## Использование в коде

### Загрузка высотной карты

```cpp
#include "voxels/ChunkManager.h"

ChunkManager* chunkManager = ...;

// Загрузить из файла (автоопределение формата)
chunkManager->setHeightMap("res/heightmaps/world.png");
// или
chunkManager->setHeightMap("res/heightmaps/world.raw");

// Настройка масштаба высот
// baseHeight - базовая высота (смещение)
// scale - масштаб (умножается на значения из карты)
chunkManager->setHeightMapScale(10.0f, 0.5f); // базовая высота 10, масштаб 0.5

// Вернуться к процедурной генерации
chunkManager->clearHeightMap();
```

### Программная работа с высотными картами

```cpp
#include "voxels/HeightMapUtils.h"

using namespace HeightMapUtils;

// Загрузить высотную карту (по умолчанию flip=true)
HeightData2D* heightMap = convertDTMToHeightData("res/heightmaps/world.png");
// или с явным указанием flip
HeightData2D* heightMap = convertDTMToHeightData("res/heightmaps/world.png", false);

// Сглаживание террейна (3 прохода)
HeightData2D* smoothed = smoothTerrain(3, *heightMap);

// Сохранить в RAW формат
saveHeightMapRAW("res/heightmaps/smoothed.raw", 
                 smoothed->width, smoothed->height, *smoothed);

// Установить в ChunkManager
chunkManager->setHeightMap(smoothed);
```

## Утилиты HeightMapUtils

### Загрузка
- `convertDTMToHeightData(filepath)` - загрузить PNG/TGA
- `loadRAWToHeightData(filepath)` - загрузить RAW (автоопределение размера)
- `loadHeightMapRAW(filepath, width, height, factor, clampHeight)` - загрузить RAW с указанными размерами

### Сохранение
- `saveHeightMapRAW(filepath, width, height, data)` - сохранить в RAW формат
- `saveHeightMapRAW(filepath, width, height, heightData)` - сохранить HeightData2D

### Обработка
- `smoothTerrain(passes, heightData)` - сглаживание террейна
- `smoothTerrainInPlace(passes, heightData)` - сглаживание на месте
- `getHeightStats(heightData)` - получить статистику (min, max, average)

### Работа с тайлами
- `getTileInfo(worldX, worldZ, tileSize)` - получить информацию о тайле
- `extractTile(source, tileX, tileZ, tileSize, overlap)` - извлечь тайл из большой карты

## Примеры

### Пример 1: Загрузка готовой карты

```cpp
// В Engine::init() или при создании мира
chunkManager->setHeightMap("res/heightmaps/mountain.png");
chunkManager->setHeightMapScale(15.0f, 1.0f); // базовая высота 15, масштаб 1.0
```

### Пример 2: Сглаживание процедурно сгенерированной карты

```cpp
// Генерируем карту процедурно (можно использовать любой генератор)
HeightData2D* heightMap = new HeightData2D(1024, 1024);
// ... заполняем данными ...

// Сглаживаем
smoothTerrainInPlace(5, *heightMap);

// Сохраняем для повторного использования
saveHeightMapRAW("res/heightmaps/smoothed.raw", 
                 heightMap->width, heightMap->height, *heightMap);

// Используем в игре
chunkManager->setHeightMap(heightMap);
```

### Пример 3: Работа с большими картами (тайлы)

```cpp
// Загружаем большую карту
HeightData2D* largeMap = convertDTMToHeightData("res/heightmaps/large_world.png");

// Извлекаем тайл для конкретной области
int tileX = 5;
int tileZ = 3;
int tileSize = 256;
HeightData2D* tile = extractTile(*largeMap, tileX, tileZ, tileSize, 8);

// Используем тайл
chunkManager->setHeightMap(tile);
```

## Примечания

1. **Производительность**: Высотные карты загружаются один раз при установке. Чанки генерируются на лету из карты.

2. **Память**: Большие карты (например, 4096x4096) занимают ~64MB памяти. Используйте тайлы для больших миров.

3. **Масштабирование**: Параметры `baseHeight` и `heightScale` позволяют настроить высоту террейна под ваши нужды.

4. **Совместимость**: Можно переключаться между процедурной генерацией и высотными картами в любой момент.

5. **Формат RAW**: Используется формат 16-bit little-endian, совместимый с 7 Days To Die и многими другими играми. Коэффициенты конвертации U8↔U16 (257.0) обеспечивают симметричный round-trip.

6. **Flip по Y**: По умолчанию изображения переворачиваются по вертикали (flip=true) для совместимости с большинством DTM редакторов. Можно отключить, передав `false`.

7. **Grayscale поддержка**: Система автоматически определяет grayscale изображения (1 канал) и корректно их обрабатывает.

8. **TGA формат**: Пока не поддерживается. При попытке загрузки TGA будет выведено сообщение об ошибке.

9. **Билинейная интерполяция**: По умолчанию используется билинейная интерполяция при конвертации в поле плотности, что убирает "ступеньки" и делает террейн более плавным. Можно отключить для повышения производительности.

10. **Статистика высот**: При загрузке карты автоматически выводится статистика (min, max, average), что помогает быстро обнаружить проблемы с масштабированием.

11. **Edge modes**: Поддерживаются два режима обработки краев:
    - `clamp` (0) - значения за границами карты обрезаются (по умолчанию)
    - `wrap` (1) - значения за границами повторяются (для тайлинга)

