#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <memory>
#include <glm/glm.hpp>

namespace FarmEngine {

struct Vertex3D {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::vec4 tangent;
    
    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions();
};

struct MeshData {
    std::vector<Vertex3D> vertices;
    std::vector<uint32_t> indices;
};

class Mesh {
public:
    Mesh(VkDevice device, VkPhysicalDevice physicalDevice);
    ~Mesh();
    
    bool initialize(const MeshData& data);
    void cleanup();
    
    void bind(VkCommandBuffer commandBuffer);
    void draw(VkCommandBuffer commandBuffer, uint32_t instanceCount = 1);
    
    uint32_t getIndexCount() const { return static_cast<uint32_t>(indices.size()); }
    uint32_t getVertexCount() const { return static_cast<uint32_t>(vertices.size()); }
    
private:
    void createVertexBuffers(const std::vector<Vertex3D>& vertices);
    void createIndexBuffers(const std::vector<uint32_t>& indices);
    
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    
    std::vector<Vertex3D> vertices;
    std::vector<uint32_t> indices;
    
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
};

// Terrain mesh generation
struct TerrainConfig {
    uint32_t width;
    uint32_t depth;
    float scale;
    float heightScale;
    int32_t seed;
    std::string heightmapPath;
};

class TerrainMesh {
public:
    TerrainMesh(VkDevice device, VkPhysicalDevice physicalDevice);
    ~TerrainMesh();
    
    bool generate(const TerrainConfig& config);
    void updateHeightmap(const std::vector<float>& heights);
    
    Mesh* getMesh() { return mesh.get(); }
    
    // LOD
    void setLODDistance(float distance);
    void updateLOD(const glm::vec3& cameraPosition);
    
    uint32_t getWidth() const { return config.width; }
    uint32_t getDepth() const { return config.depth; }
    
private:
    void generateVertices();
    void calculateNormals();
    void generateIndices();
    void generateLODMeshes();
    
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    
    TerrainConfig config;
    std::unique_ptr<Mesh> mesh;
    std::vector<Mesh> lodMeshes;
    
    std::vector<float> heightmap;
    std::vector<Vertex3D> vertices;
    std::vector<uint32_t> indices;
    
    float lodDistance = 50.0f;
    int32_t currentLOD = 0;
};

// Model loading (glTF/OBJ)
struct Material {
    glm::vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    uint32_t baseColorTexture;
    uint32_t normalTexture;
    uint32_t materialTexture;
};

class Model {
public:
    Model(VkDevice device, VkPhysicalDevice physicalDevice);
    ~Model();
    
    bool loadFromFile(const std::string& path);
    bool loadFromData(const void* data, size_t size);
    void cleanup();
    
    void render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
    
    const std::vector<std::unique_ptr<Mesh>>& getMeshes() const { return meshes; }
    const std::vector<Material>& getMaterials() const { return materials; }
    
private:
    bool loadGLTF(const std::string& path);
    bool loadOBJ(const std::string& path);
    
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    
    std::vector<std::unique_ptr<Mesh>> meshes;
    std::vector<Material> materials;
    std::vector<VkDescriptorSet> descriptorSets;
};

// Renderer 3D principal
class Renderer3D {
public:
    static Renderer3D& getInstance();
    
    bool initialize(VkDevice device, VkPhysicalDevice physicalDevice);
    void cleanup();
    
    // Rendering
    void beginFrame();
    void submitMesh(const Mesh* mesh, const glm::mat4& transform, 
                   const Material* material = nullptr);
    void submitTerrain(const TerrainMesh* terrain);
    void endFrame(VkCommandBuffer commandBuffer);
    
    // Camera
    void setViewProjection(const glm::mat4& view, const glm::mat4& projection);
    
    // Stats
    uint32_t getDrawCalls() const { return drawCallCount; }
    uint32_t getTriangleCount() const { return triangleCount; }
    
private:
    void createPipeline();
    void createDescriptorSets();
    void flush();
    
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    
    struct DrawCall {
        const Mesh* mesh;
        glm::mat4 transform;
        const Material* material;
    };
    
    std::vector<DrawCall> drawCalls;
    std::vector<const TerrainMesh*> terrains;
    
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    
    uint32_t drawCallCount = 0;
    uint32_t triangleCount = 0;
};

} // namespace FarmEngine
