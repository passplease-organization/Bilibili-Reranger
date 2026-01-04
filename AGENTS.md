# Repository Guidelines

## Project Structure & Module Organization
- `src/` contains the C++ backend (crawler, port listener, plugin handler), with platform-specific code under `src/platforms/` and helper utilities in `src/subFeatures/`.
- `api/` hosts the shared plugin API headers used by the main app and plugins.
- `plugins/ExamplePlugin/` is the reference plugin; use it as a template for new plugins.
- `src/test/` includes the test harness code, while `test/` stores fixture data and `ssl.pem` copied into test build outputs.
- `cmake-build-*` directories are local build outputs and should not be committed.

## Build, Test, and Development Commands
- Configure debug build: `cmake -S . -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug`.
- Build debug binary: `cmake --build cmake-build-debug` (produces `BiliBili_Reranger` under `cmake-build-debug/Debug/`).
- Configure test build: `cmake -S . -B cmake-build-test -DCMAKE_BUILD_TYPE=Test` (enables test code and links `cpr`).
- Build tests: `cmake --build cmake-build-test` (copies `test/` into `cmake-build-test/Test/testing/`).
- Docker run (recommended):
  `docker run -p 23223:23223 -e COOKIE=... -e USERAGENT=... -v <path_to_config>:/bilibili-backend/config -v <path_to_plugin>:/bilibili-backend/plugins docker.io/noname602/bilibili_reranger:latest`.

## Coding Style & Naming Conventions
- C++23 is required (`CMAKE_CXX_STANDARD 23`). Use `.h` for headers and `.cpp` for sources.
- File naming matches existing patterns: lowerCamel for sources (e.g., `crawler.cpp`) and UpperCamel for headers (e.g., `Crawler.h`).
- Follow the surrounding indentation and brace placement; avoid reformatting unrelated code.

## Testing Guidelines
- Test logic lives in `src/test/testCode.cpp` and compiles only with `CMAKE_BUILD_TYPE=Test`.
- Use fixtures from `test/` and keep new fixture files small and clearly named (e.g., `SearchData.json`).

## Commit & Pull Request Guidelines
- Commit subjects are short, imperative or phrase-style, no prefixes (e.g., “login features”, “correct workflow”).
- PRs should summarize changes, note any config/env requirements (`COOKIE`, `USERAGENT`, plugin paths), and list build/test commands run.

## Security & Configuration Tips
- Do not commit real `COOKIE` or `USERAGENT` values; they are required at runtime.
- Plugins are loaded from `plugins/` (or mounted plugin directory in Docker); keep interfaces aligned with `api/interface.h`.

## Prompts
中文回答我的问题