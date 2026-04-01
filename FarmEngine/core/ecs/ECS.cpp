#include "ECS.h"

namespace FarmEngine {

ECS::ECS() {
    entities.reserve(MAX_ENTITIES);
}

ECS::~ECS() = default;

EntityID ECS::createEntity() {
    EntityID id;
    
    if (!availableIDs.empty()) {
        id = availableIDs.back();
        availableIDs.pop_back();
        entities[id].active = true;
    } else {
        id = static_cast<EntityID>(entities.size());
        entities.emplace_back(id);
    }
    
    return id;
}

void ECS::destroyEntity(EntityID entity) {
    if (entity >= entities.size()) return;
    
    entities[entity].active = false;
    entities[entity].mask.reset();
    
    // Limpiar componentes
    for (auto& componentList : components) {
        if (componentList.second && entity < componentList.second->size()) {
            (*componentList.second)[entity].reset();
        }
    }
    
    availableIDs.push_back(entity);
}

void ECS::update(float deltaTime) {
    // Actualizar sistemas basados en ECS
    (void)deltaTime;
}

} // namespace FarmEngine
