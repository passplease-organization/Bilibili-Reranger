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
后端现已支持 `plugin` 处理请求，前端设置页会为可接收请求的插件渲染独立配置卡片。考虑到插件可能由 C++ 程序实现，这里的协议刻意保持简单，只要求插件返回静态 JSON 描述，前端负责渲染和状态展示。

### `/plugins`
- 无参数时：返回全部插件及其可配置描述
- 有参数时：处理指定插件的配置提交

请求体约定：
- `plugin`：插件名称
- `data`：发给插件的完整 JSON 配置对象

获取插件列表时，建议返回：

```json
{
  "ok": true,
  "plugins": [
    {
      "plugin": "example",
      "name": "示例插件",
      "description": "用于演示插件配置",
      "configurable": true,
      "status": "ready",
      "statusText": "运行正常",
      "fields": [
        {
          "key": "enabled",
          "label": "启用",
          "type": "bool",
          "value": true
        },
        {
          "key": "apiUrl",
          "label": "接口地址",
          "type": "string",
          "input": "url",
          "value": "http://127.0.0.1:8080"
        },
        {
          "key": "mode",
          "label": "模式",
          "type": "select",
          "value": "safe",
          "options": [
            { "label": "安全", "value": "safe" },
            { "label": "激进", "value": "aggressive" }
          ]
        }
      ]
    }
  ]
}
```

字段说明：
- `plugin`：插件唯一标识
- `name`：前端展示名称
- `description`：插件说明
- `configurable`：是否允许前端渲染配置卡片
- `status`：插件当前状态
- `statusText`：状态的文本说明
- `fields`：插件声明的配置项列表

前端建议支持的字段类型：
- `bool`
- `string`
- `number`
- `select`

其中：
- `bool` 用于开关
- `string` 用于普通文本输入
- `number` 用于数字输入
- `select` 用于枚举选项，需额外提供 `options`

提交插件配置时，请求示例：

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

成功返回示例：

```json
{
  "ok": true,
  "plugin": "example",
  "status": "ready",
  "statusText": "保存成功",
  "errors": []
}
```

失败返回示例：

```json
{
  "ok": false,
  "plugin": "example",
  "status": "error",
  "statusText": "配置校验失败",
  "errors": [
    {
      "key": "apiUrl",
      "message": "地址不能为空"
    }
  ]
}
```

推荐统一使用以下状态值，便于前端直接判断插件状态：
- `ready`
- `unconfigured`
- `disabled`
- `error`
