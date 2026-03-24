#pragma once

namespace farm {

class PhysicsWorld {
public:
    bool init() { return true; }
    void shutdown() {}
    void update(float dt) {}
};

} // namespace farm
