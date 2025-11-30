# Bilibili Reranger Backend

## Architecture

This is the backend code for the entire project, with an architecture based on C++. The choice of C++ was primarily to learn how to program in it. Project using third-party libraries:

1. [nlohmann-json](https://github.com/nlohmann/json) for handling various JSON text
2. [cpr](https://github.com/libcpr/cpr) for making Bilibili API requests and testing code
3. [CURL](https://github.com/curl/curl) for making Bilibili API requests
4. [boost](https://github.com/boostorg/boost) for continuously listening to ports and handling network requests

The functionality is mainly implemented by plugins. The main program acts as a framework that handles various routine tasks, while the plugins determine which specific videos to keep or remove.

## Usage

Docker deployment is strongly recommended. Use the packaged image for direct deployment with the following command:

```bash
docker run -p 23223:23223 -e COOKIE=<your_bilibili_cookie> -e USERAGENT=<browser_user_agent> -v <path_to_config>:/bilibili-backend/config -v <path_to_plugin>:/bilibili-backend/plugins docker.io/noname602/bilibili_reranger:latest
```

Currently, both `COOKIE` and `USERAGENT` are required. The `config` folder stores configuration files, and the `plugins` folder stores the loaded plugins. By default, there is an `EXAMPLE PLUGIN`.

## Plugins

There is an [example plugin](plugins/ExamplePlugin/main.cpp) in the code, providing interfaces in the [API dynamic link library](api/interface.h). All methods exported as C programs in the [API dynamic link library](api) are available for easy plugin development.

## URL Requests

All supported URL requests:

| URL Path      | Meaning                       | Supported Parameters (not mentioned parameters unsupported)          | Other                                                                              |
|---------------|-------------------------------|----------------------------------------------------------------------|------------------------------------------------------------------------------------|
| /all_category | Get all registered categories | No additional parameters                                             |                                                                                    |
| /set_cookie   | Set global COOKIE             | `COOKIE`: Set value, empty means reset to environment variable value | Only valid for this program run, does not change environment variable stored value |

Crawling parameters:

| Parameter  | Meaning                  | Other                                                 |
|------------|--------------------------|-------------------------------------------------------|
| category   | Category for this work   |                                                       |
| cookie_env | Set COOKIE for this work | Only valid for this work (different from /set_cookie) |