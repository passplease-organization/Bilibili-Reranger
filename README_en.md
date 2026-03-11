[中文](README.md)

BrowserManager docs: [中文](browser/README.md) / [English](browser/README_en.md)

# Backend Service

## Architecture
### Overall Architecture
This is the backend code for the whole project. The architecture is implemented in C++. The reason for choosing C++ is simple: I wanted to learn how to program in C++. Third-party libraries used:
1. [nlohmann-json](https://github.com/nlohmann/json) for JSON serialization/deserialization
2. [cpr](https://github.com/libcpr/cpr) for Bilibili API requests and AI calls
3. [CURL](https://github.com/curl/curl) for low-level HTTP request support
4. [Boost](https://github.com/boostorg/boost) for port listening and HTTP handling (asio/beast/url)
5. [libsodium](https://github.com/jedisct1/libsodium) for login and encryption-related features
6. [OpenSSL](https://github.com/openssl/openssl) for RSA public-key encryption and key exchange
7. [libpqxx](https://github.com/jtv/libpqxx) for PostgreSQL access (used for client data and cookie persistence)

Most functionality is implemented through plugins. The main program is just a framework that handles various routine tasks, while the plugins decide which videos should be kept or removed. The actual crawling work is also not performed by the C++ code itself, but by another container where **BrowserManager** runs. The C++ side is mainly responsible for interacting with **BrowserManager**.
> Before January 28, 2026, this project was based on Bilibili reverse engineering. Later, the [reference material](https://github.com/SocialSisterYi/bilibili-API-collect) received a legal notice, and considering compatibility with other platforms, the architecture was changed to the current one.

### **BrowserManager** Architecture
#### Work Sent by the Backend
At this stage, each client handler corresponds to one backend browser. That mapping is stored in the database, and the related data is therefore persisted for a long time as well. However, when creating a new handler, you only need to submit `workers`; the rest has already been wrapped in `api/webAPIs/browse.cpp`.

Communication JSON format:
```json5
{
  "clientID": "client ID",
  "platform": "platform supported by the handler",
  "context": {
    // basic information required by the browser
  },
  "workers": [// sometimes the maintenance parameters below are omitted
    // each worker corresponds to one item
    {
      "type": "worker type name",
      "info": {
        // custom JSON defined by the worker itself
      }
    }
  ],
  "mode": "closeWorker" // optional; when omitted, the request path is used instead
}
```
#### Work Results Returned by the Browser
```json5
{
  "ok": true,// always present, indicates whether browser execution succeeded
  "error": {// may appear if an error occurs
    "name": "",
    "message": ""
  },
  "back": [// always present, though some requests may return an empty array
    {
      // result of one worker; if one step returns too much, it may be an array
    },
    [
      // case where a worker returns multiple results
    ],
    [
      // special case for DoWhile; each loop result is wrapped inside here
      [
        // each item represents one loop iteration, and its structure matches the outermost one
      ]
    ]
  ]
}
```
## Usage
Docker deployment is strongly recommended. Use the packaged image directly with the following command:
```bash
docker run -p 23223:23223 -e BROWSE_URL=<your_browse_manager_url> -v <path_to_config>:/bilibili-backend/config -v <path_to_plugin>:/bilibili-backend/plugins docker.io/noname602/bilibili_reranger:latest
```
`BROWSE_URL` is required. It represents the address of the browser manager service group. When deployed with `cdocker-composebushu`, the default internal address is `http://browse_manager`. The `config` folder stores configuration files, and the `plugins` folder stores the loaded plugins. One `EXAMPLE PLUGIN` is included by default.
## Plugins
There is an [example plugin](plugins/ExamplePlugin/main.cpp) in the codebase. The plugin interface is provided in the [API dynamic library](api/interface.h), and the methods exported as C functions from the [API dynamic library](api) are available for plugin development.
## URL Requests
### Known Bug
The most important bug is this: the program uses the [Boost](https://github.com/boostorg/boost) library to parse URL parameters. In my testing, its URL parameter parsing sometimes fails and produces completely incorrect parsing results, so the expected parameters cannot be obtained. The behavior can also differ depending on whether the program is started from an IDE, the command line, or Docker. Therefore, some request errors may actually be caused by incorrect backend parsing. To avoid this bug, the plan is to move some affected parameters from the `url` into the `body`.
### Supported Requests
All supported URL requests:

| URL Path      | Meaning | URL Parameters (except `id`, which is required by default; omitted parameters are unsupported) | `Body` Parameters (usually encrypted) | Other | Status |
|---------------|---------|--------------------------------------------------------------|---------------------------------------|-------|--------|
| /all_category | Get all registered categories | No extra parameters | | Must be called after the platform is set, otherwise it returns `null` | |
| /login | Log in to the target platform (obtain COOKIE, etc.) | No parameters other than `id` means fetch all supported platforms<br/>`test`: this option overrides all other parameters and is used to test whether login is needed; enable with `true` | `platform`: the platform to set (required every time) | Can vary depending on supported platform and plugin; extra parameters may be handled by the plugin itself | |
| /key | Exchange encryption keys with the backend | No parameters at all | `key`: frontend symmetric encryption key (success returns the encrypted `id` parameter)<br/>sending nothing means obtaining the asymmetric public key<br/>`admin`: admin ID | Use asymmetric encryption for initial connection, then symmetric encryption for later communication | |
| /test | Frontend checks various information | `id` (required) | `key`: symmetrically encrypted key, used to verify key correctness after the client logs in again | HTTP 500 means failure, HTTP 200 means success | |
| /init | Initialize the crawler | No extra parameters | No extra parameters | Must be called after login | |
| /set | Set parameters | `platform`: set the working platform | No supported parameters | | |

In addition, a return value of 500 indicates an internal processing error; please check the response body for details. Also, the Body section is encrypted as a whole (the entire JSON is encrypted), rather than encrypting each field in the JSON separately.

Crawling parameters (for other URL paths):

| Parameter | Meaning | Other |
|-----------|---------|-------|
| category | Type of work for this request | Must be used together with `/set`, and only works for the current platform |
| cookie_env | Set the COOKIE for this request | Only valid for the current request (different from `/set_cookie`) |
| id | Value used by the backend to identify the client | Required by default for all requests except `/key`<br/>encryption is applied to the whole body, not to individual JSON fields |

## Login
Parameters required for login and their meanings (excluding the common parameters mentioned above)
### Bilibili
Relevant code is in `src/platforms/bilibiliLogin.cpp`

| Parameter | Meaning | Other |
|-----------|---------|-------|
| validate | One of the captcha parameters | |
| seccode | One of the captcha parameters | |
| token | One of the parameters used to obtain the captcha | |
| challenge | One of the parameters used to obtain the captcha | |

For the first login request, the backend makes the request to Bilibili on behalf of the frontend and then returns the result to the frontend. After the user completes verification on the frontend, the frontend sends the result back, and the backend completes the final login process and returns the login result to the frontend.<br><br>
Bilibili login flow:
- Access the backend and obtain the captcha
- Submit the captcha values (4 in total), account name, and password to log in

Sometimes Bilibili returns an unsafe status and requires phone verification. In that case, additional steps are needed, and the previous step will return a newly obtained captcha:
- Perform the human verification again using the captcha returned in the previous step. On top of the parameters from the previous steps, add `phone_verification` to the Body; its content does not matter, the key point is that this field exists. The backend will then complete human verification and send the SMS code.
- The user receives the verification code and sends it back to the backend, and the backend sets the COOKIE.

### Request Flow
You should first connect to `/key` to obtain the public key, then exchange the symmetric key and store `id`. After that, connect to `/login` repeatedly until login succeeds, then call `/init` to initialize the backend, and finally use `category` to obtain videos of different categories.

## Admin
Administrators can configure their own admin key by modifying the `admin_client_key` field in the configuration file. Administrators currently have the following advantages:

| Type | Explanation | How to Set | Other |
|------|-------------|------------|-------|
| Data Sync | After admin verification, the same `Client ID` can be shared, so what you see is the same in every interface and repeated login is unnecessary.<br/>Also, the connection never times out, and even after a very long time the `ID` will not be cleared | Set through the `/key` request | |
