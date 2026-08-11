---
name: frontend-plugin-settings
description: 实现、审查或调试 Bilibili-Reranger 后端插件的前端通用设置接口。用于编写会响应 `/plugins` 的 `describe` 与 `save` 请求的插件，或排查插件设置卡片无法自动渲染、字段值异常、保存后未刷新等问题。Use when building or reviewing a plugin compatible with the `frontend-plugin-settings` protocol, version 1.
---

# Bilibili-Reranger 前端插件设置协议

实现插件设置接口前，先阅读 [完整协议](references/protocol.md)。该文件是可独立使用的协议快照；若正在本仓库工作，再以 `README.md` 和 `src/component/utils/pluginProtocol.ts` 为当前实现的最终依据。

## 实现步骤

1. 只处理后端转发给插件的 `data` 对象；外层 `{ "plugin": "…", "data": { … } }` 由 backend 处理。
2. 校验 `protocol === "frontend-plugin-settings"`、`version === 1`，并按 `action` 分支处理。
3. 对 `describe` 返回完整、严格符合协议的 JSON 描述对象；每次调用都从当前配置与当前运行状态构建它。
4. 对 `save` 校验整个 `values` 对象、持久化配置。不要依赖只提交变更字段；成功后前端会立即再调用一次 `describe`。
5. 交付前用协议中的最小示例核对字段类型、`select.options`、当前 `values` 与 `status`。

## 必须遵守的兼容性约束

- 固定协议名为 `frontend-plugin-settings`，版本固定为 JSON 数字 `1`。
- `describe` 的响应必须是 JSON 对象，且 `protocol`、`version` 正确；否则前端不会自动生成表单。
- 字段仅支持 `string`、`number`、`boolean`、`select`。每项必须有非空字符串 `key`、字符串 `label` 和有效 `type`。
- `select` 必须提供非空 `options`；每个选项必须是 `{ "label": string, "value": string }`。不要发送数字或对象 value。
- `values` 应当返回当前完整值。前端会按字段类型归一化缺失或不匹配的值，因此插件仍须在 `save` 时做最终校验。
- 数字字段被清空时，`save.values[key]` 会是 `null`；未知的选择值也可能被提交。两者都必须安全处理。
- `describe` 必须可重复调用且无副作用。想退出通用设置页时，对 `describe` 不返回任何内容；返回任意非空但不合规的内容会显示“未接入协议”。

## 排错顺序

当卡片未自动渲染时，依次核对：响应是否非空 JSON、协议名/版本、`fields` 是否为数组、每个字段是否合法、`select` 的选项是否完整。需要精确的前端归一化规则（例如 `status` 的兼容格式或缺失值回退）时，阅读完整协议的“前端解析与回退”部分。
