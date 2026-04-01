#pragma once

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>

namespace FarmEngine {

struct ChunkConfig {
    int32_t chunkSize = 32;
    int32_t renderDistance = 8;
    float lodDistances[4] = {20.0f, 50.0f, 100.0f, 200.0f};
};

class Chunk {
public:
    Chunk(int32_t x, int32_t z, const ChunkConfig& config);
    ~Chunk();
    
    // Posición en el mundo
    int32_t getChunkX() const { return chunkX; }
    int32_t getChunkZ() const { return chunkZ; }
    glm::vec3 getPosition() const;
    
    // Generación y carga
    bool generate(int32_t seed);
    bool loadFromDisk(const std::string& path);
    bool saveToDisk(const std::string& path);
    
    // Datos de bloques
    uint8_t getBlock(int32_t x, int32_t y, int32_t z) const;
    void setBlock(int32_t x, int32_t y, int32_t z, uint8_t blockID);
    
    // Mesh generation
    bool buildMesh();
    void rebuildMesh();
    
    // Estado
    bool isLoaded() const { return loaded; }
    bool isModified() const { return modified; }
    bool needsMeshUpdate() const { return needsMeshRebuild; }
    
    void setLoaded(bool value) { loaded = value; }
    void setModified(bool value) { modified = value; }
    
private:
    int32_t chunkX;
    int32_t chunkZ;
    ChunkConfig config;
    
    std::vector<uint8_t> blocks;  // Block IDs
    std::vector<uint8_t> data;    // Block data (metadata)
    std::vector<uint8_t> light;   // Light values
    
    bool loaded = false;
    bool modified = false;
    bool needsMeshRebuild = true;
    
    // Mesh data (separada para streaming)
    struct MeshData {
        std::vector<float> vertices;
        std::vector<uint32_t> indices;
        bool dirty = true;
    };
    
    MeshData opaqueMesh;
    MeshData transparentMesh;
};

class ChunkManager {
public:
    static ChunkManager& getInstance();
    
    void initialize(const ChunkConfig& config);
    void cleanup();
    
    // Actualización
    void update(const glm::vec3& playerPosition, float deltaTime);
    
    // Obtener chunks
    Chunk* getChunk(int32_t chunkX, int32_t chunkZ);
    Chunk* getChunkAtWorldPos(const glm::vec3& worldPos);
    
    // Generación de chunks
    void requestChunkGeneration(int32_t chunkX, int32_t chunkZ);
    
    // Streaming
    void loadChunksAroundPlayer(const glm::vec3& playerPosition);
    void unloadDistantChunks(const glm::vec3& playerPosition);
    
    // Bloques individuales
    uint8_t getBlock(const glm::vec3& worldPos);
    void setBlock(const glm::vec3& worldPos, uint8_t blockID);
    
    // Estadísticas
    size_t getLoadedChunkCount() const { return chunks.size(); }
    size_t getPendingGenerationCount() const { return pendingGeneration.size(); }
    
private:
    ChunkManager() = default;
    ~ChunkManager();
    
    glm::ivec3 worldToChunk(const glm::vec3& worldPos) const;
    glm::ivec3 worldToLocal(const glm::vec3& worldPos) const;
    
    ChunkConfig config;
    
    // Chunks cargados: key = (chunkX << 16) | chunkZ
    std::unordered_map<int64_t, std::unique_ptr<Chunk>> chunks;
    
    // Cola de generación
    std::vector<std::pair<int32_t, int32_t>> pendingGeneration;
    
    // Chunks visibles actualmente
    std::vector<Chunk*> visibleChunks;
    
    int32_t lastPlayerChunkX = 0;
    int32_t lastPlayerChunkZ = 0;
};

// Thread pool para generación de chunks
class ChunkGenerationThread {
public:
    ChunkGenerationThread();
    ~ChunkGenerationThread();
    
    void start();
    void stop();
    
    void addTask(int32_t chunkX, int32_t chunkZ, int32_t seed);
    
private:
    void threadFunction();
    
    // Implementación multihilo
};

} // namespace FarmEngine
