#pragma once

#include <string>

class ChunkManager;
class WorldSave;
class Menu;

class WorldManager {
public:
    WorldManager(ChunkManager* chunkManager, WorldSave* worldSave, Menu* menu);
    
    bool createWorld(const std::string& worldName, int seed,
                     float baseFreq, int octaves, float lacunarity, float gain,
                     float baseHeight, float heightVariation);
    
    bool loadWorld(const std::string& worldPath,
                   float& baseFreq, int& octaves, float& lacunarity, float& gain,
                   float& baseHeight, float& heightVariation, int& seed);
    
    bool saveWorld(const std::string& worldPath, int seed,
                   float baseFreq, int octaves, float lacunarity, float gain,
                   float baseHeight, float heightVariation);
    
    void unloadWorld();
    
    bool isWorldLoaded() const { return worldLoaded; }
    const std::string& getCurrentWorldPath() const { return currentWorldPath; }
    
private:
    ChunkManager* chunkManager;
    WorldSave* worldSave;
    Menu* menu;
    
    bool worldLoaded = false;
    std::string currentWorldPath = "";
};

