cmake_minimum_required(VERSION 3.20)
project(FarmEngine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build options
option(FARMENGINE_BUILD_EDITOR "Build integrated editor" ON)
option(FARMENGINE_BUILD_TOOLS "Build development tools" ON)
option(FARMENGINE_VULKAN_VALIDATION "Enable Vulkan validation layers" ON)

# ============================================================================
# Find Dependencies
# ============================================================================

find_package(Vulkan REQUIRED)
find_package(glfw3 QUIET)
find_package(glm QUIET)

# If packages not found, use bundled dependencies (FetchContent)
if(NOT glfw3_FOUND OR NOT glm_FOUND)
    include(FetchContent)
    
    if(NOT glfw3_FOUND)
        FetchContent_Declare(
            glfw
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG 3.3.8
        )
        FetchContent_MakeAvailable(glfw)
    endif()
    
    if(NOT glm_FOUND)
        FetchContent_Declare(
            glm
            GIT_REPOSITORY https://github.com/g-truc/glm.git
            GIT_TAG 0.9.9.8
        )
        FetchContent_MakeAvailable(glm)
    endif()
endif()

# ============================================================================
# Core Library
# ============================================================================

add_library(FarmEngineCore STATIC
    # Core systems
    core/logger/Logger.cpp
    core/logger/Logger.h
    core/memory/MemoryManager.cpp
    core/memory/MemoryManager.h
    core/jobsystem/JobSystem.cpp
    core/jobsystem/JobSystem.h
    core/ecs/ECS.cpp
    core/ecs/ECS.h
    core/ecs/Entity.h
    core/ecs/Component.h
    core/ecs/System.h
    core/Engine.cpp
    core/Engine.h
)

target_include_directories(FarmEngineCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(FarmEngineCore PUBLIC
    ${Vulkan_LIBRARIES}
    glfw
    glm::glm
)

target_compile_definitions(FarmEngineCore PRIVATE
    VK_USE_PLATFORM_XLIB_KHR
    VK_USE_PLATFORM_WAYLAND_KHR
)

# ============================================================================
# Renderer Library
# ============================================================================

add_library(FarmEngineRenderer STATIC
    # Vulkan context
    renderer/vulkan/context/VulkanContext.cpp
    renderer/vulkan/context/VulkanContext.h
    renderer/vulkan/context/Swapchain.cpp
    renderer/vulkan/context/Swapchain.h
    
    # Resource management
    renderer/vulkan/resource/ResourceSystem.cpp
    renderer/vulkan/resource/ResourceSystem.h
    renderer/vulkan/resource/GPUBuffer.cpp
    renderer/vulkan/resource/GPUBuffer.h
    renderer/vulkan/resource/GPUTexture.cpp
    renderer/vulkan/resource/GPUTexture.h
    
    # Pipeline management
    renderer/vulkan/pipeline/PipelineManager.cpp
    renderer/vulkan/pipeline/PipelineManager.h
    renderer/vulkan/pipeline/PipelineCache.cpp
    renderer/vulkan/pipeline/PipelineCache.h
    
    # Render Graph
    renderer/vulkan/graph/RenderGraph.cpp
    renderer/vulkan/graph/RenderGraph.h
    renderer/vulkan/graph/RenderPassNode.h
    
    # Render passes
    renderer/passes/ShadowPass.cpp
    renderer/passes/ShadowPass.h
    renderer/passes/GBufferPass.cpp
    renderer/passes/GBufferPass.h
    renderer/passes/LightingPass.cpp
    renderer/passes/LightingPass.h
    
    # 2D Renderer
    renderer/2d/SpriteRenderer.cpp
    renderer/2d/SpriteRenderer.h
    renderer/2d/TilemapRenderer.cpp
    renderer/2d/TilemapRenderer.h
    renderer/2d/SpriteBatch.cpp
    renderer/2d/SpriteBatch.h
    
    # 3D Renderer
    renderer/3d/MeshRenderer.cpp
    renderer/3d/MeshRenderer.h
    renderer/3d/TerrainRenderer.cpp
    renderer/3d/TerrainRenderer.h
    renderer/3d/InstancedRenderer.cpp
    renderer/3d/InstancedRenderer.h
    
    renderer/Renderer.cpp
    renderer/Renderer.h
)

target_include_directories(FarmEngineRenderer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(FarmEngineRenderer PUBLIC
    FarmEngineCore
    ${Vulkan_LIBRARIES}
)

# ============================================================================
# World System
# ============================================================================

add_library(FarmEngineWorld STATIC
    world/terrain/Terrain.cpp
    world/terrain/Terrain.h
    world/terrain/Heightmap.cpp
    world/terrain/Heightmap.h
    
    world/chunks/ChunkSystem.cpp
    world/chunks/ChunkSystem.h
    world/chunks/CropChunk.h
    world/chunks/WorldStreamer.cpp
    world/chunks/WorldStreamer.h
    
    world/tiles/Tilemap.cpp
    world/tiles/Tilemap.h
    world/tiles/Tile.h
    
    world/World.cpp
    world/World.h
)

target_include_directories(FarmEngineWorld PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(FarmEngineWorld PUBLIC
    FarmEngineCore
    FarmEngineRenderer
)

# ============================================================================
# Simulation System
# ============================================================================

add_library(FarmEngineSimulation STATIC
    simulation/crops/CropSystem.cpp
    simulation/crops/CropSystem.h
    simulation/crops/Crop.h
    simulation/crops/CropData.h
    
    simulation/animals/AnimalSystem.cpp
    simulation/animals/AnimalSystem.h
    simulation/animals/Animal.h
    simulation/animals/AnimalAI.cpp
    simulation/animals/AnimalAI.h
    
    simulation/weather/WeatherSystem.cpp
    simulation/weather/WeatherSystem.h
    simulation/weather/Season.h
    simulation/weather/Weather.h
    
    simulation/soil/SoilSystem.cpp
    simulation/soil/SoilSystem.h
    simulation/soil/SoilTile.h
    
    simulation/Simulation.cpp
    simulation/Simulation.h
)

target_include_directories(FarmEngineSimulation PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(FarmEngineSimulation PUBLIC
    FarmEngineCore
    FarmEngineWorld
)

# ============================================================================
# Physics (Optional - stub for now)
# ============================================================================

add_library(FarmEnginePhysics STATIC
    physics/PhysicsWorld.cpp
    physics/PhysicsWorld.h
    physics/Collision.cpp
    physics/Collision.h
)

target_include_directories(FarmEnginePhysics PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(FarmEnginePhysics PUBLIC
    FarmEngineCore
)

# ============================================================================
# Audio (Optional - stub for now)
# ============================================================================

add_library(FarmEngineAudio STATIC
    audio/AudioSystem.cpp
    audio/AudioSystem.h
    audio/AudioSource.cpp
    audio/AudioSource.h
)

target_include_directories(FarmEngineAudio PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(FarmEngineAudio PUBLIC
    FarmEngineCore
)

# ============================================================================
# Editor & Tools
# ============================================================================

if(FARMENGINE_BUILD_EDITOR OR FARMENGINE_BUILD_TOOLS)
    # Dear ImGui integration
    add_library(FarmEngineEditorLib STATIC
        tools/editor/Editor.cpp
        tools/editor/Editor.h
        tools/editor/EditorUI.cpp
        tools/editor/EditorUI.h
        tools/inspector/Inspector.cpp
        tools/inspector/Inspector.h
    )
    
    target_include_directories(FarmEngineEditorLib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    
    target_link_libraries(FarmEngineEditorLib PUBLIC
        FarmEngineCore
        FarmEngineRenderer
        FarmEngineWorld
        FarmEngineSimulation
    )
endif()

# ============================================================================
# Main Engine Executable
# ============================================================================

add_executable(FarmEngine
    main.cpp
)

target_link_libraries(FarmEngine PRIVATE
    FarmEngineCore
    FarmEngineRenderer
    FarmEngineWorld
    FarmEngineSimulation
    FarmEnginePhysics
    FarmEngineAudio
)

if(FARMENGINE_BUILD_EDITOR)
    target_link_libraries(FarmEngine PRIVATE FarmEngineEditorLib)
    target_compile_definitions(FarmEngine PRIVATE FARMENGINE_WITH_EDITOR)
endif()

# ============================================================================
# Plugin System Example
# ============================================================================

add_library(ExamplePlugin MODULE
    plugins/ExamplePlugin.cpp
    plugins/ExamplePlugin.h
    plugins/IPlugin.h
)

target_link_libraries(ExamplePlugin PRIVATE
    FarmEngineCore
)

set_target_properties(ExamplePlugin PROPERTIES
    PREFIX ""
    OUTPUT_NAME "example_plugin"
)

# ============================================================================
# Shader Compilation
# ============================================================================

# Create shader compilation directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)

# Find glslc (Vulkan shader compiler)
find_program(GLSLC glslc HINTS $ENV{VULKAN_SDK}/bin)

if(GLSLC)
    file(GLOB SHADER_SOURCES "${CMAKE_SOURCE_DIR}/assets/shaders/*.vert" 
                              "${CMAKE_SOURCE_DIR}/assets/shaders/*.frag"
                              "${CMAKE_SOURCE_DIR}/assets/shaders/*.comp")
    
    foreach(SHADER ${SHADER_SOURCES})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        set(SHADER_OUTPUT "${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME}.spv")
        
        add_custom_command(
            OUTPUT ${SHADER_OUTPUT}
            COMMAND ${GLSLC} ${SHADER} -o ${SHADER_OUTPUT}
            DEPENDS ${SHADER}
            COMMENT "Compiling shader: ${SHADER_NAME}"
        )
        
        list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT})
    endforeach()
    
    add_custom_target(Shaders DEPENDS ${SHADER_OUTPUTS})
    add_dependencies(FarmEngine Shaders)
else()
    message(WARNING "glslc not found. Shaders will not be compiled automatically.")
endif()

# ============================================================================
# Installation
# ============================================================================

install(TARGETS FarmEngine
    RUNTIME DESTINATION bin
)

install(DIRECTORY assets/
    DESTINATION share/farmengine/assets
)

# ============================================================================
# Configuration Summary
# ============================================================================

message(STATUS "")
message(STATUS "FarmEngine Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Editor: ${FARMENGINE_BUILD_EDITOR}")
message(STATUS "  Build Tools: ${FARMENGINE_BUILD_TOOLS}")
message(STATUS "  Vulkan Validation: ${FARMENGINE_VULKAN_VALIDATION}")
message(STATUS "  Vulkan Include: ${Vulkan_INCLUDE_DIRS}")
message(STATUS "  Vulkan Libraries: ${Vulkan_LIBRARIES}")
message(STATUS "")
