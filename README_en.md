[中文](README.md)

# Bilibili Reranger Backend

## Architecture

This is the backend code for the entire project, with an architecture based on C++. The choice of C++ was primarily to learn how to program in it. Project using third-party libraries:

1. [nlohmann-json](https://github.com/nlohmann/json) for JSON serialization and deserialization
2. [cpr](https://github.com/libcpr/cpr) for Bilibili API requests and AI calls
3. [CURL](https://github.com/curl/curl) for low-level HTTP support
4. [boost](https://github.com/boostorg/boost) for port listening and HTTP handling (asio/beast/url)
5. [libsodium](https://github.com/jedisct1/libsodium) for login and encryption features

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

| URL Path      | Meaning                                           | URL Parameters (except `id` is required by default, unspecified means unsupported)                                        | `Body` Parameters (usually encrypted)                                                               | Other                                                                                     | Status |
|---------------|---------------------------------------------------|---------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------|--------|
| /all_category | Get all registered categories                     | No additional parameters                                                                                                  |                                                                                                     |                                                                                           |        |
| /login        | Login to the crawling platform (get COOKIE, etc.) | No parameters means fetch all supported platforms (no `id` required)<br/>`platform`: platform to set (required each time) | `username`: account<br/>`password`: password                                                        | Varies by platform and plugin; extra parameters can be handled by the plugin              |        |
| /key          | Exchange encryption keys with backend             | No parameters means get RSA public key                                                                                    | `key`: ESA-encrypted symmetric key (success returns encrypted `id` parameter)<br/>`admin`: admin id | Use RSA for initial handshake, then use symmetric encryption for subsequent communication |        |
| /test         | Frontend checks various information               | `id` (required)                                                                                                           | `key`: ESA-encrypted key for validating after client re-login                                       | 500 indicates failure; 200 indicates success                                              |        |
| /init         | Initialize crawler                                | No additional parameters                                                                                                  | No additional parameters                                                                            | Must be called after `/login`                                                             |        |
| /set          | Set parameters                                    | `platform`: set working platform                                                                                          | No supported parameters                                                                             |                                                                                           |        |

Additionally, a return value of 500 indicates an internal processing error; check the response body for details.

Crawling parameters (for other URL paths):

| Parameter  | Meaning                          | Other                                                                                          |
|------------|----------------------------------|------------------------------------------------------------------------------------------------|
| category   | Category for this work           | Must be used with `/set`, only works for the current platform                                  |
| cookie_env | Set COOKIE for this work         | Only valid for this work (different from /set_cookie)                                          |
| id         | Backend value to identify client | Required by default for all except `/key`<br/>Encryption applies to the whole body, not fields |

## Login

Required login parameters and meanings (in addition to the common parameters above).

### Bilibili

| Parameter | Meaning                            | Other |
|-----------|------------------------------------|-------|
| validate  | One of the captcha parameters      |       |
| seccode   | One of the captcha parameters      |       |
| token     | One of the captcha request params  |       |
| challenge | One of the captcha request params  |       |

The first login request is made by the backend to Bilibili, then returned to the frontend. After the user completes verification, the frontend sends the results back and the backend finishes login and returns the final result.<br><br>
The first request does not need `username` and `password`. The last login request must include them; if both are absent, it is treated as the initial captcha-parameter request.

### Request Flow

First connect to `/key` to get the public key, then exchange the ESA key and store `id`. Next call `/login` multiple times until login succeeds, then call `/init` to initialize the backend, and finally use `category` to fetch videos for different categories.

## Admin

Admins can configure their own key by modifying the `admin_client_key` field in the config file. Admins currently have the following benefits:

| Type      | Explanation                                                                                                                                                                                                 | How to Set                 | Other |
|-----------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|----------------------------|-------|
| Data Sync | After admin verification, the same `Client ID` is shared, so views are consistent and no repeated logins are needed.<br/>The connection never expires, even long after the `ID` would otherwise be cleared. | Set via the `/key` request |       |
