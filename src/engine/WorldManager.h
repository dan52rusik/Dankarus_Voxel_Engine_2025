#pragma once

#include <string>
#include <cstdint>

class ChunkManager;
class WorldSave;
class Menu;
class Camera;
class WorldBuilder;
class WaterSimulator;

class WorldManager {
public:
    WorldManager(ChunkManager* chunkManager, WorldSave* worldSave, Menu* menu);
    
    bool createWorld(const std::string& worldName, int64_t seed,
                     float baseFreq, int octaves, float lacunarity, float gain,
                     float baseHeight, float heightVariation);
    
    bool loadWorld(const std::string& worldPath,
                   float& baseFreq, int& octaves, float& lacunarity, float& gain,
                   float& baseHeight, float& heightVariation, int64_t& seed,
                   std::string& worldName, Camera* camera = nullptr);
    
    bool saveWorld(const std::string& worldPath, const std::string& worldName, int64_t seed,
                   float baseFreq, int octaves, float lacunarity, float gain,
                   float baseHeight, float heightVariation, Camera* camera = nullptr);
    
    void unloadWorld();
    
    bool isWorldLoaded() const { return worldLoaded; }
    const std::string& getCurrentWorldPath() const { return currentWorldPath; }
    const std::string& getCurrentWorldName() const { return currentWorldName; }
    int64_t getCurrentSeed() const { return currentSeed; }
    
    // Найти безопасную точку и поставить камеру над террейном
    // Проверяет: выше воды, не на отвесном склоне, в пределах границ мира
    // Если предпочтительная точка плохая - ищет ближайшую безопасную по спирали
    void spawnPlayerSafely(Camera* camera, float preferX = 0.0f, float preferZ = 0.0f,
                           float heightOffset = 3.0f, float searchRadiusMeters = 512.0f);
    
private:
	ChunkManager* chunkManager;
	class DecoManager* decoManager;
    WorldSave* worldSave;
    Menu* menu;
    WorldBuilder* worldBuilder;
    
    bool worldLoaded = false;
    std::string currentWorldPath = "";
    std::string currentWorldName = "";
    int64_t currentSeed = 0;
};

