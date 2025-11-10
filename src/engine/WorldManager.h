#pragma once

#include <string>
#include <cstdint>

class ChunkManager;
class WorldSave;
class Menu;
class Camera;

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
    
private:
    ChunkManager* chunkManager;
    WorldSave* worldSave;
    Menu* menu;
    
    bool worldLoaded = false;
    std::string currentWorldPath = "";
    std::string currentWorldName = "";
    int64_t currentSeed = 0;
};

