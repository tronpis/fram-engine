#pragma once

namespace farm {

class Engine;

class Editor {
public:
    explicit Editor(Engine& engine) {}
    bool init() { return true; }
    void shutdown() {}
    bool isActive() const { return false; }
    void processInput() {}
    void update(float dt) {}
    void render(class Renderer& renderer) {}
};

} // namespace farm
