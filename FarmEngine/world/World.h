#pragma once

namespace farm {

class Renderer;

class World {
public:
    bool init() { return true; }
    void shutdown() {}
    void update(float dt) {}
    void render(Renderer& renderer) {}
};

} // namespace farm
