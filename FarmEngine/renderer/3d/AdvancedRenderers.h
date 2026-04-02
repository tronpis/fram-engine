#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <memory>
#include <glm/glm.hpp>

namespace FarmEngine {

// ============================================================================
// PARTICLE SYSTEM - GPU Based
// ============================================================================

struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec4 color;        // current color
    glm::vec4 startColor;   // initial color
    float lifetime;
    float maxLifetime;
    float size;
    float rotation;
    float angularVelocity;
    uint32_t type;
    uint32_t padding[3];
};

struct ParticleEmitterConfig {
    std::string name;
    
    // Emission
    float emissionRate = 10.0f;       // particles per second
    uint32_t maxParticles = 10000;
    bool looping = true;
    float duration = 0.0f;            // 0 = infinite
    
    // Initial state
    glm::vec3 position = glm::vec3(0);
    glm::vec3 direction = glm::vec3(0, 1, 0);
    float spreadAngle = 45.0f;
    glm::vec3 initialVelocityMin = glm::vec3(-1);
    glm::vec3 initialVelocityMax = glm::vec3(1);
    
    // Lifetime
    float minLifetime = 1.0f;
    float maxLifetime = 3.0f;
    
    // Size
    float minSize = 0.1f;
    float maxSize = 1.0f;
    glm::vec2 sizeOverLifetime[5];    // curve points
    
    // Color
    glm::vec4 colorOverLifetime[5];   // curve points
    
    // Forces
    glm::vec3 gravity = glm::vec3(0, -9.81f, 0);
    float drag = 0.0f;
    
    // Texture
    uint32_t textureIndex = 0;
    bool useTextureSheet = false;
    uint32_t sheetRows = 1;
    uint32_t sheetColumns = 1;
    float frameRate = 30.0f;
};

class ParticleSystem {
public:
    ParticleSystem(VkDevice device, VkPhysicalDevice physicalDevice);
    ~ParticleSystem();
    
    bool initialize(size_t maxTotalParticles = 100000);
    void cleanup();
    
    // Emitters
    uint32_t createEmitter(const ParticleEmitterConfig& config);
    void destroyEmitter(uint32_t emitterId);
    void setEmitterPosition(uint32_t emitterId, const glm::vec3& pos);
    void setEmitterEnabled(uint32_t emitterId, bool enabled);
    
    // Simulation
    void update(float deltaTime);
    void simulate(VkCommandBuffer commandBuffer, float deltaTime);
    
    // Rendering
    void render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout,
               const glm::mat4& view, const glm::mat4& projection);
    
    // Stats
    uint32_t getActiveParticleCount() const { return activeParticleCount; }
    uint32_t getEmitterCount() const { return static_cast<uint32_t>(emitters.size()); }
    
private:
    void createBuffers();
    void createPipeline();
    void createDescriptorSets();
    void dispatchCompute(VkCommandBuffer commandBuffer, uint32_t particleCount);
    
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    
    // Buffers (double buffered for simulation)
    VkBuffer particleBuffer[2] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkDeviceMemory particleBufferMemory[2] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkBuffer emitBuffer = VK_NULL_HANDLE;
    VkDeviceMemory emitBufferMemory = VK_NULL_HANDLE;
    
    // Compute
    VkPipeline computePipeline = VK_NULL_HANDLE;
    VkPipelineLayout computePipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout computeDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet computeDescriptorSet = VK_NULL_HANDLE;
    
    // Render
    VkPipeline renderPipeline = VK_NULL_HANDLE;
    VkPipelineLayout renderPipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSet renderDescriptorSet = VK_NULL_HANDLE;
    
    // State
    std::vector<ParticleEmitterConfig> emitters;
    uint32_t activeParticleCount = 0;
    uint32_t maxParticles;
    uint32_t currentReadBuffer = 0;
    uint32_t currentWriteBuffer = 1;
    
    bool initialized = false;
};

// ============================================================================
// INSTANCED RENDERER - For vegetation (100k+ instances)
// ============================================================================

struct InstanceData {
    glm::mat4 transform;
    glm::vec4 color;          // tint
    glm::vec4 uvOffset;       // for texture atlas
    uint32_t lodLevel;
    uint32_t windType;
    float windIntensity;
    float padding;
};

struct VegetationConfig {
    std::string name;
    class Mesh* mesh;
    struct Material material;

    
    // LOD
    float lodDistances[4] = {20, 50, 100, 200};
    uint32_t lodCount = 4;
    
    // Wind animation
    float windSpeed = 1.0f;
    float windAmplitude = 0.1f;
    
    // Culling
    float cullDistance = 500.0f;
    bool useFrustumCulling = true;
    bool useOcclusionCulling = true;
};

class InstancedRenderer {
public:
    InstancedRenderer(VkDevice device, VkPhysicalDevice physicalDevice);
    ~InstancedRenderer();
    
    bool initialize(size_t maxInstancesPerType = 100000);
    void cleanup();
    
    // Vegetation types
    uint32_t registerVegetationType(const VegetationConfig& config);
    
    // Instance management
    void addInstance(uint32_t vegetationType, const InstanceData& data);
    void addInstances(uint32_t vegetationType, const std::vector<InstanceData>& instances);
    void clearInstances(uint32_t vegetationType);
    void clearAllInstances();
    
    // Batch optimization (GPU instancing + indirect drawing)
    void buildBatches();
    
    // Rendering
    void updateBuffers(VkCommandBuffer commandBuffer);
    void render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout,
               const glm::mat4& view, const glm::mat4& projection);
    
    // Frustum culling (GPU via compute shader)
    void performFrustumCulling(VkCommandBuffer commandBuffer, 
                               const glm::mat4& viewProjection);
    
    // Stats
    uint32_t getTotalInstanceCount() const { return totalInstanceCount; }
    uint32_t getVisibleInstanceCount() const { return visibleInstanceCount; }
    uint32_t getVegetationTypeCount() const { return static_cast<uint32_t>(vegetationTypes.size()); }
    
private:
    void createBuffers();
    void createIndirectBuffers();
    void createPipeline();
    void createCullingPipeline();
    
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    
    // Vegetation types
    struct VegetationType {
        VegetationConfig config;
        Mesh* mesh;
        VkPipeline pipeline;
        uint32_t instanceCount;
        uint32_t maxInstances;
    };
    std::vector<VegetationType> vegetationTypes;
    
    // Instance buffers (per type)
    std::vector<VkBuffer> instanceBuffers;
    std::vector<VkDeviceMemory> instanceBufferMemories;
    std::vector<InstanceData*> mappedInstances;
    
    // Indirect draw buffers
    VkBuffer indirectDrawBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indirectDrawBufferMemory = VK_NULL_HANDLE;
    VkBuffer visibilityBuffer = VK_NULL_HANDLE;
    VkDeviceMemory visibilityBufferMemory = VK_NULL_HANDLE;
    
    // Compute pipelines for culling
    VkPipeline cullingPipeline = VK_NULL_HANDLE;
    VkPipelineLayout cullingPipelineLayout = VK_NULL_HANDLE;
    
    // Pipelines
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    
    uint32_t totalInstanceCount = 0;
    uint32_t visibleInstanceCount = 0;
    
    bool needsRebuild = true;
};

// ============================================================================
// SKINNED MESH RENDERER - For animated characters/animals
// ============================================================================

constexpr uint32_t MAX_JOINTS = 128;
constexpr uint32_t MAX_SKINNING_WEIGHTS = 4;

struct VertexSkinned {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::vec4 tangent;
    
    // Skinning data
    int32_t jointIndices[MAX_SKINNING_WEIGHTS];
    float weights[MAX_SKINNING_WEIGHTS];
    
    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 6> getAttributeDescriptions();
};

struct Joint {
    glm::mat4 inverseBindMatrix;
    glm::mat4 localTransform;
    glm::mat4 globalTransform;
    int32_t parentIndex = -1;
};

struct AnimationClip {
    std::string name;
    float duration;
    float ticksPerSecond;
    
    struct Keyframe {
        float time;
        glm::vec3 position;
        glm::quat rotation;
        glm::vec3 scale;
    };
    
    struct JointAnimation {
        std::vector<Keyframe> keyframes;
    };
    
    std::vector<JointAnimation> jointAnimations;
};

struct Skeleton {
    std::vector<Joint> joints;
    std::vector<AnimationClip> animations;
    uint32_t rootJoint = 0;
    
    // Current pose
    std::vector<glm::mat4> jointMatrices;
    
    void updatePose(float time, const std::string& animation);
    void blendPoses(const std::vector<float>& weights, 
                   const std::vector<std::string>& animations);
};

struct SkinnedMeshData {
    std::vector<VertexSkinned> vertices;
    std::vector<uint32_t> indices;
    Skeleton skeleton;
    std::vector<Material> materials;
};

class SkinnedMesh {
public:
    SkinnedMesh(VkDevice device, VkPhysicalDevice physicalDevice);
    ~SkinnedMesh();
    
    bool initialize(const SkinnedMeshData& data);
    bool loadFromFile(const std::string& path);  // glTF with skinning
    void cleanup();
    
    void bind(VkCommandBuffer commandBuffer);
    void draw(VkCommandBuffer commandBuffer);
    
    // Animation
    void playAnimation(const std::string& name, float speed = 1.0f, bool loop = true);
    void crossfadeToAnimation(const std::string& name, float duration = 0.2f);
    void updateAnimation(float deltaTime);
    
    // Accessors
    Skeleton& getSkeleton() { return skeleton; }
    uint32_t getJointCount() const { return static_cast<uint32_t>(skeleton.joints.size()); }
    uint32_t getIndexCount() const { return static_cast<uint32_t>(indices.size()); }
    
    // Uniform buffer for joint matrices
    VkBuffer getJointBuffer() const { return jointBuffer; }
    void updateJointBuffer(VkCommandBuffer commandBuffer);
    
private:
    void createVertexBuffers(const std::vector<VertexSkinned>& vertices);
    void createIndexBuffers(const std::vector<uint32_t>& indices);
    void createJointBuffer();
    
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    
    std::vector<VertexSkinned> vertices;
    std::vector<uint32_t> indices;
    Skeleton skeleton;
    
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
    VkBuffer jointBuffer = VK_NULL_HANDLE;
    VkDeviceMemory jointBufferMemory = VK_NULL_HANDLE;
    
    // Animation state
    std::string currentAnimation;
    std::string nextAnimation;
    float currentTime = 0.0f;
    float crossfadeTime = 0.0f;
    float crossfadeDuration = 0.0f;
    float animationSpeed = 1.0f;
    bool looping = true;
};

class SkinnedMeshRenderer {
public:
    SkinnedMeshRenderer(VkDevice device, VkPhysicalDevice physicalDevice);
    ~SkinnedMeshRenderer();
    
    bool initialize();
    void cleanup();
    
    // Character management
    uint32_t addCharacter(SkinnedMesh* mesh, const glm::mat4& initialTransform);
    void removeCharacter(uint32_t characterId);
    
    SkinnedMesh* getCharacter(uint32_t characterId);
    
    // Rendering
    void updateAnimations(float deltaTime);
    void updateJointBuffers(VkCommandBuffer commandBuffer);
    void render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout,
               const glm::mat4& view, const glm::mat4& projection);
    
    // Stats
    uint32_t getCharacterCount() const { return static_cast<uint32_t>(characters.size()); }
    
private:
    void createPipeline();
    void createDescriptorSets();
    
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    
    struct Character {
        SkinnedMesh* mesh;
        glm::mat4 transform;
        uint32_t id;
    };
    
    std::vector<Character> characters;
    uint32_t nextCharacterId = 0;
    
    // Pipeline
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
};

} // namespace FarmEngine
