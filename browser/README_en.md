[Chinese](README.md)

See root documentation: [Chinese](../README.md) / [English](../README_en.md)

## Purpose
This folder contains all code for the BrowseManager part.
## Routes
| Path                 | Returned JSON | Body Parameters | Notes |
|----------------------|---------------|-----------------|-------|
| /                    | See project README | See project README | General work |
| /test                | `ok`: whether normal | See project README | Test whether the service is available |
| /other/closeWorker   | `ok`: whether normal | See project README | Close the corresponding handler |
| /other/testContext   | `ok`: whether normal | See project README | Test the COOKIE corresponding to the handler; currently exactly the same as `/` |
| /screen              | Sets the `X-Accel-Redirect` header<br/>`url`: used by nginx to access the specific login page | URL parameters:<br/>`token`, `session`: both are returned by `/login` | Dedicated to nginx. It returns the corresponding port, and nginx accesses it |
| /other/login/backend | `ok`: whether normal, `context`: account information required by browser access | `token`: returned by `/login` | Backend obtains the client login result and closes the page directly |
| /other/login         | `ok`: whether normal | See project README | Open a login link for the client |
## Environment Variables
| Variable | Purpose | Default | Notes |
|----------|---------|---------|-------|
| PORT | Port listened to by the main program | 3000 | |
| PLUGIN_PATH | Plugin config file path | config/plugins.json | Not recommended to change |
| MAX_LOGIN_PORT | Maximum number of login windows | 10 | |
| MAX_LOGIN_PORT | Maximum number of login windows | 10 | |
| LOGIN_SECONDS | Backend idle expiration time during client login | 60 seconds | |
| COOKIE | Cookie used by test code | | Test code only |
| USER_AGENT | User-Agent used by test code | | Test code only |
| WAITING_TIME | Configures the wait time for related services to start when a client logs in | 5000 milliseconds | |
## Plugins
### Installation
To install plugins, write plugin names into the config file corresponding to `PLUGIN_PATH` (default: `config/plugins.json`). The config format is:
```json
{
  "plugins": [
    "just write the plugin name",
    "a relative path is also allowed, and it will be converted to a path relative to the config JSON folder"
  ]
}
```
### Development
The browser side currently supports a basic plugin mechanism. Plugins can do two things:
- Register new `Worker` types, so the `workers` array in backend requests supports new `type` values
- Add their own routes, logs, or other initialization logic to `fastify` when the service starts

### Current Interfaces
The plugin entry code is in `src/PluginAPI.ts`. The most important interfaces for plugin development are:

- `registerWorker(name, workerFactory)`: registers a new worker type (corresponding to the BrowseAction class in backend)
- `Worker`: base class for all custom workers
- `ServerPlugin`: object type exported by plugins by default
- `onServerInit(context)`: called when the service starts
- `onServerClose(context)`: called when the service shuts down

As long as a plugin calls `registerWorker(...)` during startup, the main program can later find your worker through the `type` field in requests and execute your crawling strategy.

### Packaging
Plugins need to be packaged as Node.js packages so the main program can load them.

- First compile the plugin to JavaScript
- Then let the main project `require("your plugin name")`

The current implementation only supports writing plugin name strings. It does **not** yet support passing separate parameters for each plugin in this config file.

### Minimum Requirements
A usable plugin needs to default-export an object. This object must at least have `onServerInit`, for example:
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

- `name` is the plugin name. It can be omitted, but it is recommended
- `onServerInit` runs once when the service starts
- `context.logger` is the fastify logger and can be used directly

### New Worker
Registering a Worker means calling `registerWorker`. After registration succeeds, when the backend sends a work request whose `type` and `info` correspond to your newly registered Worker, the corresponding logic can run.

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
        "prefix": "Page title: "
      }
    }
  ]
}
```

The main program automatically executes the worker you registered.

### Current Version Capability Boundary
The following are **already supported**:

- Plugin initialization at startup
- Plugin resource cleanup on shutdown
- Registering custom workers
- Registering custom HTTP routes
## Warning
Except for the remote desktop part (whose safety also depends on nginx assistance), this part has no encryption measures. Data is transmitted in plaintext. Either deploy it with the rest of the backend in the same compose setup or LAN, or add your own security protection. Users should also make sure that their login address is safe.

If this part must be separated for any reason, make sure the container is proxied by nginx. Related behavior is based on this assumption, and a fixed access endpoint is easier to use.
