#pragma once

namespace farm {

class World;

class Simulation {
public:
    bool init(World& world) { return true; }
    void shutdown() {}
    void update(float dt) {}
    void fixedUpdate(float dt) {}
};

} // namespace farm
