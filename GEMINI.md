# BilibiliReranger-Backend

## Project Overview

This project is a C++ backend application for crawling data from Bilibili. It's built with C++23 and uses CMake for its build system. The application has a modular architecture, with a core crawling engine and a plugin system that allows for extending its functionality.

The project is structured into three main components:

*   **`src`**: The main application logic, including the crawler engine and plugin handler.
*   **`api`**: A shared library that defines the public API for the application and its plugins. It uses `nlohmann/json` for JSON parsing and `cpr` for HTTP requests.
*   **`plugins`**: Example plugins that demonstrate how to extend the application's functionality.

The application uses `vcpkg` for dependency management. The main dependencies are:

*   **`CURL`**: For making HTTP requests.
*   **`Boost`**: For URL handling and other utilities.
*   **`nlohmann/json`**: For working with JSON data.
*   **`cpr`**: A C++ wrapper for `curl`.

## Building and Running

### Building

The project uses CMake for building. To build the project, you can use the following commands:

```bash
# For debug build
cmake -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug .
cmake --build cmake-build-debug

# For release build
cmake -B cmake-build-release -DCMAKE_BUILD_TYPE=Release .
cmake --build cmake-build-release
```

### Running

After building, the main executable will be located in the `cmake-build-debug` or `cmake-build-release` directory. You can run it from the project root directory:

```bash
# For debug build
./cmake-build-debug/BiliBili_Reranger

# For release build
./cmake-build-release/BiliBili_Reranger
```

### Testing

There is no dedicated test suite found.

## Development Conventions

*   The project uses C++23.
*   The code is formatted using a consistent style, but no specific style guide is mentioned.
*   The project has a plugin-based architecture. New functionality should be added as plugins.
*   The project uses `vcpkg` for dependency management. New dependencies should be added to the `vcpkg.json` file.

## Prompt
Please communicate with me using Chinese
Only modify my code or attempt to resolve issues when I explicitly instruct you to do so. By default, I am discussing project architecture and functional requirements with you, not asking you to modify my code.