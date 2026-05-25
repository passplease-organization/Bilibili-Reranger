# Bilibili-Reranger SDK Notes

## Contents

1. Local SDK layout
2. Plugin ABI headers
3. Crawl task registration
4. Current video access
5. SDK TOML config pattern
6. Build and deployment checks
7. Host-loading caveat

## 1. Local SDK Layout

- SDK root: `/opt/bilibili-reranger-sdk`
- Backend root: `/opt/bilibili-reranger-backend`
- Release headers: `/opt/bilibili-reranger-sdk/release/include/BiliBili_Reranger`
- Debug headers: `/opt/bilibili-reranger-sdk/debug/include/BiliBili_Reranger`
- Release shared library: `/opt/bilibili-reranger-sdk/release/lib/libBiliBili_Reranger_API.so`
- Debug shared library: `/opt/bilibili-reranger-sdk/debug/lib/libBiliBili_Reranger_API.so`

Inspect release headers by default. Switch to debug headers only when investigating debug-specific build/runtime behavior.

## 2. Plugin ABI Headers

`interface.h` declares the exported hooks the host looks for:

- Required: `extern "C" PluginStatus load()`
- Optional: `registerGroups()`
- Optional: `roughJudge()`
- Optional: `judge()`
- Optional/deprecated or specialized: `getURL()`, `getWorker()`, `dealJson()`

`pluginInterface.h` defines:

- `PluginStatus::{FAIL,SUCCESS,PASS}`
- `VideoStatus::{KEEP,THROW,UNKNOWN}`
- `crawlTask::WorkingMode::{SEARCH,SUBSCRIBE,TAG,HOME_PAGE_FILTER}`
- `crawlTask::Task`
- `crawlTask::Group`
- Registration helpers such as `crawlTask::registerGroup()`

Keep exported entry points `extern "C"` so dynamic lookup stays stable.

## 3. Crawl Task Registration

For homepage filtering, the common shape is:

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

Adjust the group/task labels and video count to the plugin design. Preserve the working mode unless intentionally switching feature class.

## 4. Current Video Access

`utils/BilibiliInterface.h` exposes:

- `webAPI::nowVideo()`
- `webAPI::Video`
- Accessors such as `title()`, `author()`, `description()`, `url()`, `duration()`, `publishTime()`, `views()`
- Raw JSON through `getJson()`
- Conversion helpers such as `Video::fromJson()`

Typical callback usage:

```cpp
extern "C" VideoStatus judge() {
    const auto* video = webAPI::nowVideo();
    if (video == nullptr || video->title() == nullptr) {
        return VideoStatus::KEEP;
    }
    return VideoStatus::KEEP;
}
```

Use accessors first. Read `getJson()` only for fields not already surfaced by the SDK.

## 5. SDK TOML Config Pattern

`utils/configUtil.h` provides:

- `makeConfigDir()`
- `toConfigPath(...)`
- `Config`, aliased to `toml::value`
- `cppUtil::getConfig(...)`
- `cppUtil::saveConfig(...)`
- `cppUtil::setConfig(...)`

Recommended pattern:

```cpp
#include <utils/configUtil.h>
#include <toml11/find.hpp>

std::string pluginConfigPath(const char* name, const char* fileType = ".toml") {
    char buffer[512]{};
    toConfigPath(buffer, name, sizeof(buffer), fileType);
    return buffer;
}

Config makeConfig(const bool enabled) {
    Config config;
    cppUtil::setConfig(
        config,
        "enabled",
        enabled,
        std::string("是否启用该插件功能。")
    );
    return config;
}

bool loadEnabled() {
    (void)makeConfigDir();
    const std::string path = pluginConfigPath("plugin_name");
    if (!std::filesystem::exists(path)) {
        cppUtil::saveConfig(path, makeConfig(true));
        return true;
    }
    const Config config = cppUtil::getConfig(path);
    return toml::find_or<bool>(config, "enabled", true);
}
```

Practical notes:

- `setConfig` is the preferred way to produce TOML defaults with comments.
- `getConfig` returns `Config`; parse with TOML accessors instead of handwritten text parsing.
- `toConfigPath("foo", ".json")` is also valid when a plugin intentionally keeps some auxiliary data in JSON.
- Runtime config should live under the SDK-resolved config directory rather than tracked source.

## 6. Build And Deployment Checks

Repository-specific commands vary, but this plugin template currently uses:

```bash
cmake --build cmake-build-debug --target VIDEO_PLUGIN
cmake --build cmake-build-release --target VIDEO_PLUGIN_COPY_DLL
nm -D running_path/plugin/libVIDEO_PLUGIN.so | grep -E ' load$| registerGroups$| roughJudge$| judge$'
```

Confirm:

- The shared library links.
- The copy target places the `.so` where the backend mounts plugin artifacts.
- The required `load` symbol remains exported.
- Optional hooks expected by the plugin are still exported.

## 7. Host-Loading Caveat

The SDK shared library may reference host-owned symbols supplied by the backend process. Because of that:

- A direct standalone `dlopen`, Python `ctypes.CDLL`, or similar probe can fail with unresolved host symbols even when the plugin is valid inside the backend.
- Example failure shape observed locally: unresolved `webAPI::BrowseController::controller`.
- Use the backend host, linked integration flow, or dependency inspection before treating that as a plugin defect.
