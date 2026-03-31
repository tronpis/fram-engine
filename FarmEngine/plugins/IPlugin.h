#pragma once

namespace farm {

// Base plugin interface (stub)
class IPlugin {
public:
    virtual ~IPlugin() = default;
    virtual bool init() { return true; }
    virtual void shutdown() {}
};

} // namespace farm
