# 服务前端
[English](README_en.md)

***这部分当前主要靠Codex完成***

## 架构
基于Vue搭建，主要使用TS代码（但是我也是边写边学）

网站内容是静态的，部署中的无服务器函数或者辅助docker容器主要是处理加载B站视频图片时的Referer，否则B站不允许加载图片，除此以外暂无他用。
## 部署
### 使用Docker部署（推荐）
拉取主分支，参考其中的`docker-compose.yml`文件即可

优点：不必访问者再部署一个后端，且不用更改默认配置
### 使用Vercel部署（已弃用）
[![Deploy with Vercel](https://vercel.com/button)](https://vercel.com/new/clone?repository-url=https://github.com/passplease/Bilibili-Reranger/tree/frontend&repository-name=my-bilibili)

使用Vercel的无服务器函数加载B站视频

缺点：还需要访问网站者自己部署一个后端
### 使用EdgeOne部署（已弃用）
首先Fork仓库，然后去[EdgeOne部署页面](https://console.tencentcloud.com/edgeone/pages/create/git)导入仓库，其余配置保持默认即可。

缺点：还需要访问网站者自己部署一个后端
## 使用
进入页面首先进行登录，登录后初始化后台，等到后台初始化成功即可进入主页找视频看了，若非必要不要乱改设置

初次使用需去设置页面配置全部的后端服务地址（否则无法工作）（docker-compose部署则不需要）

## 插件配置接口
### `data` 字段的前端通用协议
为了让前端可以为插件自动生成设置卡片，当前前端约定 `data` 内部使用下面的协议。其余外层字段由 `backend` 处理，`plugin` 实际只需要关心 `data` 这一段。

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

字段说明：
- `protocol`：固定为 `frontend-plugin-settings`
- `version`：当前固定为 `1`
- `action`：前端发起的动作
- `values`：当 `action` 为 `save` 时提交的字段值对象，键名与 `fields[].key` 一一对应

当前前端支持的动作：
- `describe`：请求插件返回自己的设置描述、状态和当前值
- `save`：提交当前表单值

### 前端请求时序
当前 settings 页的前端行为固定如下：

1. 前端先请求 `/plugins`
2. 如果后端返回 `{ "plugin": ["a", "b"] }`，前端把它当作插件名列表
3. 前端会对列表中的每个插件单独再发一次 `describe`
4. 如果某个插件没有任何回信，前端认为它“没有配置项”
5. 如果某个插件有回信，但格式不符合本文档协议，前端保留这个插件卡片，但不会自动生成表单
6. 用户点击保存后，前端发送一次 `save`
7. `save` 请求成功后，前端会立即再次对该插件发一次 `describe`，用来刷新最新状态和当前值

换句话说，plugin 需要假设：
- `describe` 会被多次调用，不只是第一次打开页面
- `save` 成功后，很快还会再收到一次 `describe`

保存请求示例：

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

### `save` 时 plugin 实际会收到的值
`values` 是一个对象，字段名来自你在 `fields` 中声明的 `key`。前端不会再附带额外包装，也不会附带“脏字段列表”，而是把当前整张表单的值一次性提交。

例如：

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

这里 plugin 实际收到的值类型规则如下：
- `string`：始终收到 JSON 字符串
- `number`：始终收到 JSON 数字；如果用户把输入框清空，则收到 `null`
- `boolean`：始终收到 JSON 布尔值 `true` 或 `false`
- `select`：始终收到 JSON 字符串，对应被选中的 `options[].value`

你不应该假设：
- `number` 一定非空
- `select` 的值一定是你当前后端逻辑里认识的枚举

plugin 应该自行做最终校验和容错。

### 插件描述返回格式
插件如果希望被前端自动渲染为通用设置表单，建议在收到 `describe` 后返回：

```json
{
  "protocol": "frontend-plugin-settings",
  "version": 1,
  "name": "示例插件",
  "description": "用于演示插件配置",
  "status": {
    "type": "ready",
    "text": "运行正常"
  },
  "fields": [
    {
      "key": "enabled",
      "label": "启用插件",
      "type": "boolean",
      "default": true
    },
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
  "values": {
    "enabled": true,
    "mode": "safe"
  },
  "submitLabel": "保存插件配置"
}
```

返回字段说明：
- `protocol`：固定为 `frontend-plugin-settings`
- `version`：固定为 `1`
- `name`：卡片标题，不传则前端退回使用插件名
- `description`：卡片描述，可为空字符串
- `status`：当前插件状态
- `fields`：表单字段定义数组
- `values`：当前字段值对象
- `submitLabel`：保存按钮文本，不传则前端使用默认值

### 字段定义规则
每个字段至少需要：

```json
{
  "key": "enabled",
  "label": "启用插件",
  "type": "boolean"
}
```

支持的字段属性如下：
- `key`：字段唯一标识，保存时会作为 `values` 的键名
- `label`：前端显示名称
- `type`：字段类型
- `description`：字段说明，可选
- `placeholder`：输入框占位文本，仅 `string` 或 `number` 有意义
- `input`：输入框类型，仅 `string` 有意义，当前支持 `text`、`password`、`url`
- `options`：选项数组，仅 `select` 必填
- `default`：默认值，可选

### 各字段类型在前端的表现和取值
`string`
- 前端渲染为单行输入框
- `input` 不传时按普通文本框处理
- `input: "password"` 时按密码框处理
- `input: "url"` 时按 URL 输入框处理，仅表示前端输入校验方式，不代表 plugin 会作为独立服务运行
- 保存时收到 JSON 字符串

`number`
- 前端渲染为数字输入框
- 用户输入有效数字时，保存时收到 JSON 数字
- 用户清空输入框时，保存时收到 `null`
- `default` 应该写成数字或 `null`

`boolean`
- 前端渲染为复选框
- 保存时始终收到 `true` 或 `false`
- `default` 应该写成布尔值

`select`
- 前端渲染为下拉选择框
- 必须提供 `options`
- `options` 结构固定为 `{ "label": "展示文本", "value": "真实值" }`
- 保存时收到当前选中的 `value` 字符串
- `default` 应该写成某个 `options[].value`

### `values` 返回规则
`values` 用于告诉前端当前这份配置的现值。它不需要包含额外元信息，只需要是一个普通对象。

例如：

```json
{
  "values": {
    "enabled": true,
    "retryCount": 3,
    "mode": "safe"
  }
}
```

前端会按字段类型做如下处理：
- 某个字段在 `values` 中缺失时，前端会回退到 `default`
- `string` 缺失时最终显示为空字符串
- `number` 缺失时最终显示为空
- `boolean` 缺失时最终显示为 `false` 或 `default`
- `select` 缺失时最终回退到 `default`，再不行就取第一个选项

### `status` 返回规则
推荐返回：

```json
{
  "status": {
    "type": "ready",
    "text": "运行正常"
  }
}
```

状态类型约定：
- `ready`
- `info`
- `warning`
- `error`

`text` 会直接显示在卡片状态区域，所以应该写成给用户看的短句，而不是调试日志。

### plugin 开发建议
- `describe` 应该是幂等的，随时可以调用
- `save` 最好只负责校验和落库，成功后把最新状态留给下一次 `describe`
- 如果 plugin 想隐藏在前端通用设置之外，直接对 `describe` 不回信即可
- 如果 plugin 有自定义复杂 UI，可以继续返回自定义内容；当前前端只会把它标记为“未接入前端通用协议”

如果插件返回了内容，但没有遵循上述协议，前端仍会保留该插件卡片，只是不再自动生成配置表单，而是提示该插件未接入前端通用协议。
