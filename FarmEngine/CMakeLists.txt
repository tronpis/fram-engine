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

# Fetch fmt library for logging
include(FetchContent)
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 10.2.1
)
FetchContent_MakeAvailable(fmt)

# ============================================================================
# Core Library
# ============================================================================

add_library(FarmEngineCore STATIC
    # Logger
    core/logger/Logger.cpp
    core/logger/Logger.h
    # JobSystem
    core/jobsystem/JobSystem.cpp
    core/jobsystem/JobSystem.h
    # Window
    platform/window/Window.cpp
    platform/window/Window.h
    # Engine
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
    fmt::fmt
)

# Platform-specific Vulkan definitions
if(UNIX AND NOT APPLE)
    target_compile_definitions(FarmEngineCore PRIVATE
        VK_USE_PLATFORM_XLIB_KHR
        VK_USE_PLATFORM_WAYLAND_KHR
    )
endif()

# ============================================================================
# Renderer Library
# ============================================================================

add_library(FarmEngineRenderer STATIC
    renderer/Renderer.cpp
    renderer/Renderer.h
    # Render Graph System
    src/rendering/graph/RenderGraph.h
    src/rendering/graph/RenderGraph.cpp
    src/rendering/graph/RenderPipeline.h
    src/rendering/graph/RenderPipeline.cpp
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
        tools/editor/Editor.h
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

# Create a minimal main.cpp for now
file(WRITE ${CMAKE_BINARY_DIR}/main.cpp "
#include \"core/Engine.h\"

int main() {
    farm::Engine engine;
    if (engine.init()) {
        // Engine initialized successfully
        return 0;
    }
    return 1;
}
")

add_executable(FarmEngine
    ${CMAKE_BINARY_DIR}/main.cpp
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
    # Explicitly list shader files to ensure CMake detects changes
    set(SHADER_SOURCES
        ${CMAKE_SOURCE_DIR}/assets/shaders/basic.vert
        ${CMAKE_SOURCE_DIR}/assets/shaders/basic.frag
    )
    
    # Filter out non-existent files
    set(EXISTING_SHADERS "")
    foreach(SHADER ${SHADER_SOURCES})
        if(EXISTS ${SHADER})
            list(APPEND EXISTING_SHADERS ${SHADER})
        endif()
    endforeach()
    
    foreach(SHADER ${EXISTING_SHADERS})
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
    
    if(SHADER_OUTPUTS)
        add_custom_target(Shaders DEPENDS ${SHADER_OUTPUTS})
        add_dependencies(FarmEngine Shaders)
    endif()
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
