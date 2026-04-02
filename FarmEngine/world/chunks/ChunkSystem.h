#pragma once

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <glm/glm.hpp>

namespace FarmEngine {

struct ChunkConfig {
    int32_t chunkSize = 32;
    int32_t chunkHeight = 64;
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
    size_t getLoadedChunkCount() const;
    size_t getPendingGenerationCount() const;
    
    // Chunks visibles
    const std::vector<Chunk*>& getVisibleChunks() const;
    
private:
    ChunkManager() = default;
    ~ChunkManager();
    
    glm::ivec3 worldToChunk(const glm::vec3& worldPos) const;
    glm::ivec3 worldToLocal(const glm::vec3& worldPos) const;
    
    void processPendingGenerations();
    void buildChunkMeshes();
    void updateVisibleChunks(const glm::vec3& cameraPosition);
    
    ChunkConfig config;
    
    // Chunks cargados: key = packed uint64 from chunk coords
    std::unordered_map<uint64_t, std::unique_ptr<Chunk>> chunks;
    mutable std::mutex chunkMutex;
    
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

// ============================================================================
// LOD System
// ============================================================================

struct LODConfig {
    float lodDistances[4] = {20.0f, 50.0f, 100.0f, 200.0f};
    float vegetationLODDistances[4] = {10.0f, 30.0f, 60.0f, 120.0f};
    float terrainLODDistances[4] = {30.0f, 80.0f, 150.0f, 300.0f};
};

struct VegetationInstance {
    glm::vec3 position;
    float rotation;
    float scale;
    uint32_t variant;
    int32_t lodLevel;
    bool visible;
};

class LODSystem {
public:
    static LODSystem& getInstance();
    
    void initialize(const LODConfig& config);
    void update(const glm::vec3& cameraPosition);
    
    // Calculate LOD level based on distance
    int32_t calculateLOD(float distance) const;
    float getLODDistance(int32_t lodLevel) const;
    
    // Apply LOD to vegetation instances
    void applyVegetationLOD(std::vector<VegetationInstance>& instances,
                           const glm::vec3& cameraPosition);
    
    const LODConfig& getConfig() const { return config; }
    
private:
    LODSystem() = default;
    
    LODConfig config;
    glm::vec3 currentCameraPosition;
};

// ============================================================================
// Culling System
// ============================================================================

struct CullingConfig {
    int32_t chunkSize = 32;
    float maxCullDistance = 500.0f;
    bool enableFrustumCulling = true;
    bool enableDistanceCulling = true;
    bool enableOcclusionCulling = false;
};

struct CullingResult {
    std::vector<Chunk*> visible;
    size_t visibleCount = 0;
    size_t culled = 0;
};

class CullingSystem {
public:
    static CullingSystem& getInstance();
    
    void initialize(const CullingConfig& config);
    
    // Set frustum from view-projection matrix
    void setFrustum(const glm::mat4& viewProjMatrix);
    
    // Test if bounding box/sphere is in frustum
    bool isBoxInFrustum(const glm::vec3& min, const glm::vec3& max) const;
    bool isSphereInFrustum(const glm::vec3& center, float radius) const;
    
    // Distance-based culling test
    bool shouldCullByDistance(const glm::vec3& objectPosition,
                             const glm::vec3& cameraPosition,
                             float maxDistance) const;
    
    // Cull chunks
    CullingResult cullChunks(const std::vector<Chunk*>& chunks,
                            const glm::vec3& cameraPosition,
                            float maxDistance);
    
    // Cull vegetation instances
    void cullVegetation(std::vector<VegetationInstance>& instances,
                       const glm::vec3& cameraPosition,
                       float maxDistance);
    
    const CullingConfig& getConfig() const { return config; }
    
private:
    CullingSystem() = default;
    
    CullingConfig config;
    glm::vec4 frustumPlanes[6]; // Left, Right, Bottom, Top, Near, Far
};

} // namespace FarmEngine
