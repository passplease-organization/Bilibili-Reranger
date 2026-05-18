# Repository Guidelines

## Project Structure & Module Organization
- `src/` contains the C++ backend entrypoint, crawler, port listener, and plugin loader.
- `src/platforms/` holds platform-specific handlers such as Bilibili integration.
- `src/subFeatures/` stores smaller shared helpers, for example request utilities.
- `api/` provides the shared plugin API and exported interfaces used by the main app and plugins.
- `plugins/ExamplePlugin/` is the reference plugin implementation and the best template for new plugins.
- `src/test/` contains test-only code, while `test/` stores fixtures copied into Test builds.
- `cmake-build-*` directories are local outputs and should stay untracked.

项目全部分支请看远端仓库，此部分只是后端(backend)部分，需要和`frontend`分支一起工作，`master`分支是部署使用的，不涉及代码。项目目的是为我刷视频时从各大视频平台筛选视频，并通过这部分展现在我面前。项目面向个人使用，这个部分使用docker-compose与browser部分(在`browser/`目录下，是TS语言的，负责操控浏览器完成具体爬取任务)一同部署，网站为静态网站，docker-compose中会使用nginx封装向外提供服务（只暴露nginx的端口），负责数据处理和与browser部分的对接。

当前环境是在docker开发容器中，其他辅助服务在其他容器，你看不到，配置文件在`.devcontainer/`下，同样还有browser部分的配置文件在`.devcontainer/browser/`下。

## Build, Test, and Development Commands
- Configure a debug build: `cmake -S . -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug`
- Build the debug binary: `cmake --build cmake-build-debug`
- Configure the test build: `cmake -S . -B cmake-build-test -DCMAKE_BUILD_TYPE=Test`
- Build and run test artifacts: `cmake --build cmake-build-test`
  The Test configuration enables `src/test/testCode.cpp`, links `cpr`, and copies `test/` data into the build output.
- Recommended local deployment uses Docker:
  `docker run -p 23223:23223 -e BROWSE_URL=... -v <config_dir>:/bilibili-backend/config -v <plugin_dir>:/bilibili-backend/plugins docker.io/noname602/bilibili_reranger:latest`

## Coding Style & Naming Conventions
- Use C++23 and keep source/header pairs in `.cpp` and `.h`.
- Follow existing naming patterns: lowerCamel for source files such as `crawler.cpp`, UpperCamel for headers such as `Crawler.h`.
- Match the surrounding indentation and brace style. Do not reformat unrelated code in the same change.
- Keep platform logic in `src/platforms/` and shared plugin-facing declarations in `api/`.

## Testing Guidelines
- Add backend tests in `src/test/testCode.cpp` or helper headers beside it.
- Keep fixture files in `test/` small and purpose-specific, for example `SearchData.json`.
- Validate changes with the `Test` CMake build before opening a PR.
- GitHub Actions runs backend Test builds and integration checks, so local failures usually reproduce in CI.

## Security & Configuration Tips
- Never commit real `COOKIE`, `USERAGENT`, database, or admin-key values.
- Keep plugin interfaces aligned with `api/interface.h` and verify mounted plugin directories when testing Docker flows.

## Prompts
中文回答我的问题
所有问题都可以阅读我的文件，但是不着急修改文件，首先告诉我原因，我同意后再统一修改文件
