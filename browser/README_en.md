[中文](README.md)

# Browser Manager

This directory contains all code for the BrowserManager part of the project.

See also the backend overview in the root documentation:
- [Root README (Chinese)](../README.md)
- [Root README (English)](../README_en.md)

## Routes

| Path                   | JSON Response                                                                 | Body Parameters                               | Notes                                                                 |
|------------------------|-------------------------------------------------------------------------------|-----------------------------------------------|-----------------------------------------------------------------------|
| /                      | See the project README                                                        | See the project README                        | Main working endpoint                                                 |
| /test                  | `ok`: whether normal                                                          | See the project README                        | Test whether the service is available                                 |
| /other/closeWorker     | `ok`: whether normal                                                          | See the project README                        | Close the corresponding handler                                       |
| /other/testContext     | `ok`: whether normal                                                          | See the project README                        | Test the handler context data                                         |
| /screen                | Sets `X-Accel-Redirect` header; `url` is for nginx to access the login page   | URL params: `token`, `session` from `/login` | Intended for nginx; it returns the internal port and nginx proxies it |
| /other/login/backend   | `ok`, `context`: account info required by browser access                      | `token` returned by `/login`                  | Backend collects login result from client and closes the page         |
| /other/login           | `ok`: whether normal                                                          | See the project README                        | Open a login link for the client                                      |

## Environment Variables

| Variable         | Purpose                                          | Default               | Notes            |
|------------------|--------------------------------------------------|-----------------------|------------------|
| PORT             | Main service port                                | 3000                  |                  |
| PLUGIN_PATH      | Plugin config file path                          | `config/plugins.json` | Usually keep this |
| MAX_LOGIN_PORT   | Maximum number of login windows                  | 10                    |                  |
| LOGIN_SECONDS    | Backend idle expiration time during login (sec)  | 60                    |                  |
| COOKIE           | Cookie used by test code                         |                       | Test only        |
| USER_AGENT       | User-Agent used by test code                     |                       | Test only        |

## Plugins

### Installation

To install plugins, write plugin entries into the config file pointed to by `PLUGIN_PATH`. By default it is `config/plugins.json`.

```json
{
  "plugins": [
    "your-plugin-package-name",
    "../plugins-dev/local-plugin/dist/index.js"
  ]
}
```

Each item supports two forms:
- Package name, loaded by `require("package-name")`
- Local JavaScript path, resolved relative to the folder containing `plugins.json`

For example, if `PLUGIN_PATH` is `/crawler/config/plugins.json`, then:
- `"my-browser-plugin"` is loaded as an npm package
- `"../plugins-dev/demo-plugin/dist/index.js"` resolves to `/crawler/plugins-dev/demo-plugin/dist/index.js`

The current implementation only supports a string array. It does **not** yet support per-plugin option objects in this config file.

### Development

The browser side already has a basic plugin mechanism. A plugin can do two main things:
- Register new `Worker` types so the backend request `workers` array can use new `type` values
- Add routes, logs, or startup logic to the Fastify server during initialization

This section only documents abilities that are already implemented.

### Current Interfaces

The plugin entry API is defined in `src/PluginAPI.ts`. The most important interfaces are:
- `registerWorker(name, workerFactory)`: register a new worker type (corresponds to browser actions used by the backend)
- `Worker`: base class for all custom workers
- `ServerPlugin`: the object type a plugin should export as default
- `onServerInit(context)`: called when the server starts
- `onServerClose(context)`: called when the server is shutting down

As long as your plugin calls `registerWorker(...)` during startup, the main program can later find that worker by the request `type` field and execute your crawling logic.

### Packaging

Plugins should be prepared as Node.js-loadable JavaScript modules.

Typical flow:
- Write the plugin in TypeScript
- Compile it to JavaScript
- Let the browser service load it by package name or local JS file path

For local development, the easiest way is usually:
- Compile with `tsc -w`
- Point `plugins.json` to the generated `dist/index.js`

### Minimum Plugin Example

A usable plugin must default-export an object, and at minimum it needs `onServerInit`.

```ts
import { ServerPlugin } from "../../src/PluginAPI";

const plugin: ServerPlugin = {
  name: "my-browser-plugin",
  async onServerInit(context) {
    context.logger.info("plugin started successfully");
  }
};

export default plugin;
```

This means:
- `name` is the plugin name; it is optional but recommended
- `onServerInit` runs once when the service starts
- `context.logger` is the Fastify logger

### Registering a New Worker

To add a new worker, call `registerWorker`. After registration, when the backend sends a work request with matching `type` and `info`, your worker logic will be executed.

Example request body:

```json
{
  "clientID": "demo-client",
  "platform": "demo-platform",
  "context": {
    "cookie": "a=b"
  },
  "workers": [
    {
      "type": "ReadTitleWorker",
      "info": {
        "prefix": "Title: "
      }
    }
  ]
}
```

### What `handler` Can Do

The `handler` passed into `work(handler)` is the browser operation object managed by the main program. The most commonly used members are:
- `handler.newPage()`: create a new page
- `handler.page`: current page object; you can call Playwright APIs on it directly
- `handler.records`: recorded network responses
- `handler.stopRecord()`: stop recording network responses

So for plugin development, the main things you need are:
- Basic TypeScript classes
- `async/await`
- Some Playwright page operations

### Current Capability Boundary

The following are already supported:
- Plugin initialization at startup
- Plugin cleanup on shutdown
- Custom worker registration
- Custom HTTP route registration

The following are **not** yet provided as dedicated plugin APIs:
- Per-plugin option objects in `plugins.json`
- Dedicated config/data directories for each plugin
- Finer-grained hooks around each request
- A separate standalone SDK package only for plugin authors

### Development Notes

- The main program loads plugins with `require(...)`, so plugin output must be loadable in CommonJS mode
- Duplicate `registerWorker` names will fail, so do not reuse built-in worker names
- `workers[].info` is not validated automatically; validate fields in your constructor if needed
- If your worker depends on `handler.page`, make sure a page has already been created or call `handler.newPage()` yourself
- Local path plugins should point to built JavaScript files, not `.ts` source files

## Warning

Except for the remote desktop part (whose safety also depends on nginx), this module currently has no encryption of its own. Data is transmitted in plain text. Either deploy it together with the rest of the backend in one compose/network, or add your own protection.

If this part must be separated for any reason, make sure the container is placed behind nginx. Several behaviors in this module assume such a setup, and a fixed external entry is also easier to maintain.
