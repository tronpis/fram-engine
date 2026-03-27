#include "Window.h"
#include "core/logger/Logger.h"

namespace farm {

Window::Window() = default;

Window::~Window() {
    if (m_initialized) {
        shutdown();
    }
}

bool Window::init(const std::string& title, int width, int height) {
    if (m_initialized) {
        FARM_LOG_WARN("Window already initialized");
        return true;
    }

    FARM_LOG_INFO("Creating window: {} ({}x{})", title, width, height);

    // TODO: Implement GLFW initialization
    // For now, just set the dimensions and mark as initialized
    m_width = width;
    m_height = height;
    m_initialized = true;

    FARM_LOG_INFO("Window created successfully (stub)");
    return true;
}

void Window::shutdown() {
    if (!m_initialized) {
        return;
    }

    FARM_LOG_INFO("Shutting down window...");

    // TODO: Implement GLFW cleanup
    // For now, just reset state
    m_handle = nullptr;
    m_width = 0;
    m_height = 0;
    m_initialized = false;

    FARM_LOG_INFO("Window shutdown complete");
}

void Window::pollEvents() {
    if (!m_initialized) {
        return;
    }

    // TODO: Implement GLFW event polling
    // glfwPollEvents();
}

bool Window::shouldClose() const {
    if (!m_initialized) {
        return true;
    }

    // TODO: Implement GLFW window close check
    // return glfwWindowShouldClose(m_handle);
    return false;
}

} // namespace farm
