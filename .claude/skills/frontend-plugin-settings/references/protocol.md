# Bilibili-Reranger 前端插件设置协议（v1）

本协议供 backend 转发的 plugin 实现使用。前端入口为 `/plugins`；backend 负责外层请求和插件路由，插件主要处理 `data`。

## 发现与调用时序

1. 前端请求 `/plugins`，期待 `{ "plugin": ["plugin-name"] }`。
2. 前端分别向每个名称发送 `describe`。
3. 没有响应或空响应表示没有配置项；有非空响应但不合规表示该插件不使用通用协议。
4. 用户保存时，前端发送 `save`；请求成功后立即再次发送 `describe`，刷新状态和当前值。

因此 `describe` 必须幂等，`save` 应写入并校验配置，随后让下一次 `describe` 报告实际状态。

## 请求

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

保存时 `data` 为：

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

`action` 仅为 `describe` 或 `save`。`values` 是当前整张表单，不是变更字段的增量。

## describe 响应

```json
{
  "protocol": "frontend-plugin-settings",
  "version": 1,
  "name": "示例插件",
  "description": "用于演示插件配置",
  "status": { "type": "ready", "text": "运行正常" },
  "fields": [
    { "key": "enabled", "label": "启用插件", "type": "boolean", "default": true },
    {
      "key": "mode",
      "label": "模式",
      "type": "select",
      "options": [
        { "label": "安全", "value": "safe" },
        { "label": "快速", "value": "fast" }
      ],
      "default": "safe"
    }
  ],
  "values": { "enabled": true, "mode": "safe" },
  "submitLabel": "保存插件配置"
}
```

必需的顶层字段是 `protocol` 与 `version`。可选字段及默认行为：

| 字段 | 规则 |
| --- | --- |
| `name` | 字符串；缺失时使用插件名。 |
| `description` | 字符串；缺失时为空。 |
| `status` | 推荐 `{ type, text }`；见下文。 |
| `fields` | 数组；缺失时为空数组。若出现一个非法字段，整个响应不合规。 |
| `values` | 普通对象；缺失或非法时按每个字段的默认值归一化。 |
| `submitLabel` | 字符串；缺失时为“保存插件配置”。 |

## 字段定义和保存类型

公共属性：`key`（字符串）、`label`（字符串）、`type`（下列之一）；可选 `description`、`placeholder`（字符串）和 `default`（string / number / boolean / null）。

| `type` | 附加属性 | `save` 时的值 |
| --- | --- | --- |
| `string` | `input` 可为 `text`、`password`、`url` | JSON 字符串 |
| `number` | 无 | JSON 数字；清空输入为 `null` |
| `boolean` | 无 | `true` 或 `false` |
| `select` | 必需的非空 `options: [{ label: string, value: string }]` | 所选 `value` 的字符串 |

`input: "url"` 仅影响浏览器输入校验，不表示插件提供独立 URL 服务。插件应拒绝或安全回退无法识别的 select 值，并自行处理数字为 `null` 的情况。

## 状态与前端解析

推荐状态：

```json
{ "type": "warning", "text": "令牌即将过期" }
```

`type` 只能为 `ready`、`info`、`warning`、`error`，`text` 应是面向用户的短句。

前端也兼容字符串 `status`：若它等于上述类型之一，显示 `statusText` 或默认文本；其他字符串显示为 info 状态的文本。未提供或不合法的状态使用 info 和默认文本。

前端会归一化 `values`：缺失 string 显示空字符串；缺失 number 显示 `null`；缺失 boolean 使用布尔 default 或 `false`；缺失/无效 select 使用有效 default 或第一个选项。不要将这一容错当作插件侧校验的替代品。
