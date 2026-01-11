# BilibiliReranger-Backend

## Project Overview

BilibiliReranger-Backend is a C++ project designed as a Bilibili video crawler application. It provides a framework for crawling Bilibili videos with support for plugins to extend functionality. The project uses CMake for building and relies on external libraries like CURL, Boost, nlohmann-json, and cpr.

The project appears to be in a non-functional state according to the README, but the codebase shows a structured architecture with a main executable, an API library, and a plugin system. It includes features like port listening (optionally), plugin handling, and integration with Bilibili APIs.

## Project Structure

- `api/` - Contains the shared library providing the API and interfaces for plugins, including Bilibili API integration and plugin interfaces
- `plugins/` - Contains plugin implementations, with `ExamplePlugin` as a reference implementation
- `src/` - Main application source code including crawler, port listener, and plugin handler
- `test/` - Contains test data files (JSON) and SSL certificates
- `CMakeLists.txt` - Main CMake build configuration
- `Dockerfile` - Docker configuration for the build environment

## Building and Running

### Prerequisites

- CMake (3.27 or higher)
- A C++23 compatible compiler (GCC, Clang, or MSVC)
- vcpkg (for dependency management)
- Dependencies managed via vcpkg: `cpr`, `curl`, `nlohmann-json`, `boost-url`, and potentially `cryptopp`

### Build Process

1. Install dependencies using vcpkg as shown in the Dockerfile:
   ```bash
   vcpkg install cpr curl nlohmann-json boost-url
   ```
2. Configure the project:
   ```bash
   mkdir cmake-build-debug
   cd cmake-build-debug
   cmake -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake ..
   ```
3. Build the project:
   ```bash
   cmake --build .
   ```
4. The executable will be created in the build directory under `Debug` or `Release` subdirectories.

### Running

The main executable is `BiliBili_Reranger`. The project supports both standalone execution and port listening modes (controlled by `NEED_PORT` compile flag). When `NEED_PORT` is defined, the application listens on a port for commands; otherwise, it takes command-line arguments.

## Architecture

The project consists of three main components:

1. **Main Executable (BiliBili_Reranger)**: The core application that coordinates crawling tasks and manages plugins.
2. **API Library (BiliBili_Reranger_API)**: A shared library providing interfaces for Bilibili API access, plugin development, and utility functions.
3. **Plugins**: Dynamic libraries (like ExamplePlugin) that extend functionality by implementing specific crawling tasks.

The API library provides interfaces for Bilibili API calls, plugin interfaces, and configuration management. The plugin system allows for modular extension of crawling capabilities.

## Development Conventions

- Uses C++23 standard
- CMake-based build system
- Plugin architecture for extensibility
- Cross-platform considerations (with Windows-specific warnings in code)
- Uses nlohmann JSON for JSON handling
- Uses cpr for HTTP requests (as opposed to raw CURL)
- Includes development/debugging flags and configurations

The project includes development-specific flags (via `DEVELOP` definition) and test-specific configurations (via `TEST_DLL` flag).
## Prompt
请使用中文与我对话
只在我明确告知你的情况下为我的代码进行修改或尝试解决问题，默认我是在和你讨论项目架构和功能问题，不能主动修改我的代码