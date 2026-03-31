#include "core/Engine.h"
#include "core/logger/Logger.h"
#include "plugins/IPlugin.h"
#include "platform/window/Window.h"
#include "renderer/Renderer.h"
#include "world/World.h"
#include "simulation/Simulation.h"
#include "physics/PhysicsWorld.h"
#include "audio/AudioSystem.h"
#include "core/jobsystem/JobSystem.h"

#ifdef FARMENGINE_WITH_EDITOR
#include "tools/editor/Editor.h"
#endif

namespace farm {

Engine::Engine() {
    // Logger must be initialized before any logging
    Logger::init();
    FARM_LOG_INFO("FarmEngine initializing...");
}

Engine::~Engine() {
    if (m_initialized) {
        shutdown();
    }
}

bool Engine::init(const std::string& config) {
    if (m_initialized) {
        FARM_LOG_WARN("Engine already initialized");
        return true;
    }
    
    FARM_LOG_INFO("Initializing engine subsystems...");
    
    bool initializationFailed = false;
    
    // Create job system for multithreading
    m_jobSystem = std::make_unique<JobSystem>();
    if (!m_jobSystem->init()) {
        FARM_LOG_ERROR("Failed to initialize job system");
        initializationFailed = true;
    }
    
    // Create window
    m_window = std::make_unique<Window>();
    if (!m_window->init("FarmEngine", 1920, 1080)) {
        FARM_LOG_ERROR("Failed to create window");
        m_window.reset();  // Clean up invalid window immediately
        initializationFailed = true;
    }
    
    // Create renderer (only if window succeeded)
    if (!initializationFailed) {
        m_renderer = std::make_unique<Renderer>();
        if (!m_renderer->init(*m_window)) {
            FARM_LOG_ERROR("Failed to initialize renderer");
            initializationFailed = true;
        }
    }
    
    // Create world (only if previous steps succeeded)
    if (!initializationFailed) {
        m_world = std::make_unique<World>();
        if (!m_world->init()) {
            FARM_LOG_ERROR("Failed to initialize world");
            initializationFailed = true;
        }
    }
    
    // Create simulation (only if previous steps succeeded)
    if (!initializationFailed) {
        m_simulation = std::make_unique<Simulation>();
        if (!m_simulation->init(*m_world)) {
            FARM_LOG_ERROR("Failed to initialize simulation");
            initializationFailed = true;
        }
    }
    
    // Create physics (only if previous steps succeeded)
    if (!initializationFailed) {
        m_physics = std::make_unique<PhysicsWorld>();
        if (!m_physics->init()) {
            FARM_LOG_ERROR("Failed to initialize physics");
            initializationFailed = true;
        }
    }
    
    // Create audio (optional - doesn't block initialization)
    if (!initializationFailed) {
        m_audio = std::make_unique<AudioSystem>();
        if (!m_audio->init()) {
            FARM_LOG_WARN("Audio system initialization failed (optional)");
            m_audio.reset();  // Clean up failed optional component
        }
    }
    
#ifdef FARMENGINE_WITH_EDITOR
    // Create editor (optional - doesn't block initialization)
    if (!initializationFailed) {
        m_editor = std::make_unique<Editor>(*this);
        if (!m_editor->init()) {
            FARM_LOG_WARN("Editor initialization failed (optional)");
            m_editor.reset();  // Clean up failed optional component
        }
    }
#endif
    
    // Handle initialization failure - cleanup in reverse order
    if (initializationFailed) {
        FARM_LOG_ERROR("Engine initialization failed - cleaning up");
        
#ifdef FARMENGINE_WITH_EDITOR
        if (m_editor) {
            m_editor->shutdown();
            m_editor.reset();
        }
#endif
        
        if (m_audio) {
            m_audio->shutdown();
            m_audio.reset();
        }
        
        if (m_physics) {
            m_physics->shutdown();
            m_physics.reset();
        }
        
        if (m_simulation) {
            m_simulation->shutdown();
            m_simulation.reset();
        }
        
        if (m_world) {
            m_world->shutdown();
            m_world.reset();
        }
        
        if (m_renderer) {
            m_renderer->shutdown();
            m_renderer.reset();
        }
        
        if (m_window) {
            m_window->shutdown();
            m_window.reset();
        }
        
        if (m_jobSystem) {
            m_jobSystem->shutdown();
            m_jobSystem.reset();
        }
        
        // Shutdown any plugins that were registered before failure
        for (auto it = m_plugins.rbegin(); it != m_plugins.rend(); ++it) {
            if (*it) {
                (*it)->shutdown();
            }
        }
        m_plugins.clear();
        
        return false;
    }
    
    m_initialized = true;
    m_running = true;
    
    FARM_LOG_INFO("Engine initialization complete");
    FARM_LOG_INFO("=========================================");
    FARM_LOG_INFO("FarmEngine v0.1.0 Ready!");
    FARM_LOG_INFO("=========================================");
    
    return true;
}

void Engine::run() {
    if (!m_initialized) {
        FARM_LOG_ERROR("Cannot run - engine not initialized");
        return;
    }
    
    FARM_LOG_INFO("Starting main loop...");
    
    auto lastTime = std::chrono::high_resolution_clock::now();
    
    while (m_running) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        m_deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;
        
        m_time += m_deltaTime;
        
        // Process input events
        processInput();
        
        // Update with variable timestep
        update();
        
        // Fixed timestep for physics/simulation
        m_fixedTimeAccumulator += m_deltaTime;
        while (m_fixedTimeAccumulator >= m_fixedTimestep) {
            fixedUpdate();
            m_fixedTimeAccumulator -= m_fixedTimestep;
        }
        
        // Render frame
        render();
        
        // Execute pending updates
        for (auto& func : m_pendingUpdates) {
            func();
        }
        m_pendingUpdates.clear();
    }
    
    FARM_LOG_INFO("Main loop ended");
}

void Engine::stop() {
    m_running = false;
}

void Engine::shutdown() {
    // Don't log if already shutting down or not initialized
    if (m_initialized) {
        FARM_LOG_INFO("Shutting down engine...");
    }
    
    m_running = false;
    
#ifdef FARMENGINE_WITH_EDITOR
    if (m_editor) {
        m_editor->shutdown();
        m_editor.reset();
    }
#endif
    
    if (m_audio) {
        m_audio->shutdown();
        m_audio.reset();
    }
    
    if (m_physics) {
        m_physics->shutdown();
        m_physics.reset();
    }
    
    if (m_simulation) {
        m_simulation->shutdown();
        m_simulation.reset();
    }
    
    if (m_world) {
        m_world->shutdown();
        m_world.reset();
    }
    
    if (m_renderer) {
        m_renderer->shutdown();
        m_renderer.reset();
    }
    
    if (m_window) {
        m_window->shutdown();
        m_window.reset();
    }
    
    if (m_jobSystem) {
        m_jobSystem->shutdown();
        m_jobSystem.reset();
    }
    
    // Shutdown all registered plugins in reverse order
    for (auto it = m_plugins.rbegin(); it != m_plugins.rend(); ++it) {
        if (*it) {
            (*it)->shutdown();
        }
    }
    m_plugins.clear();
    
    m_initialized = false;
    
    // Log after all subsystems are shut down but before logger shutdown
    // Logger shutdown must be the very last thing
    if (Logger::isInitialized()) {
        FARM_LOG_INFO("Engine shutdown complete");
        Logger::shutdown();
    }
}

void Engine::processInput() {
    if (m_window) {
        m_window->pollEvents();
    }
    
#ifdef FARMENGINE_WITH_EDITOR
    if (m_editor && m_editor->isActive()) {
        m_editor->processInput();
    }
#endif
}

void Engine::update() {
    // Update simulation systems
    if (m_simulation) {
        m_simulation->update(m_deltaTime);
    }
    
    // Update world systems
    if (m_world) {
        m_world->update(m_deltaTime);
    }
    
#ifdef FARMENGINE_WITH_EDITOR
    if (m_editor && m_editor->isActive()) {
        m_editor->update(m_deltaTime);
    }
#endif
}

void Engine::fixedUpdate() {
    const float fixedDt = static_cast<float>(m_fixedTimestep);
    
    // Physics update
    if (m_physics) {
        m_physics->update(fixedDt);
    }
    
    // Simulation update (crop growth, animal AI, etc.)
    if (m_simulation) {
        m_simulation->fixedUpdate(fixedDt);
    }
}

void Engine::render() {
    if (!m_renderer) return;
    
    // Start frame
    if (!m_renderer->beginFrame()) {
        return;
    }
    
    // Render world
    if (m_world) {
        m_world->render(*m_renderer);
    }
    
#ifdef FARMENGINE_WITH_EDITOR
    // Render editor UI
    if (m_editor && m_editor->isActive()) {
        m_editor->render(*m_renderer);
    }
#endif
    
    // End frame and present
    m_renderer->endFrame();
}

void Engine::registerPlugin(const std::string& name, std::unique_ptr<IPlugin> plugin) {
    if (!plugin) {
        FARM_LOG_ERROR("Attempted to register null plugin: {}", name);
        return;
    }
    
    // Initialize the plugin
    if (!plugin->init()) {
        FARM_LOG_ERROR("Plugin initialization failed: {}", name);
        return;
    }
    
    // Store plugin for later use
    m_plugins.emplace_back(std::move(plugin));
    FARM_LOG_INFO("Registered plugin: {}", name);
    FARM_LOG_DEBUG("Plugin '{}' initialized and registered successfully", name);
}

void Engine::queueUpdate(std::function<void()> func) {
    m_pendingUpdates.push_back(std::move(func));
}

} // namespace farm
