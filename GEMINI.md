# Bilibili Reranger Backend

## Project Overview

This project is the backend for **Bilibili Reranger**, a C++-based service designed to interact with the Bilibili platform. It functions as a framework for managing and filtering Bilibili content, with the core logic implemented through a plugin-based architecture. The backend communicates with a separate `BrowserManager` component to handle the actual web crawling and data extraction.

The system provides a RESTful API for client applications, handling user authentication (including Bilibili's captcha), data encryption, and task management. It uses a PostgreSQL database for data and cookie persistence.

**Key Technologies:**
- **Backend:** C++
- **API/Web:** Boost (Asio, Beast), CPR, CURL
- **Data:** nlohmann/json, libpqxx (PostgreSQL)
- **Security:** OpenSSL, libsodium
- **Build:** CMake

## Building and Running

The project is intended to be run using Docker.

### Docker
The recommended method for deployment is using the pre-built Docker image.

**Run Command:**
```bash
docker run -p 23223:23223 \
  -e COOKIE=<your_bilibili_cookie> \
  -e USERAGENT=<browser_user_agent> \
  -v <path_to_config>:/bilibili-backend/config \
  -v <path_to_plugin>:/bilibili-backend/plugins \
  docker.io/noname602/bilibili_reranger:latest
```
**Required Environment Variables:**
- `COOKIE`: Your Bilibili account cookie.
- `USERAGENT`: Your browser's User-Agent string.

### Building from Source (Inferred)
While not the primary method, the project uses CMake for its build system. A typical build process would involve:

```bash
# Configure the project
cmake -B cmake-build-debug -S .

# Build the project
cmake --build cmake-build-debug
```

## Development Conventions

### Plugin Architecture
The core business logic is implemented via plugins. The main application acts as a framework, handling API requests, scheduling, and other boilerplate tasks, while plugins determine how to process and filter content. An [example plugin](plugins/ExamplePlugin/main.cpp) is provided to guide development.

### API
The backend exposes a set of RESTful endpoints for clients. Key interactions include:
- `/key`: To exchange encryption keys.
- `/login`: To authenticate with the target platform (e.g., Bilibili).
- `/init`: To initialize the crawler after login.
- `/set`: To configure the working platform and other parameters.

All communication with the backend (except for the initial key exchange) is expected to be encrypted.

### Code Structure
- `api/`: Contains the source code for the external API, including endpoint handlers and data interfaces.
- `src/`: Contains the main application logic, including the port listener, plugin handler, and platform-specific implementations.
- `plugins/`: Contains the plugins that extend the core functionality.
- `browser/`: Contains a separate Node.js project for the `BrowserManager`.

## Prompts
中文回答我的问题
所有问题都可以阅读我的文件，但是不着急修改文件，首先告诉我原因，我同意后再统一修改文件
