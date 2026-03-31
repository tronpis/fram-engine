#pragma once

#include <string>

namespace farm {

/**
 * @brief Window class for creating and managing the application window
 */
class Window {
public:
    Window();
    ~Window();

    bool init(const std::string& title, int width, int height);
    void shutdown();

    void pollEvents();
    bool shouldClose() const;

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

private:
    void* m_handle = nullptr;  // GLFWwindow* would go here
    int m_width = 0;
    int m_height = 0;
    bool m_initialized = false;
};

} // namespace farm
