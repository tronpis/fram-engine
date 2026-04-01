#pragma once

#include <vector>
#include <string>
#include <memory>

namespace FarmEngine {

struct EntityInfo {
    uint32_t entityID;
    std::string name;
    std::string type;
    bool active;
};

struct ComponentInfo {
    std::string name;
    std::string value;
    bool editable;
};

class Inspector {
public:
    static Inspector& getInstance();
    
    void initialize();
    void cleanup();
    
    // Selección
    void selectEntity(uint32_t entityID);
    void deselectEntity();
    uint32_t getSelectedEntity() const { return selectedEntity; }
    
    // Información
    EntityInfo getEntityInfo(uint32_t entityID) const;
    std::vector<ComponentInfo> getComponents(uint32_t entityID) const;
    
    // Edición
    bool editComponent(uint32_t entityID, const std::string& componentName,
                      const std::string& newValue);
    
    // Añadir/Remover componentes
    bool addComponent(uint32_t entityID, const std::string& componentType);
    bool removeComponent(uint32_t entityID, const std::string& componentType);
    
    // Callbacks para UI
    using SelectionCallback = std::function<void(uint32_t)>;
    void setSelectionCallback(SelectionCallback callback);
    
private:
    Inspector() = default;
    ~Inspector();
    
    uint32_t selectedEntity = 0;
    SelectionCallback selectionCallback;
};

class EntityTree {
public:
    static EntityTree& getInstance();
    
    void initialize();
    void cleanup();
    
    // Navegación
    struct TreeNode {
        uint32_t entityID;
        std::string name;
        std::vector<uint32_t> children;
        uint32_t parent = 0;
        bool expanded = true;
    };
    
    const std::vector<TreeNode>& getRootNodes() const { return rootNodes; }
    const TreeNode* getNode(uint32_t entityID) const;
    
    // Modificación
    void addNode(uint32_t entityID, const std::string& name, 
                uint32_t parentID = 0);
    void removeNode(uint32_t entityID);
    void reorderNode(uint32_t entityID, uint32_t newParentID, int32_t index);
    
    // Búsqueda
    std::vector<uint32_t> searchByName(const std::string& query) const;
    std::vector<uint32_t> searchByType(const std::string& type) const;
    
    // Filtros
    void setFilter(const std::string& filter);
    void clearFilter();
    
private:
    EntityTree() = default;
    
    std::vector<TreeNode> rootNodes;
    std::unordered_map<uint32_t, TreeNode> nodes;
    std::string currentFilter;
};

// Herramientas de debugging
class DebugTools {
public:
    static DebugTools& getInstance();
    
    void initialize();
    void cleanup();
    
    // FPS Counter
    float getFPS() const { return fps; }
    float getFrameTime() const { return frameTime; }
    
    // Memory stats
    size_t getTotalMemoryUsage() const;
    size_t getAssetMemoryUsage() const;
    
    // Render stats
    uint32_t getDrawCalls() const { return drawCalls; }
    uint32_t getTriangleCount() const { return triangleCount; }
    uint32_t getEntityCount() const { return entityCount; }
    
    // Profiling
    void beginProfile(const std::string& name);
    void endProfile(const std::string& name);
    std::vector<std::pair<std::string, float>> getProfileResults() const;
    
    // Console commands
    using CommandCallback = std::function<void(const std::vector<std::string>&)>;
    void registerCommand(const std::string& name, CommandCallback callback,
                        const std::string& description = "");
    void executeCommand(const std::string& commandLine);
    
    // Log
    void addLogMessage(const std::string& message, int32_t level = 0);
    const std::vector<std::pair<int32_t, std::string>>& getLogs() const;
    
    void update(float deltaTime);
    
private:
    DebugTools() = default;
    
    float fps = 0.0f;
    float frameTime = 0.0f;
    float frameAccumulator = 0.0f;
    int32_t frameCount = 0;
    
    uint32_t drawCalls = 0;
    uint32_t triangleCount = 0;
    uint32_t entityCount = 0;
    
    struct ProfileEntry {
        std::string name;
        float startTime;
        float totalTime;
        int32_t callCount;
    };
    
    std::unordered_map<std::string, ProfileEntry> profileData;
    std::vector<std::pair<std::string, float>> profileResults;
    
    std::unordered_map<std::string, CommandCallback> commands;
    std::vector<std::pair<int32_t, std::string>> logs;
    
    static const size_t maxLogs = 1000;
};

// Widget base para herramientas
class ToolWidget {
public:
    virtual ~ToolWidget() = default;
    virtual void render() = 0;
    virtual void update(float deltaTime) {}
    virtual bool isActive() const { return active; }
    virtual void setActive(bool value) { active = value; }
    
protected:
    bool active = true;
};

} // namespace FarmEngine
