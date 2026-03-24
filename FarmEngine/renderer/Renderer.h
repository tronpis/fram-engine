#pragma once

namespace farm {

class Window;

class Renderer {
public:
    bool init(Window& window) { return true; }
    void shutdown() {}
    bool beginFrame() { return true; }
    void endFrame() {}
};

} // namespace farm
