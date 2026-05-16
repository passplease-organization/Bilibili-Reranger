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
The backend now supports handling requests through `plugin`s. There are two layers here:
- Backend-fixed standard: the outer request format is always `{ "plugin": "plugin-name", "data": { ... } }`
- Frontend generic standard: if the content inside `data` follows the protocol below, the frontend can render a generic settings card automatically

This section is written for backend and plugin developers. It explains:
- how the frontend calls a plugin
- what a plugin should return for automatic rendering
- what values a plugin receives on save
- when and how the frontend refreshes plugin state

### `/plugins`
- If the request body does not match the plugin request format, the backend returns the full plugin list
- If the request body matches the plugin request format, the backend forwards the request to the target plugin

The plugin request format must be exactly:

```json
{
  "plugin": "example",
  "data": {
    "anything": "decided by plugin"
  }
}
```

Field meanings:
- `plugin`: plugin name
- `data`: a JSON object fully defined by the plugin itself; the frontend does not standardize its internal structure

Any request that does not match the format above will cause the backend to return the full plugin list in this form:

```json
{
  "plugin": ["plugin-name"]
}
```

Frontend adaptation rules:
- If the response looks like `{ "plugin": ["a", "b"] }`, treat it as the plugin list result
- If a specific plugin is requested and there is no reply from that plugin, treat it as “this plugin has no configurable items”
- If a specific plugin replies, the reply format is entirely defined by that plugin; if it wants to be rendered automatically by the generic frontend settings page, it should follow the frontend protocol below

In other words, this layer only guarantees:
- a fixed request format from the frontend to a plugin
- a fixed plugin-list response format
- “no reply” means “no configuration items”

Everything else, including configuration fields, content shape, and status representation for an individual plugin, is defined by the plugin itself.

### Frontend Standard Inside `data`
To let the frontend generate plugin settings cards automatically, the frontend currently expects the following protocol inside `data`. Everything outside `data` is handled by the backend, so the plugin only needs to care about the `data` object itself.

```json
{
  "plugin": "example",
  "data": {
    "protocol": "frontend-plugin-settings",
    "version": 1,
    "action": "describe"
  }
}
```

Field meanings:
- `protocol`: must be `frontend-plugin-settings`
- `version`: currently fixed to `1`
- `action`: the operation initiated by the frontend
- `values`: a field-value object submitted when `action` is `save`; keys correspond one-to-one with `fields[].key`

Currently supported actions:
- `describe`: request the plugin's settings description, status, and current values
- `save`: submit the current form values

### Frontend Request Flow
The settings page currently behaves like this:

1. The frontend requests `/plugins`
2. If the backend returns `{ "plugin": ["a", "b"] }`, the frontend treats it as the plugin-name list
3. The frontend then sends a separate `describe` request for each plugin
4. If a plugin does not reply at all, the frontend treats it as “this plugin has no configurable items”
5. If a plugin replies but the format does not follow this document, the frontend keeps the card but does not auto-render a form
6. When the user clicks save, the frontend sends a `save` request
7. After a successful `save`, the frontend immediately sends another `describe` request for that same plugin to refresh the latest status and values

In practice, a plugin should assume that:
- `describe` may be called many times, not just once when the page opens
- a successful `save` is usually followed by another `describe` very soon

Example save request:

```json
{
  "plugin": "example",
  "data": {
    "protocol": "frontend-plugin-settings",
    "version": 1,
    "action": "save",
    "values": {
      "enabled": true,
      "mode": "safe"
    }
  }
}
```

### What the Plugin Actually Receives on `save`
`values` is a plain object. Each key comes from the `key` declared in `fields`. The frontend does not send extra wrappers or dirty-field metadata; it submits the full current form state each time.

Example:

```json
{
  "protocol": "frontend-plugin-settings",
  "version": 1,
  "action": "save",
  "values": {
    "enabled": true,
    "mode": "safe",
    "retryCount": 3
  }
}
```

The actual value rules are:
- `string`: always a JSON string
- `number`: always a JSON number; if the user clears the input, the plugin receives `null`
- `boolean`: always `true` or `false`
- `select`: always a JSON string equal to the selected `options[].value`

Do not assume that:
- a `number` field is always non-empty
- a `select` value is always still valid for your backend logic

The plugin should perform final validation and fallback handling itself.

### Plugin Description Response Format
If a plugin wants to be rendered automatically as a generic settings form, it should return something like this after receiving `describe`:

```json
{
  "protocol": "frontend-plugin-settings",
  "version": 1,
  "name": "Example Plugin",
  "description": "Used to demonstrate plugin configuration",
  "status": {
    "type": "ready",
    "text": "Working normally"
  },
  "fields": [
    {
      "key": "enabled",
      "label": "Enable Plugin",
      "type": "boolean",
      "default": true
    },
    {
      "key": "mode",
      "label": "Mode",
      "type": "select",
      "options": [
        { "label": "Safe", "value": "safe" },
        { "label": "Fast", "value": "fast" }
      ],
      "default": "safe"
    }
  ],
  "values": {
    "enabled": true,
    "mode": "safe"
  },
  "submitLabel": "Save Plugin Settings"
}
```

Response field meanings:
- `protocol`: must be `frontend-plugin-settings`
- `version`: must be `1`
- `name`: card title; if omitted, the frontend falls back to the plugin name
- `description`: card description; may be an empty string
- `status`: current plugin status
- `fields`: field-definition array
- `values`: current field-value object
- `submitLabel`: text shown on the save button; optional

### Field Definition Rules
Each field must at least provide:

```json
{
  "key": "enabled",
  "label": "Enable Plugin",
  "type": "boolean"
}
```

Supported field properties:
- `key`: unique field identifier; used as the key inside `values` during save
- `label`: display label shown in the frontend
- `type`: field type
- `description`: optional field description
- `placeholder`: optional placeholder; meaningful for `string` and `number`
- `input`: input type; meaningful only for `string`, currently `text`, `password`, or `url`
- `options`: required for `select`
- `default`: optional default value

### How Each Field Type Renders and Saves
`string`
- Rendered as a single-line input
- If `input` is omitted, it is treated as a plain text input
- `input: "password"` renders as a password field
- `input: "url"` renders as a URL input; this only affects frontend validation and does not imply that the plugin runs as a standalone service
- Saved as a JSON string

`number`
- Rendered as a numeric input
- Saved as a JSON number when the input contains a valid number
- Saved as `null` when the user clears the field
- `default` should be a number or `null`

`boolean`
- Rendered as a checkbox
- Always saved as `true` or `false`
- `default` should be a boolean

`select`
- Rendered as a dropdown
- `options` is required
- Each option must be `{ "label": "shown text", "value": "actual value" }`
- Saved as the selected `value` string
- `default` should be one of the `options[].value` entries

### `values` Rules
`values` tells the frontend the current effective values. It should be a plain object without extra metadata.

Example:

```json
{
  "values": {
    "enabled": true,
    "retryCount": 3,
    "mode": "safe"
  }
}
```

The frontend handles missing values as follows:
- if a field is missing in `values`, the frontend falls back to `default`
- missing `string` becomes an empty string if no `default` is available
- missing `number` becomes empty if no `default` is available
- missing `boolean` becomes `false` or `default`
- missing `select` falls back to `default`, then to the first available option

### `status` Rules
Recommended shape:

```json
{
  "status": {
    "type": "ready",
    "text": "Working normally"
  }
}
```

Supported status types:
- `ready`
- `info`
- `warning`
- `error`

`text` is displayed directly in the card status area, so it should be a short user-facing sentence, not a debug log.

### Plugin Development Notes
- `describe` should be idempotent and safe to call repeatedly
- `save` should ideally validate and persist, then leave the final refreshed state to the next `describe`
- if a plugin wants to stay outside the generic frontend settings flow, simply do not reply to `describe`
- if a plugin needs a fully custom UI, it may still return custom content; the current frontend will keep the card but mark it as not using the generic frontend protocol

If a plugin replies with content but does not follow this protocol, the frontend will still keep the plugin card, but it will not auto-generate a settings form and will instead indicate that the plugin has not adopted the generic frontend protocol.
