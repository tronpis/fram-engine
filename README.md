# Farm Engine

A modern Vulkan-based game engine specialized for farming simulation games.

## 📁 Project Structure

- **FarmEngine/** - Main engine source code and documentation
  - See [FarmEngine/README.md](FarmEngine/README.md) for detailed documentation

## 🚀 Quick Start

```bash
cd FarmEngine
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

## 🎯 Features

- Modern Vulkan renderer with Render Graph architecture
- Hybrid 2D/3D rendering pipeline
- ECS (Entity Component System) architecture
- Massive crop simulation support (100k+ crops)
- Modular plugin system
- Integrated editor with Dear ImGui
- World streaming & chunk system

## 📋 Requirements

- **Compiler:** GCC 11+ / Clang 13+ / MSVC 2022
- **Vulkan SDK:** 1.3+
- **CMake:** 3.20+

## 📖 Documentation

For full documentation, architecture details, and API reference, see:
- [FarmEngine README](FarmEngine/README.md)
- [Architecture Documentation](FarmEngine/ARQUITECTURA.md)

## 📝 License

MIT License
