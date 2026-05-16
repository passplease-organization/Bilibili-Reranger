# Service Frontend
[中文](README.md)

***Now this branch is almost done by Codex***

## Architecture
Built with Vue and mainly written in TypeScript. I am still learning while building it.

The site is static. Serverless functions or an auxiliary Docker container are used to handle the `Referer` header when loading images from Bilibili; otherwise those images cannot be loaded. No other server-side logic is currently required.

## Deployment
### Deploy with Docker (Recommended)
Pull the main branch and follow the `docker-compose.yml` in it.

Advantage: visitors do not need to deploy an extra backend, and no default configuration changes are needed.

### Deploy with Vercel (Deprecated)
[![Deploy with Vercel](https://vercel.com/button)](https://vercel.com/new/clone?repository-url=https://github.com/passplease/Bilibili-Reranger/tree/frontend&repository-name=my-bilibili)

Uses Vercel serverless functions to load Bilibili images.

Drawback: visitors still need to deploy their own backend.

### Deploy with EdgeOne (Deprecated)
Fork the repository, then import it via the [EdgeOne deployment page](https://console.tencentcloud.com/edgeone/pages/create/git). Keep the remaining settings at their defaults.

Drawback: visitors still need to deploy their own backend.

## Usage
After opening the page, log in first, initialize the backend after logging in, and wait until initialization succeeds. Then you can enter the home page and start looking for videos. Avoid changing settings unless necessary.

For first-time use, configure all backend service URLs on the settings page (otherwise the app will not work). This is not required for docker-compose deployment.

## Plugin Configuration API
The backend now supports handling requests through `plugin`s. The frontend settings page will render an individual configuration card for each plugin that can receive requests. Since plugins may be implemented as C++ programs, this protocol is intentionally simple: plugins only need to return static JSON descriptions, while the frontend handles rendering and status display.

### `/plugins`
- Without parameters: return all plugins and their configuration descriptors
- With parameters: handle a configuration submission for the specified plugin

Request body fields:
- `plugin`: plugin name
- `data`: the full JSON configuration object to send to the plugin

Recommended response when listing plugins:

```json
{
  "ok": true,
  "plugins": [
    {
      "plugin": "example",
      "name": "Example Plugin",
      "description": "Used to demonstrate plugin configuration",
      "configurable": true,
      "status": "ready",
      "statusText": "Working normally",
      "fields": [
        {
          "key": "enabled",
          "label": "Enabled",
          "type": "bool",
          "value": true
        },
        {
          "key": "apiUrl",
          "label": "API URL",
          "type": "string",
          "input": "url",
          "value": "http://127.0.0.1:8080"
        },
        {
          "key": "mode",
          "label": "Mode",
          "type": "select",
          "value": "safe",
          "options": [
            { "label": "Safe", "value": "safe" },
            { "label": "Aggressive", "value": "aggressive" }
          ]
        }
      ]
    }
  ]
}
```

Field descriptions:
- `plugin`: unique plugin identifier
- `name`: display name shown in the frontend
- `description`: plugin description
- `configurable`: whether the frontend should render a configuration card
- `status`: current plugin status
- `statusText`: text description of the status
- `fields`: list of declared configuration fields

Recommended field types supported by the frontend:
- `bool`
- `string`
- `number`
- `select`

Meaning of each type:
- `bool` for toggle-like configuration
- `string` for plain text input
- `number` for numeric input
- `select` for enumerated options, which must also provide `options`

Example request when submitting plugin configuration:

```json
{
  "plugin": "example",
  "data": {
    "enabled": true,
    "apiUrl": "http://127.0.0.1:8080",
    "mode": "safe"
  }
}
```

Example success response:

```json
{
  "ok": true,
  "plugin": "example",
  "status": "ready",
  "statusText": "Saved successfully",
  "errors": []
}
```

Example failure response:

```json
{
  "ok": false,
  "plugin": "example",
  "status": "error",
  "statusText": "Configuration validation failed",
  "errors": [
    {
      "key": "apiUrl",
      "message": "The URL cannot be empty"
    }
  ]
}
```

The following status values are recommended so the frontend can determine plugin state directly:
- `ready`
- `unconfigured`
- `disabled`
- `error`
