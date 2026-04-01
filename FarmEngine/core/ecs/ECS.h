#pragma once

#include <vector>
#include <unordered_map>
#include <memory>
#include <typeindex>
#include <bitset>
#include <array>

namespace FarmEngine {

constexpr size_t MAX_ENTITIES = 10000;
constexpr size_t MAX_COMPONENTS = 64;

using EntityID = uint32_t;
using ComponentMask = std::bitset<MAX_COMPONENTS>;

class Component {
public:
    virtual ~Component() = default;
    EntityID entityID = UINT32_MAX;
};

class Entity {
public:
    EntityID id;
    ComponentMask mask;
    bool active = true;
    
    Entity() : id(UINT32_MAX) {}
    Entity(EntityID entityId) : id(entityId) {}
};

class ECS {
public:
    ECS();
    ~ECS();
    
    EntityID createEntity();
    void destroyEntity(EntityID entity);
    
    template<typename T, typename... Args>
    T& addComponent(EntityID entity, Args&&... args);
    
    template<typename T>
    T* getComponent(EntityID entity);
    
    template<typename T>
    void removeComponent(EntityID entity);
    
    template<typename... Components>
    std::vector<EntityID> getEntitiesWithComponents();
    
    void update(float deltaTime);
    
private:
    std::vector<Entity> entities;
    std::vector<EntityID> availableIDs;
    std::vector<std::vector<std::unique_ptr<Component>>> components;
    std::unordered_map<std::type_index, uint32_t> componentTypeToIndex;
    
    uint32_t nextComponentIndex = 0;
    
    template<typename T>
    uint32_t getComponentTypeIndex();
};

// Template implementations
template<typename T, typename... Args>
T& ECS::addComponent(EntityID entity, Args&&... args) {
    if (entity >= entities.size() || !entities[entity].active) {
        throw std::out_of_range("Invalid or inactive entity ID");
    }
    
    auto index = getComponentTypeIndex<T>();
    
    if (index >= components.size()) {
        components.resize(index + 1);
    }
    
    while (components[index].size() <= entity) {
        components[index].push_back(nullptr);
    }
    
    auto component = std::make_unique<T>(std::forward<Args>(args)...);
    component->entityID = entity;
    T* ptr = component.get();
    components[index][entity] = std::move(component);
    
    entities[entity].mask.set(index);
    
    return *ptr;
}

template<typename T>
T* ECS::getComponent(EntityID entity) {
    if (entity >= entities.size() || !entities[entity].active) {
        return nullptr;
    }
    
    auto index = getComponentTypeIndex<T>();
    
    if (index >= components.size()) {
        return nullptr;
    }
    
    if (entity >= components[index].size()) {
        return nullptr;
    }
    
    return static_cast<T*>(components[index][entity].get());
}

template<typename T>
void ECS::removeComponent(EntityID entity) {
    if (entity >= entities.size() || !entities[entity].active) {
        return;
    }
    
    auto index = getComponentTypeIndex<T>();
    
    if (index < components.size() && entity < components[index].size()) {
        components[index][entity].reset();
        entities[entity].mask.reset(index);
    }
}

template<typename T>
uint32_t ECS::getComponentTypeIndex() {
    auto key = std::type_index(typeid(T));
    
    auto it = componentTypeToIndex.find(key);
    if (it != componentTypeToIndex.end()) {
        return it->second;
    }
    
    if (nextComponentIndex >= MAX_COMPONENTS) {
        throw std::overflow_error("Maximum number of component types exceeded");
    }
    
    uint32_t index = nextComponentIndex++;
    componentTypeToIndex[key] = index;
    return index;
}

template<typename... Components>
std::vector<EntityID> ECS::getEntitiesWithComponents() {
    std::vector<EntityID> result;
    ComponentMask requiredMask;
    
    ((requiredMask.set(getComponentTypeIndex<Components>()), ...));
    
    for (const auto& entity : entities) {
        if (entity.active && (entity.mask & requiredMask) == requiredMask) {
            result.push_back(entity.id);
        }
    }
    
    return result;
}

} // namespace FarmEngine
