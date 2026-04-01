#pragma once

#include <vector>
#include <string>
#include <memory>
#include <glm/glm.hpp>

namespace FarmEngine {

enum class TerrainType {
    Plains,
    Desert,
    Forest,
    Mountains,
    Hills,
    Swamp,
    Snow
};

enum class Biome {
    Farmland,
    Pasture,
    Orchard,
    Vineyard,
    Greenhouse,
    Pond,
    Barn,
    Silo
};

struct TerrainConfig {
    int32_t seed = 12345;
    float scale = 0.01f;
    float heightScale = 64.0f;
    float moistureScale = 0.01f;
    float temperatureScale = 0.01f;
    
    // Octavas de ruido
    int32_t octaves = 6;
    float persistence = 0.5f;
    float lacunarity = 2.0f;
};

class NoiseGenerator {
public:
    NoiseGenerator(int32_t seed);
    
    float noise2D(float x, float y) const;
    float noise3D(float x, float y, float z) const;
    
    float octaveNoise2D(float x, float y, int32_t octaves, 
                        float persistence, float lacunarity) const;
    
    void setSeed(int32_t seed);
    
private:
    std::vector<int32_t> permutation;
    int32_t seed;
    
    float fade(float t) const;
    float lerp(float a, float b, float t) const;
    float grad(int32_t hash, float x, float y) const;
};

class TerrainGenerator {
public:
    TerrainGenerator(const TerrainConfig& config);
    
    void generateHeightmap(int32_t width, int32_t depth, 
                          std::vector<float>& heightmap);
    
    void generateBiomeMap(int32_t width, int32_t depth,
                         const std::vector<float>& heightmap,
                         std::vector<Biome>& biomeMap);
    
    float getHeightAt(int32_t x, int32_t z) const;
    Biome getBiomeAt(int32_t x, int32_t z) const;
    
    // Generación específica para farming
    float getMoistureAt(int32_t x, int32_t z) const;
    float getTemperatureAt(int32_t x, int32_t z) const;
    float getFertilityAt(int32_t x, int32_t z) const;
    
    const TerrainConfig& getConfig() const { return config; }
    
private:
    TerrainType calculateTerrainType(float height, float moisture, float temperature) const;
    Biome calculateBiome(TerrainType terrain, float fertility) const;
    
    TerrainConfig config;
    std::unique_ptr<NoiseGenerator> heightNoise;
    std::unique_ptr<NoiseGenerator> moistureNoise;
    std::unique_ptr<NoiseGenerator> temperatureNoise;
    std::unique_ptr<NoiseGenerator> detailNoise;
};

// Sistema de erosión térmica/hidráulica
class TerrainErosion {
public:
    static void applyThermalErosion(std::vector<float>& heightmap,
                                   int32_t width, int32_t depth,
                                   int32_t iterations = 10);
    
    static void applyHydraulicErosion(std::vector<float>& heightmap,
                                     std::vector<float>& moisture,
                                     int32_t width, int32_t depth,
                                     int32_t iterations = 20);
    
    static void smoothTerrain(std::vector<float>& heightmap,
                             int32_t width, int32_t depth,
                             float strength = 0.5f);
};

// Features del terreno (árboles, rocas, agua)
struct TerrainFeature {
    enum class Type {
        Tree,
        Rock,
        Bush,
        Flower,
        Crop,
        Water,
        Path
    };
    
    Type type;
    glm::vec3 position;
    float rotation;
    float scale;
    uint32_t variant;
};

class TerrainFeatureGenerator {
public:
    TerrainFeatureGenerator(const TerrainConfig& config);
    
    void generateFeatures(int32_t chunkX, int32_t chunkZ,
                         const std::vector<float>& heightmap,
                         const std::vector<Biome>& biomeMap,
                         std::vector<TerrainFeature>& features);
    
    // Features específicos para granja
    void generateFarmFeatures(int32_t chunkX, int32_t chunkZ,
                             std::vector<TerrainFeature>& features);
    
private:
    TerrainConfig config;
    NoiseGenerator featureNoise{0};
};

} // namespace FarmEngine
