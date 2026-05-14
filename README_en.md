[Chinese](README.md)

BrowserManager docs: [Chinese](browser/README.md) / [English](browser/README_en.md)

# Backend Service

## Architecture
### Overall Architecture
This is the backend code for the whole project. The architecture is implemented in C++. The reason for choosing C++ is simple: I wanted to learn how to program in C++. Third-party libraries:
1. [nlohmann-json](https://github.com/nlohmann/json) for JSON serialization/deserialization
2. [cpr](https://github.com/libcpr/cpr) for Bilibili API requests and AI calls
3. [CURL](https://github.com/curl/curl) for low-level HTTP request support
4. [Boost](https://github.com/boostorg/boost) for port listening and HTTP handling (asio/beast/url)
5. [libsodium](https://github.com/jedisct1/libsodium) for login and encryption-related features
6. [OpenSSL](https://github.com/openssl/openssl) for RSA public-key encryption and key exchange
7. [libpqxx](https://github.com/jtv/libpqxx) for PostgreSQL access (used for client data and cookie persistence)

Most functionality is implemented through plugins. The main program is only a framework that handles miscellaneous work, while plugins decide whether specific videos should be kept or filtered out. The actual crawling is not performed by the C++ code either; it is done by another container running **BrowserManager**. The C++ code is mainly responsible for interacting with **BrowserManager**.
> Before January 28, 2026, this was implemented by reverse-engineering Bilibili. Later, the [reference project](https://github.com/SocialSisterYi/bilibili-API-collect) received a legal notice, and because compatibility with other platforms was also needed, the architecture was changed to the current one.

### **BrowserManager** Architecture
#### Work Sent by the Backend
At this point, each client handler corresponds to one backend browser. The mapping is stored in the database, so the related data is persisted long term. When writing a new handler, however, you only need to submit `workers`; the rest is already wrapped in `api/webAPIs/browse.cpp`.
Communication JSON format:
```json5
{
  "clientID": "client ID",
  "platform": "platform supported by the handler",
  "context": {
    // basic information required by the browser
  },
  "workers": [// later maintenance parameters are sometimes omitted
    // each worker corresponds to one item
    {
      "type": "worker type name",
      "info": {
        // custom JSON inside the worker
      }
    }
  ],
  "mode": "closeWorker" // optional; when omitted, the request path is used
}
```
#### Work Results Returned by the Browser
```json5
{
  "ok": true,// always present, indicates whether browser execution succeeded
  "error": {// may exist if an error occurs
    "name": "",
    "message": ""
  },
  "back": [// always present, although some requests return an empty array
    {
      // result of each worker; if one step has too many results, it may be an array
    },
    [
      // case where a worker has too many results
    ],
    [
      // special DoWhile node; loop results are wrapped here
      [
        // each item is one loop iteration, with the same structure as the outermost level
      ]
    ]
  ]
}
```
## Usage
Docker deployment is strongly recommended. Deploy directly with the packaged image:
```bash
docker run -p 23223:23223 -e BROWSE_URL=<your_browse_manager_url> -v <path_to_config>:/bilibili-backend/config -v <path_to_plugin>:/bilibili-backend/plugins docker.io/noname602/bilibili_reranger:latest
```
`BROWSE_URL` is required. It is the address of the browser group used by the program. With docker-compose deployment, the default internal address is `http://browser:3000`. The `config` folder stores configuration files, and the `plugins` folder stores loaded plugins. One `EXAMPLE PLUGIN` is included by default.
## Plugins
The code contains an [example plugin](plugins/ExamplePlugin/main.cpp). The interface is provided by the [API dynamic library](api/interface.h), and the C-exported methods in the [API dynamic library](api) are all available for plugin development.

### Local Plugin SDK
To customize video filtering, you need to develop your own plugin. There are two development approaches.
### Use the Development Container (Recommended)
`sdk.dockerfile` defines the development container for plugins. You can develop directly from the plugin development template provided by the `plugin-template` branch. It includes both Debug and Release SDK modes.
#### Local Development
You can first build a local SDK from backend artifacts, or download the SDK directly from the repository Release page, and then let a private plugin repository find it through CMake.

Specify the SDK path when configuring a private plugin, and use the same toolchain as the backend:
```bash
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH=/path/to/reranger-sdk \
  -DCMAKE_TOOLCHAIN_FILE=/usr/local/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
```
When implementing exported plugin functions, include `interface.h` directly and implement the fixed exported functions declared there. Some functions must be implemented.
## URL Requests
### Known Bug
The most important bug: the program uses the [Boost](https://github.com/boostorg/boost) library to parse `url` parameters. In my tests, its URL parameter parsing sometimes fails and produces completely incorrect results, making it impossible to get the expected parameters. The behavior also differs between IDE, command-line, Docker, and other startup methods, so some request errors may be caused by backend parsing errors. To avoid this bug, affected parameters are planned to be moved from `url` into `body`.
### Supported Requests
All supported URL requests:

| URL Path      | Meaning                                                                                                            | URL Parameters (except `id`, which is required by default; omitted parameters are unsupported)                                                                       | `Body` Parameters (usually encrypted)                                                                                                                           | Other                                                                                                         | Requires Long-Term Session Attention     | Status |
|---------------|--------------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------------------|------------------------------------------|--------|
| /all_category | Get all registered categories                                                                                      | No extra parameters                                                                                                                                                  |                                                                                                                                                                 | Must be called after setting the platform, otherwise it returns `null`                                        |                                          |        |
| /login        | Log in to the platform being crawled (obtain COOKIE, etc.)                                                         | No parameters other than `id` means get all supported platforms<br/>`test`: overrides all other parameters; used to test whether login is needed, enable with `true` | `platform`: platform to set (required every time; without it, all platforms are returned)<br/>`screen`: screen size (see `browser/src/login.ts` for details)    | Can vary by supported platform and plugin; extra parameters can be read by plugins                            | Required except when no `id` is provided |        |
| /key          | Exchange encryption keys with the backend                                                                          | No parameters                                                                                                                                                        | `key`: frontend symmetric encryption key (success returns the encrypted `id` parameter)<br/>send nothing to get the asymmetric public key<br/>`admin`: admin ID | The first connection uses asymmetric encryption; after that, symmetric encryption is used                     |                                          |        |
| /test         | Frontend checks various information                                                                                | `id` (required)                                                                                                                                                      | `key`: symmetrically encrypted key, used to verify that the key is correct after the client logs in again                                                       | HTTP 500 means failure, HTTP 200 means success                                                                | Required                                 |        |
| /init         | Initialize the crawler; if a login `token` exists, login information is collected and stored in the database first | `token`: the `token` parameter returned in the URL by `/login` (if absent, login-info collection is skipped)                                                         | No extra parameters                                                                                                                                             | Must be called after login                                                                                    | Required                                 |        |
| /set          | Set parameters                                                                                                     | `platform`: set the working platform                                                                                                                                 | No supported parameters                                                                                                                                         |                                                                                                               |                                          |        |
| /get          | Get the result of a previous long-term request                                                                     | `session`: request session returned earlier                                                                                                                          | No supported parameters                                                                                                                                         | `ok`: whether processing has failed<br/>`finished`: whether processing has completed<br/>`data`: related data |                                          |        |

> Note: when Session mode is required, a JSON object is returned first. Its `session` field is the corresponding session. Then request `/get` with this session until a result is returned. The session is destroyed after the request succeeds.

In addition, a return value of 500 indicates an internal processing error; check the response body for details. The Body section is encrypted as a whole (the entire JSON), rather than encrypting each field separately.

root router crawling parameters (for other URL paths):

| Parameter | Meaning                                          | Other                                                                                                                     |
|-----------|--------------------------------------------------|---------------------------------------------------------------------------------------------------------------------------|
| category  | Type of work for this request                    | Must be used with `/set`; only works for the current platform                                                             |
| id        | Value used by the backend to identify the client | Required by default for all requests except `/key`<br/>encryption applies to the whole body, not individual JSON fields   |
| prepared  | weather is the preload triggered by program      | with no message back，data stored in RAM and use for speed up in next request; no need for value, just search for this tag |

### Request Flow
First connect to `/key` to obtain the public key, then exchange the symmetric key and store `id`. Next, connect to `/login` to open the login browser, then call `/init` to store login state and initialize the backend. After that, use `category` to obtain videos from different categories.

## Admin
Administrators can configure their own admin key by modifying the `admin_client_key` field in the configuration file. Current administrator benefits:

| Type | Explanation | How to Set | Other |
|------|-------------|------------|-------|
| Data Sync | After administrator verification, the same `Client ID` can be shared, so every interface shows the same data without repeated login.<br/>The connection also never times out; even after a long time, its `ID` will not be cleared | Set through the `/key` request | |
