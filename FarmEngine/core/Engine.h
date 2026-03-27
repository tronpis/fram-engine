#pragma once

#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace farm {

// Forward declarations
class Window;
class Renderer;
class World;
class Simulation;
class PhysicsWorld;
class AudioSystem;
class Editor;
class JobSystem;
class IPlugin;

/**
 * @brief Main Engine class - orchestrates all subsystems
 * 
 * This is the heart of FarmEngine. It manages initialization,
 * the main game loop, and coordination between all systems.
 * 
 * Usage:
 * @code
 * Engine engine;
 * engine.init();
 * engine.run();
 * engine.shutdown();
 * @endcode
 */
class Engine {
public:
    Engine();
    ~Engine();
    
    // Prevent copying
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    
    /**
     * @brief Initialize the engine and all subsystems
     * @param config Configuration file path (optional)
     * @return true if initialization successful
     */
    bool init(const std::string& config = "");
    
    /**
     * @brief Run the main game loop
     */
    void run();
    
    /**
     * @brief Stop the engine and exit the main loop
     */
    void stop();
    
    /**
     * @brief Shutdown the engine and cleanup all resources
     */
    void shutdown();
    
    /**
     * @brief Check if engine is running
     */
    bool isRunning() const { return m_running; }
    
    /**
     * @brief Get delta time from last frame
     */
    float getDeltaTime() const { return m_deltaTime; }
    
    /**
     * @brief Get current time in seconds
     */
    double getTime() const { return m_time; }
    
    // System accessors
    Renderer* getRenderer() { return m_renderer.get(); }
    World* getWorld() { return m_world.get(); }
    Simulation* getSimulation() { return m_simulation.get(); }
    PhysicsWorld* getPhysics() { return m_physics.get(); }
    AudioSystem* getAudio() { return m_audio.get(); }
    JobSystem* getJobSystem() { return m_jobSystem.get(); }
    
#ifdef FARMENGINE_WITH_EDITOR
    Editor* getEditor() { return m_editor.get(); }
#endif
    
    /**
     * @brief Register a plugin module
     */
    void registerPlugin(const std::string& name, std::unique_ptr<IPlugin> plugin);
    
    /**
     * @brief Queue a function to be called on next update
     */
    void queueUpdate(std::function<void()> func);
    
private:
    void processInput();
    void update();
    void render();
    void fixedUpdate();
    
    // Core state
    bool m_running = false;
    bool m_initialized = false;
    float m_deltaTime = 0.0f;
    double m_time = 0.0;
    double m_fixedTimeAccumulator = 0.0;
    const double m_fixedTimestep = 1.0 / 60.0;  // 60 Hz physics
    
    // Subsystems
    std::unique_ptr<Window> m_window;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<World> m_world;
    std::unique_ptr<Simulation> m_simulation;
    std::unique_ptr<PhysicsWorld> m_physics;
    std::unique_ptr<AudioSystem> m_audio;
    std::unique_ptr<JobSystem> m_jobSystem;
    
#ifdef FARMENGINE_WITH_EDITOR
    std::unique_ptr<Editor> m_editor;
#endif
    
    // Pending update functions
    std::vector<std::function<void()>> m_pendingUpdates;
    
    // Registered plugins
    std::vector<std::unique_ptr<IPlugin>> m_plugins;
};

} // namespace farm
