---
name: bilibili-reranger-sdk
description: Inspect, extend, and debug Bilibili-Reranger Linux plugins against the local SDK. Use when work touches plugin ABI exports such as load/registerGroups/roughJudge/judge, crawl task registration, webAPI::Video access, SDK TOML config helpers, config paths, shared-library deployment, or SDK/header lookup under /opt/bilibili-reranger-sdk.
---

# Bilibili Reranger SDK

## Workflow

1. Read `AGENTS.md`, `CLAUDE.md`, and `CMakeLists.txt` before changing plugin behavior.
2. Open the narrowest relevant SDK header:
   - `interface.h` for exported hook declarations.
   - `pluginInterface.h` for statuses, task/group types, and `WorkingMode`.
   - `utils/BilibiliInterface.h` for `webAPI::Video` and `nowVideo()`.
   - `utils/configUtil.h` for config directories, config paths, and TOML utilities.
3. Read [references/sdk-notes.md](references/sdk-notes.md) when implementing or reviewing plugin behavior, config handling, or deployment checks.
4. Prefer SDK APIs over local copies:
   - Resolve runtime config paths with `toConfigPath()`.
   - Call `makeConfigDir()` before config work.
   - Use `Config`, `cppUtil::setConfig`, `cppUtil::getConfig`, and `cppUtil::saveConfig` for TOML-backed plugin config.
   - Keep exported hooks `extern "C"`.
5. For homepage filtering, preserve the `HOME_PAGE_FILTER` task flow unless intentionally changing feature type.
6. Verify the intended build, deploy/copy target, and exported symbols after edits.

## Quick Reference

- SDK root: `/opt/bilibili-reranger-sdk`
- Backend root: `/opt/bilibili-reranger-backend`
- Release headers: `/opt/bilibili-reranger-sdk/release/include/BiliBili_Reranger`
- Debug headers: `/opt/bilibili-reranger-sdk/debug/include/BiliBili_Reranger`
- Required plugin hook: `extern "C" PluginStatus load()`
- Common optional hooks: `registerGroups()`, `roughJudge()`, `judge()`

Homepage registration usually follows:

```cpp
extern "C" void registerGroups() {
    auto* group = new crawlTask::Group("KnowledgeHomepage", "Bilibili", 50);
    auto* task = new crawlTask::Task(
        "",
        50,
        crawlTask::WorkingMode::HOME_PAGE_FILTER,
        -1
    );
    group->registerTask(task);
    crawlTask::registerGroup(group);
}
```

Current-video callbacks usually start with:

```cpp
const auto* video = webAPI::nowVideo();
if (video == nullptr || video->title() == nullptr) {
    return VideoStatus::KEEP;
}
```

## TOML Config Pattern

Use the SDK path and write helpers:

```cpp
char buffer[512]{};
toConfigPath(buffer, "plugin_name", sizeof(buffer), ".toml");

Config config;
cppUtil::setConfig(config, "enabled", true, std::string("是否启用该功能。"));
cppUtil::saveConfig(buffer, config);
```

For reading, load with `cppUtil::getConfig(...)` and parse with TOML helpers such as `toml::find_or<bool>(...)`.

## Validation

- Build the intended target.
- Run the repository copy/deploy target when present.
- Inspect the final `.so` with `nm -D` and confirm the expected hook names remain exported.
- If standalone `dlopen` or `ctypes.CDLL` fails on host-owned symbols, re-check inside the backend host before treating it as a plugin ABI failure. One locally observed unresolved host symbol is `webAPI::BrowseController::controller`.
