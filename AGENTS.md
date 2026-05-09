# AGENT.md

本文件面向在本仓库中工作的 AI 编程助手。进入任务后请先阅读本文件，再根据用户的具体需求查看源码。

## Project Context

这是 `Bilibili-Reranger` 后端插件开发模板，用于编译供主程序动态加载的 Linux `.so` 插件。插件通常是用户私人定制的筛选、排序、过滤或推荐逻辑，不一定会推送到公开仓库。

上游项目：<https://github.com/passplease/Bilibili-Reranger>

当前仓库不是完整后端，只是插件模板。主程序、SDK、接口定义和运行环境由开发容器或用户本机环境提供。

## Repository Layout

- `src/main.cpp`: 插件实现入口。当前模板只导出 `load()`。
- `CMakeLists.txt`: CMake 构建配置，生成共享库并链接 `BiliBiliRerangerAPI::API`。
- `.devcontainer/`: 推荐开发容器配置，包含后端、数据库、浏览器、前端网站等服务编排。
- `running_path/`: 本地运行时挂载目录，构建后的插件会复制到 `running_path/plugin`。该目录内容通常是本地运行产物，不应提交。
- `cmake-build-*`, `build/`, `dist/`, `.idea/`: IDE 或构建产物，不应作为功能代码处理。

## Core Technical Facts

- Language: C++23.
- Build system: CMake, minimum version `3.27`.
- Artifact type: shared library, target name `VIDEO_PLUGIN`.
- Runtime platform expected by template: Linux x64, vcpkg triplet defaults to `x64-linux`.
- SDK package: `BiliBiliRerangerAPI`, imported as `BiliBiliRerangerAPI::API`.
- Plugin SDK root defaults to `/opt/bilibili-reranger-sdk`.
- Backend root defaults to `/opt/bilibili-reranger-backend`.
- Debug builds define `DEVELOP=true`.

The minimal exported ABI currently is:

```cpp
#include <interface.h>

extern "C" PluginStatus load() {
    return PluginStatus::SUCCESS;
}
```

Keep exported plugin entry points C-compatible when the host expects dynamic symbol lookup. Do not rename or remove `extern "C" PluginStatus load()` unless the user explicitly asks and the SDK contract confirms it.

## Build And Run

Prefer using the dev container when available, because it provides the SDK, backend runtime, vcpkg, browser service, and PostgreSQL service expected by this template.

Typical CMake commands inside the proper Linux/devcontainer environment:

```bash
cmake -S . -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-debug --target VIDEO_PLUGIN
cmake --build cmake-build-debug --target VIDEO_PLUGIN_COPY_DLL
```

Release build:

```bash
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release --target VIDEO_PLUGIN
cmake --build cmake-build-release --target VIDEO_PLUGIN_COPY_DLL
```

`VIDEO_PLUGIN_COPY_DLL` copies the generated `.so` into:

```text
running_path/plugin
```

In the devcontainer compose setup, that directory is mounted to both:

```text
/opt/bilibili-reranger-backend/debug/plugins
/opt/bilibili-reranger-backend/release/plugins
```

If configuring outside the devcontainer, the user may need to pass:

```bash
-DBILIBILI_RERANGER_SDK_ROOT=/path/to/bilibili-reranger-sdk
-DBILIBILI_RERANGER_BACKEND_ROOT=/path/to/bilibili-reranger-backend
-DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
-DVCPKG_TARGET_TRIPLET=x64-linux
```

## Development Guidelines

- Treat this as a private plugin project. Avoid adding publishing, packaging, telemetry, or public release workflow unless asked.
- Keep changes scoped to plugin behavior and build support needed for that behavior.
- Prefer SDK types and APIs from `<interface.h>` and `BiliBiliRerangerAPI` over locally invented duplicate structs.
- Before changing interface-related code, inspect the installed SDK headers if available in the environment.
- Do not hard-code user secrets, Bilibili cookies, access tokens, local absolute paths, or account-specific preferences into tracked source files.
- Put user-specific runtime config under ignored runtime paths such as `running_path/config` when possible.
- Avoid committing generated `.so` files, CMake build directories, database data, IDE metadata, browser logs, AI session folders, or `.env` files.
- If adding third-party dependencies, prefer vcpkg/CMake integration and document why the dependency is needed.
- Keep the plugin robust when input data is missing or malformed. Host-side data may come from network responses, database rows, or user configuration.

## AI Workflow For This Template

When starting a task:

1. Read `AGENT.md`, then inspect `src/main.cpp` and `CMakeLists.txt`.
2. If the task depends on plugin API details, locate and read the SDK header that provides `interface.h`.
3. Identify the exact exported functions or registration hooks expected by the host before adding new plugin entry points.
4. Keep private customization logic easy to edit: use clear helper functions and small configuration structures.
5. After code changes, run the narrowest available build command. If the SDK is unavailable in the current environment, state that build verification could not be completed.

When editing:

- Use C++23 features only where they improve clarity and are supported by the configured toolchain.
- Prefer deterministic, testable ranking/filtering logic over hidden global state.
- Avoid background threads, network calls, filesystem writes, and database access inside plugin load unless the SDK explicitly expects it.
- Return failure status from plugin entry points when initialization cannot safely continue, if the SDK exposes such a status.

## Verification Checklist

Before considering a change done, check:

- The project still configures with CMake in the intended environment.
- `VIDEO_PLUGIN` builds as a shared library.
- `VIDEO_PLUGIN_COPY_DLL` places the `.so` under `running_path/plugin`.
- Exported symbols required by the host are still present.
- No private credentials or generated runtime artifacts were added to git.

Useful local checks:

```bash
cmake --build cmake-build-debug --target VIDEO_PLUGIN
cmake --build cmake-build-debug --target VIDEO_PLUGIN_COPY_DLL
git status --short
```

If symbol verification is needed:

```bash
nm -D cmake-build-debug/libVIDEO_PLUGIN.so | grep load
```

Adjust the output library path if the active generator places artifacts elsewhere.

## Notes For Future AI Agents

The current `README.md` may appear garbled if read with the wrong encoding. Do not rewrite it unless the user asks. This `AGENT.md` is the primary quick-start context for AI-assisted plugin development in this template.
