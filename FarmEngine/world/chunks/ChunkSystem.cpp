#include "ChunkSystem.h"
#include "../terrain/TerrainGenerator.h"
#include "../../renderer/Renderer.h"
#include <algorithm>
#include <cmath>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace FarmEngine {

// ============================================================================
// Chunk Implementation
// ============================================================================

Chunk::Chunk(int32_t x, int32_t z, const ChunkConfig& config)
    : chunkX(x)
    , chunkZ(z)
    , config(config)
{
    const int32_t size = config.chunkSize;
    const int32_t height = config.chunkHeight;
    const size_t totalBlocks = static_cast<size_t>(size) * height * size;
    
    blocks.resize(totalBlocks, 0);
    data.resize(totalBlocks, 0);
    light.resize(totalBlocks, 0);
}

Chunk::~Chunk() {
    // Cleanup resources
}

glm::vec3 Chunk::getPosition() const {
    return glm::vec3(
        static_cast<float>(chunkX * config.chunkSize),
        0.0f,
        static_cast<float>(chunkZ * config.chunkSize)
    );
}

bool Chunk::generate(int32_t seed) {
    if (loaded) return true;
    
    TerrainGenerator terrainGen(TerrainConfig{seed});
    
    const int32_t size = config.chunkSize;
    const int32_t worldOffsetX = chunkX * size;
    const int32_t worldOffsetZ = chunkZ * size;
    
    // Generate heightmap for this chunk
    std::vector<float> heightmap(size * size);
    for (int32_t z = 0; z < size; ++z) {
        for (int32_t x = 0; x < size; ++x) {
            heightmap[z * size + x] = terrainGen.getHeightAt(
                worldOffsetX + x, 
                worldOffsetZ + z
            );
        }
    }
    
    // Generate biome map
    std::vector<Biome> biomeMap(size * size);
    terrainGen.generateBiomeMap(size, size, heightmap, biomeMap);
    
    // Fill blocks based on heightmap
    for (int32_t z = 0; z < size; ++z) {
        for (int32_t x = 0; x < size; ++x) {
            float height = heightmap[z * size + x];
            Biome biome = biomeMap[z * size + x];
            
            // Simple block placement logic
            for (int32_t y = 0; y < config.chunkHeight; ++y) {
                uint8_t blockID = 0; // Air
                
                if (y < static_cast<int32_t>(height) - 3) {
                    blockID = 1; // Stone
                } else if (y < static_cast<int32_t>(height)) {
                    blockID = 2; // Dirt
                } else if (y == static_cast<int32_t>(height)) {
                    // Surface block based on biome
                    switch (biome) {
                        case Biome::Farmland: blockID = 3; break; // Farmland
                        case Biome::Pasture: blockID = 4; break; // Grass
                        default: blockID = 5; break; // Default surface
                    }
                }
                
                setBlock(x, y, z, blockID);
            }
        }
    }
    
    loaded = true;
    needsMeshRebuild = true;
    
    return true;
}

bool Chunk::loadFromDisk(const std::string& path) {
    // TODO: Implement disk loading
    // For now, just generate
    return generate(12345);
}

bool Chunk::saveToDisk(const std::string& path) {
    // TODO: Implement disk saving
    if (!modified) return true;
    modified = false;
    return true;
}

uint8_t Chunk::getBlock(int32_t x, int32_t y, int32_t z) const {
    if (x < 0 || x >= config.chunkSize ||
        y < 0 || y >= config.chunkHeight ||
        z < 0 || z >= config.chunkSize) {
        return 0;
    }
    
    const size_t index = static_cast<size_t>(y) * config.chunkSize * config.chunkSize +
                         static_cast<size_t>(z) * config.chunkSize +
                         static_cast<size_t>(x);
    
    if (index >= blocks.size()) return 0;
    return blocks[index];
}

void Chunk::setBlock(int32_t x, int32_t y, int32_t z, uint8_t blockID) {
    if (x < 0 || x >= config.chunkSize ||
        y < 0 || y >= config.chunkHeight ||
        z < 0 || z >= config.chunkSize) {
        return;
    }
    
    const size_t index = static_cast<size_t>(y) * config.chunkSize * config.chunkSize +
                         static_cast<size_t>(z) * config.chunkSize +
                         static_cast<size_t>(x);
    
    if (index < blocks.size()) {
        blocks[index] = blockID;
        modified = true;
        needsMeshRebuild = true;
    }
}

bool Chunk::buildMesh() {
    if (!needsMeshRebuild || !loaded) return false;
    
    // Simple greedy meshing or naive mesh generation
    opaqueMesh.vertices.clear();
    opaqueMesh.indices.clear();
    transparentMesh.vertices.clear();
    transparentMesh.indices.clear();
    
    const int32_t size = config.chunkSize;
    
    for (int32_t y = 0; y < config.chunkHeight; ++y) {
        for (int32_t z = 0; z < size; ++z) {
            for (int32_t x = 0; x < size; ++x) {
                uint8_t block = getBlock(x, y, z);
                if (block == 0) continue; // Skip air
                
                // Check if block is exposed (has at least one air neighbor)
                bool exposed = false;
                exposed |= (getBlock(x+1, y, z) == 0);
                exposed |= (getBlock(x-1, y, z) == 0);
                exposed |= (getBlock(x, y+1, z) == 0);
                exposed |= (getBlock(x, y-1, z) == 0);
                exposed |= (getBlock(x, y, z+1) == 0);
                exposed |= (getBlock(x, y, z-1) == 0);
                
                if (!exposed) continue;
                
                // Generate quad for each exposed face
                float fx = static_cast<float>(x);
                float fy = static_cast<float>(y);
                float fz = static_cast<float>(z);
                
                // Right face (+X)
                if (getBlock(x+1, y, z) == 0) {
                    opaqueMesh.vertices.insert(opaqueMesh.vertices.end(), {
                        fx+1, fy,   fz,   1, 0, 0,
                        fx+1, fy+1, fz,   1, 0, 0,
                        fx+1, fy+1, fz+1, 1, 0, 0,
                        fx+1, fy,   fz+1, 1, 0, 0
                    });
                    uint32_t base = static_cast<uint32_t>(opaqueMesh.vertices.size() / 6) - 4;
                    opaqueMesh.indices.insert(opaqueMesh.indices.end(), {
                        base, base+1, base+2,
                        base, base+2, base+3
                    });
                }
                
                // Left face (-X)
                if (getBlock(x-1, y, z) == 0) {
                    opaqueMesh.vertices.insert(opaqueMesh.vertices.end(), {
                        fx,   fy,   fz+1, -1, 0, 0,
                        fx,   fy+1, fz+1, -1, 0, 0,
                        fx,   fy+1, fz,   -1, 0, 0,
                        fx,   fy,   fz,   -1, 0, 0
                    });
                    uint32_t base = static_cast<uint32_t>(opaqueMesh.vertices.size() / 6) - 4;
                    opaqueMesh.indices.insert(opaqueMesh.indices.end(), {
                        base, base+1, base+2,
                        base, base+2, base+3
                    });
                }
                
                // Top face (+Y)
                if (getBlock(x, y+1, z) == 0) {
                    opaqueMesh.vertices.insert(opaqueMesh.vertices.end(), {
                        fx,   fy+1, fz,   0, 1, 0,
                        fx,   fy+1, fz+1, 0, 1, 0,
                        fx+1, fy+1, fz+1, 0, 1, 0,
                        fx+1, fy+1, fz,   0, 1, 0
                    });
                    uint32_t base = static_cast<uint32_t>(opaqueMesh.vertices.size() / 6) - 4;
                    opaqueMesh.indices.insert(opaqueMesh.indices.end(), {
                        base, base+1, base+2,
                        base, base+2, base+3
                    });
                }
                
                // Bottom face (-Y)
                if (getBlock(x, y-1, z) == 0) {
                    opaqueMesh.vertices.insert(opaqueMesh.vertices.end(), {
                        fx,   fy, fz+1, 0, -1, 0,
                        fx,   fy, fz,   0, -1, 0,
                        fx+1, fy, fz,   0, -1, 0,
                        fx+1, fy, fz+1, 0, -1, 0
                    });
                    uint32_t base = static_cast<uint32_t>(opaqueMesh.vertices.size() / 6) - 4;
                    opaqueMesh.indices.insert(opaqueMesh.indices.end(), {
                        base, base+1, base+2,
                        base, base+2, base+3
                    });
                }
                
                // Front face (+Z)
                if (getBlock(x, y, z+1) == 0) {
                    opaqueMesh.vertices.insert(opaqueMesh.vertices.end(), {
                        fx,   fy,   fz+1, 0, 0, 1,
                        fx,   fy+1, fz+1, 0, 0, 1,
                        fx+1, fy+1, fz+1, 0, 0, 1,
                        fx+1, fy,   fz+1, 0, 0, 1
                    });
                    uint32_t base = static_cast<uint32_t>(opaqueMesh.vertices.size() / 6) - 4;
                    opaqueMesh.indices.insert(opaqueMesh.indices.end(), {
                        base, base+1, base+2,
                        base, base+2, base+3
                    });
                }
                
                // Back face (-Z)
                if (getBlock(x, y, z-1) == 0) {
                    opaqueMesh.vertices.insert(opaqueMesh.vertices.end(), {
                        fx+1, fy,   fz,   0, 0, -1,
                        fx+1, fy+1, fz,   0, 0, -1,
                        fx,   fy+1, fz,   0, 0, -1,
                        fx,   fy,   fz,   0, 0, -1
                    });
                    uint32_t base = static_cast<uint32_t>(opaqueMesh.vertices.size() / 6) - 4;
                    opaqueMesh.indices.insert(opaqueMesh.indices.end(), {
                        base, base+1, base+2,
                        base, base+2, base+3
                    });
                }
            }
        }
    }
    
    opaqueMesh.dirty = true;
    transparentMesh.dirty = true;
    needsMeshRebuild = false;
    
    return true;
}

void Chunk::rebuildMesh() {
    needsMeshRebuild = true;
}

// ============================================================================
// ChunkManager Implementation
// ============================================================================

ChunkManager& ChunkManager::getInstance() {
    static ChunkManager instance;
    return instance;
}

ChunkManager::~ChunkManager() {
    cleanup();
}

void ChunkManager::initialize(const ChunkConfig& cfg) {
    config = cfg;
}

void ChunkManager::cleanup() {
    std::lock_guard<std::mutex> lock(chunkMutex);
    chunks.clear();
    pendingGeneration.clear();
    visibleChunks.clear();
}

void ChunkManager::update(const glm::vec3& playerPosition, float deltaTime) {
    // Calculate current chunk
    int32_t playerChunkX = static_cast<int32_t>(std::floor(playerPosition.x / config.chunkSize));
    int32_t playerChunkZ = static_cast<int32_t>(std::floor(playerPosition.z / config.chunkSize));
    
    // Only update if player changed chunks
    if (playerChunkX != lastPlayerChunkX || playerChunkZ != lastPlayerChunkZ) {
        lastPlayerChunkX = playerChunkX;
        lastPlayerChunkZ = playerChunkZ;
        
        loadChunksAroundPlayer(playerPosition);
        unloadDistantChunks(playerPosition);
    }
    
    // Update visible chunks (culling)
    updateVisibleChunks(playerPosition);
    
    // Process pending generations
    processPendingGenerations();
    
    // Build meshes for loaded chunks
    buildChunkMeshes();
}

Chunk* ChunkManager::getChunk(int32_t chunkX, int32_t chunkZ) {
    std::lock_guard<std::mutex> lock(chunkMutex);
    
    // Use proper two's complement for negative coordinates
    uint64_t key = (static_cast<uint64_t>(static_cast<int64_t>(chunkX)) << 32) | 
                   (static_cast<uint64_t>(chunkZ) & 0xFFFFFFFFULL);
    
    auto it = chunks.find(key);
    if (it != chunks.end()) {
        return it->second.get();
    }
    return nullptr;
}

Chunk* ChunkManager::getChunkAtWorldPos(const glm::vec3& worldPos) {
    glm::ivec3 chunkCoords = worldToChunk(worldPos);
    return getChunk(chunkCoords.x, chunkCoords.z);
}

void ChunkManager::requestChunkGeneration(int32_t chunkX, int32_t chunkZ) {
    std::lock_guard<std::mutex> lock(chunkMutex);
    
    // Check if chunk already exists
    if (getChunk(chunkX, chunkZ) != nullptr) {
        return;
    }
    
    pendingGeneration.emplace_back(chunkX, chunkZ);
}

void ChunkManager::loadChunksAroundPlayer(const glm::vec3& playerPosition) {
    int32_t playerChunkX = static_cast<int32_t>(std::floor(playerPosition.x / config.chunkSize));
    int32_t playerChunkZ = static_cast<int32_t>(std::floor(playerPosition.z / config.chunkSize));
    
    int32_t renderDistance = config.renderDistance;
    
    // Load chunks in spiral order from player position
    std::vector<std::pair<int32_t, int32_t>> chunkOrder;
    
    for (int32_t dz = -renderDistance; dz <= renderDistance; ++dz) {
        for (int32_t dx = -renderDistance; dx <= renderDistance; ++dx) {
            int32_t chunkX = playerChunkX + dx;
            int32_t chunkZ = playerChunkZ + dz;
            
            // Check if chunk is already loaded
            if (getChunk(chunkX, chunkZ) == nullptr) {
                chunkOrder.emplace_back(chunkX, chunkZ);
            }
        }
    }
    
    // Sort by distance to player
    glm::vec2 playerChunkPos(playerChunkX, playerChunkZ);
    std::sort(chunkOrder.begin(), chunkOrder.end(),
        [playerChunkPos](const auto& a, const auto& b) {
            // Use squared distance to avoid expensive sqrt operations
            float distA = glm::length2(glm::vec2(a.first, a.second) - playerChunkPos);
            float distB = glm::length2(glm::vec2(b.first, b.second) - playerChunkPos);
            return distA < distB;
        });
    
    // Request generation for nearby chunks
    const size_t maxPending = 16; // Limit concurrent generations
    size_t generated = 0;
    
    for (const auto& [chunkX, chunkZ] : chunkOrder) {
        if (generated >= maxPending) break;
        
        if (getChunk(chunkX, chunkZ) == nullptr) {
            requestChunkGeneration(chunkX, chunkZ);
            ++generated;
        }
    }
}

void ChunkManager::unloadDistantChunks(const glm::vec3& playerPosition) {
    int32_t playerChunkX = static_cast<int32_t>(std::floor(playerPosition.x / config.chunkSize));
    int32_t playerChunkZ = static_cast<int32_t>(std::floor(playerPosition.z / config.chunkSize));
    
    int32_t unloadDistance = config.renderDistance + 2; // Buffer zone
    
    std::vector<uint64_t> chunksToRemove;
    
    std::lock_guard<std::mutex> lock(chunkMutex);
    
    for (auto& [key, chunk] : chunks) {
        int32_t chunkX = chunk->getChunkX();
        int32_t chunkZ = chunk->getChunkZ();
        
        float distX = static_cast<float>(chunkX - playerChunkX);
        float distZ = static_cast<float>(chunkZ - playerChunkZ);
        float distance = std::sqrt(distX * distX + distZ * distZ);
        
        if (distance > unloadDistance) {
            // Save chunk before unloading if modified
            if (chunk->isModified()) {
                chunk->saveToDisk("");
            }
            chunksToRemove.push_back(key);
        }
    }
    
    // Remove distant chunks
    for (uint64_t key : chunksToRemove) {
        chunks.erase(key);
    }
}

uint8_t ChunkManager::getBlock(const glm::vec3& worldPos) {
    glm::ivec3 chunkCoords = worldToChunk(worldPos);
    glm::ivec3 localCoords = worldToLocal(worldPos);
    
    Chunk* chunk = getChunk(chunkCoords.x, chunkCoords.z);
    if (chunk == nullptr) {
        return 0; // Air
    }
    
    return chunk->getBlock(localCoords.x, localCoords.y, localCoords.z);
}

void ChunkManager::setBlock(const glm::vec3& worldPos, uint8_t blockID) {
    glm::ivec3 chunkCoords = worldToChunk(worldPos);
    glm::ivec3 localCoords = worldToLocal(worldPos);
    
    Chunk* chunk = getChunk(chunkCoords.x, chunkCoords.z);
    if (chunk == nullptr) {
        return;
    }
    
    chunk->setBlock(localCoords.x, localCoords.y, localCoords.z, blockID);
}

glm::ivec3 ChunkManager::worldToChunk(const glm::vec3& worldPos) const {
    return glm::ivec3(
        static_cast<int32_t>(std::floor(worldPos.x / config.chunkSize)),
        0,
        static_cast<int32_t>(std::floor(worldPos.z / config.chunkSize))
    );
}

glm::ivec3 ChunkManager::worldToLocal(const glm::vec3& worldPos) const {
    glm::ivec3 localPos(
        static_cast<int32_t>(std::floor(std::fmod(worldPos.x, config.chunkSize))),
        static_cast<int32_t>(worldPos.y),
        static_cast<int32_t>(std::floor(std::fmod(worldPos.z, config.chunkSize)))
    );
    
    // Handle negative coordinates
    if (localPos.x < 0) localPos.x += config.chunkSize;
    if (localPos.z < 0) localPos.z += config.chunkSize;
    
    return localPos;
}

void ChunkManager::processPendingGenerations() {
    // Pop tasks from pendingGeneration under lock
    std::vector<std::pair<int32_t, int32_t>> tasksToProcess;
    
    {
        std::lock_guard<std::mutex> lock(chunkMutex);
        
        const size_t maxPerFrame = 4; // Limit generations per frame
        size_t count = 0;
        
        auto it = pendingGeneration.begin();
        while (it != pendingGeneration.end() && count < maxPerFrame) {
            tasksToProcess.push_back(*it);
            it = pendingGeneration.erase(it);
            ++count;
        }
    }
    
    // Generate chunks without holding the lock
    for (const auto& [chunkX, chunkZ] : tasksToProcess) {
        auto chunk = std::make_unique<Chunk>(chunkX, chunkZ, config);
        chunk->generate(12345); // Use default seed
        
        // Reacquire lock only to insert the finished chunk
        uint64_t key = (static_cast<uint64_t>(static_cast<uint32_t>(chunkX)) << 32) | 
                       static_cast<uint32_t>(chunkZ);
        
        {
            std::lock_guard<std::mutex> lock(chunkMutex);
            chunks[key] = std::move(chunk);
        }
    }
}

void ChunkManager::buildChunkMeshes() {
    std::lock_guard<std::mutex> lock(chunkMutex);
    
    const size_t maxMeshBuildsPerFrame = 8;
    size_t built = 0;
    
    for (auto& [key, chunk] : chunks) {
        if (built >= maxMeshBuildsPerFrame) break;
        
        if (chunk->needsMeshUpdate()) {
            chunk->buildMesh();
            ++built;
        }
    }
}

void ChunkManager::updateVisibleChunks(const glm::vec3& cameraPosition) {
    std::lock_guard<std::mutex> lock(chunkMutex);
    
    visibleChunks.clear();
    
    int32_t cameraChunkX = static_cast<int32_t>(std::floor(cameraPosition.x / config.chunkSize));
    int32_t cameraChunkZ = static_cast<int32_t>(std::floor(cameraPosition.z / config.chunkSize));
    
    float maxViewDistance = config.lodDistances[3]; // Furthest LOD distance
    
    for (auto& [key, chunk] : chunks) {
        glm::vec3 chunkPos = chunk->getPosition();
        float chunkCenterX = chunkPos.x + config.chunkSize * 0.5f;
        float chunkCenterZ = chunkPos.z + config.chunkSize * 0.5f;
        
        float dx = chunkCenterX - cameraPosition.x;
        float dz = chunkCenterZ - cameraPosition.z;
        float distance = std::sqrt(dx * dx + dz * dz);
        
        // Distance culling
        if (distance > maxViewDistance) {
            continue;
        }
        
        // Frustum culling would be done here with camera frustum
        // For now, we just use distance culling
        
        visibleChunks.push_back(chunk.get());
    }
    
    // Sort visible chunks by distance for proper rendering order
    std::sort(visibleChunks.begin(), visibleChunks.end(),
        [cameraPosition](Chunk* a, Chunk* b) {
            glm::vec3 posA = a->getPosition();
            glm::vec3 posB = b->getPosition();
            
            // Use squared distance to avoid expensive sqrt operations
            float distA = glm::length2(posA - cameraPosition);
            float distB = glm::length2(posB - cameraPosition);
            
            return distA < distB;
        });
}

size_t ChunkManager::getLoadedChunkCount() const {
    return chunks.size();
}

size_t ChunkManager::getPendingGenerationCount() const {
    return pendingGeneration.size();
}

const std::vector<Chunk*>& ChunkManager::getVisibleChunks() const {
    return visibleChunks;
}

// ============================================================================
// LOD System Implementation
// ============================================================================

LODSystem& LODSystem::getInstance() {
    static LODSystem instance;
    return instance;
}

void LODSystem::initialize(const LODConfig& cfg) {
    config = cfg;
}

void LODSystem::update(const glm::vec3& cameraPosition) {
    currentCameraPosition = cameraPosition;
}

int32_t LODSystem::calculateLOD(float distance) const {
    for (int32_t i = 0; i < 4; ++i) {
        if (distance < config.lodDistances[i]) {
            return i;
        }
    }
    return 3; // Maximum LOD (lowest detail)
}

float LODSystem::getLODDistance(int32_t lodLevel) const {
    if (lodLevel < 0 || lodLevel >= 4) {
        return config.lodDistances[3];
    }
    return config.lodDistances[lodLevel];
}

void LODSystem::applyVegetationLOD(std::vector<VegetationInstance>& instances,
                                   const glm::vec3& cameraPosition) {
    for (auto& instance : instances) {
        float distance = glm::length(instance.position - cameraPosition);
        int32_t lod = calculateLOD(distance);
        
        // Adjust instance detail based on LOD
        instance.lodLevel = lod;
        
        // Could cull very distant instances or reduce their detail
        if (lod >= 3) {
            instance.visible = false; // Cull furthest vegetation
        } else {
            instance.visible = true;
        }
    }
}

// ============================================================================
// Culling System Implementation
// ============================================================================

CullingSystem& CullingSystem::getInstance() {
    static CullingSystem instance;
    return instance;
}

void CullingSystem::initialize(const CullingConfig& cfg) {
    config = cfg;
}

void CullingSystem::setFrustum(const glm::mat4& viewProjMatrix) {
    // Extract frustum planes from view-projection matrix
    // Plane equations: Ax + By + Cz + D = 0
    
    // Left plane
    frustumPlanes[0] = glm::vec4(
        viewProjMatrix[0][3] + viewProjMatrix[0][0],
        viewProjMatrix[1][3] + viewProjMatrix[1][0],
        viewProjMatrix[2][3] + viewProjMatrix[2][0],
        viewProjMatrix[3][3] + viewProjMatrix[3][0]
    );
    
    // Right plane
    frustumPlanes[1] = glm::vec4(
        viewProjMatrix[0][3] - viewProjMatrix[0][0],
        viewProjMatrix[1][3] - viewProjMatrix[1][0],
        viewProjMatrix[2][3] - viewProjMatrix[2][0],
        viewProjMatrix[3][3] - viewProjMatrix[3][0]
    );
    
    // Bottom plane
    frustumPlanes[2] = glm::vec4(
        viewProjMatrix[0][3] + viewProjMatrix[0][1],
        viewProjMatrix[1][3] + viewProjMatrix[1][1],
        viewProjMatrix[2][3] + viewProjMatrix[2][1],
        viewProjMatrix[3][3] + viewProjMatrix[3][1]
    );
    
    // Top plane
    frustumPlanes[3] = glm::vec4(
        viewProjMatrix[0][3] - viewProjMatrix[0][1],
        viewProjMatrix[1][3] - viewProjMatrix[1][1],
        viewProjMatrix[2][3] - viewProjMatrix[2][1],
        viewProjMatrix[3][3] - viewProjMatrix[3][1]
    );
    
    // Near plane
    frustumPlanes[4] = glm::vec4(
        viewProjMatrix[0][3] + viewProjMatrix[0][2],
        viewProjMatrix[1][3] + viewProjMatrix[1][2],
        viewProjMatrix[2][3] + viewProjMatrix[2][2],
        viewProjMatrix[3][3] + viewProjMatrix[3][2]
    );
    
    // Far plane
    frustumPlanes[5] = glm::vec4(
        viewProjMatrix[0][3] - viewProjMatrix[0][2],
        viewProjMatrix[1][3] - viewProjMatrix[1][2],
        viewProjMatrix[2][3] - viewProjMatrix[2][2],
        viewProjMatrix[3][3] - viewProjMatrix[3][2]
    );
    
    // Normalize planes
    for (int i = 0; i < 6; ++i) {
        float length = glm::length(glm::vec3(frustumPlanes[i]));
        if (length > 0.0001f) {
            frustumPlanes[i] /= length;
        }
    }
}

bool CullingSystem::isBoxInFrustum(const glm::vec3& min, const glm::vec3& max) const {
    for (int i = 0; i < 6; ++i) {
        // Test against each frustum plane
        glm::vec4 plane = frustumPlanes[i];
        
        // Find the most positive vertex for this plane
        glm::vec3 positiveVertex(
            plane.x > 0 ? max.x : min.x,
            plane.y > 0 ? max.y : min.y,
            plane.z > 0 ? max.z : min.z
        );
        
        float distance = plane.x * positiveVertex.x +
                        plane.y * positiveVertex.y +
                        plane.z * positiveVertex.z +
                        plane.w;
        
        if (distance < 0) {
            return false; // Outside frustum
        }
    }
    
    return true; // Inside or intersecting frustum
}

bool CullingSystem::isSphereInFrustum(const glm::vec3& center, float radius) const {
    for (int i = 0; i < 6; ++i) {
        glm::vec4 plane = frustumPlanes[i];
        
        float distance = plane.x * center.x +
                        plane.y * center.y +
                        plane.z * center.z +
                        plane.w;
        
        if (distance < -radius) {
            return false; // Outside frustum
        }
    }
    
    return true; // Inside or intersecting frustum
}

bool CullingSystem::shouldCullByDistance(const glm::vec3& objectPosition,
                                         const glm::vec3& cameraPosition,
                                         float maxDistance) const {
    float distance = glm::length(objectPosition - cameraPosition);
    return distance > maxDistance;
}

CullingResult CullingSystem::cullChunks(const std::vector<Chunk*>& chunks,
                                        const glm::vec3& cameraPosition,
                                        float maxDistance) {
    CullingResult result;
    
    for (Chunk* chunk : chunks) {
        if (!chunk || !chunk->isLoaded()) {
            continue;
        }
        
        glm::vec3 chunkPos = chunk->getPosition();
        float chunkSize = static_cast<float>(config.chunkSize);
        float chunkHeight = static_cast<float>(config.chunkHeight);
        
        // Create bounding box for chunk
        glm::vec3 minBound(chunkPos.x, 0, chunkPos.z);
        glm::vec3 maxBound(chunkPos.x + chunkSize, chunkHeight, chunkPos.z + chunkSize);
        
        // Distance culling
        glm::vec3 chunkCenter = chunkPos + glm::vec3(chunkSize * 0.5f, chunkHeight * 0.5f, chunkSize * 0.5f);
        if (shouldCullByDistance(chunkCenter, cameraPosition, maxDistance)) {
            result.culled++;
            continue;
        }
        
        // Frustum culling
        if (!isBoxInFrustum(minBound, maxBound)) {
            result.culled++;
            continue;
        }
        
        result.visible.push_back(chunk);
        result.visibleCount++;
    }
    
    return result;
}

void CullingSystem::cullVegetation(std::vector<VegetationInstance>& instances,
                                   const glm::vec3& cameraPosition,
                                   float maxDistance) {
    size_t writeIndex = 0;
    
    for (size_t i = 0; i < instances.size(); ++i) {
        VegetationInstance& inst = instances[i];
        
        // Distance culling
        if (shouldCullByDistance(inst.position, cameraPosition, maxDistance)) {
            inst.visible = false;
            continue;
        }
        
        // Frustum culling (using point test for simplicity)
        if (!isSphereInFrustum(inst.position, inst.scale)) {
            inst.visible = false;
            continue;
        }
        
        inst.visible = true;
        
        // Compact the array
        if (writeIndex != i) {
            instances[writeIndex] = inst;
        }
        writeIndex++;
    }
    
    instances.resize(writeIndex);
    // (Keep them in the vector but mark as not visible)
}

} // namespace FarmEngine
