[English](README_en.md)

参见根目录文档： [中文](../README.md) / [English](../README_en.md)

## 作用
这个文件夹是全部的BrowseManager部分的代码
## 路由
| 路径                   | 返回Json                                            | Body参数                                   | 备注                            |
|----------------------|---------------------------------------------------|------------------------------------------|-------------------------------|
| /                    | 参见项目README                                        | 参见项目README                               | 一般工作                          |
| /test                | ok: 正常与否                                          | 参见项目README                               | 测试服务是否正常可用                    |
| /other/closeWorker   | ok: 正常与否                                          | 参见项目README                               | 关闭对应handler                   |
| /other/testContext   | ok: 正常与否                                          | 参见项目README                               | 测试handler对应COOKIE，现与和`/`完全一致  |
| /screen              | （设置`X-Accel-Redirect`头）<br/>url: 用于nginx访问具体的登录页面 | （Url参数）<br/>token,session: 均为`/login`返回的 | 专门给nginx用的，会返回对应端口，nginx去访问即可 |
| /other/login/backend | ok: 正常与否，context: 浏览器访问时需要的账号信息                   | token: `/login`返回的                       | backend获取客户端登录结果（会直接关闭页面）     |
| /other/login         | ok: 正常与否                                          | 参见项目README                               | 为客户端打开登录链接                    |
## 环境变量
| 变量名            | 作用                    | 默认值                 | 备注     |
|----------------|-----------------------|---------------------|--------|
| PORT           | 主程序监听的端口              | 3000                |        |
| PLUGIN_PATH    | 插件配置文件地址              | config/plugins.json | 不建议更改  |
| MAX_LOGIN_PORT | 登录窗口最大数量              | 10                  |        |
| MAX_LOGIN_PORT | 登录窗口最大数量              | 10                  |        |
| LOGIN_SECONDS  | 客户端登录时，后端空置过期时长       | 60秒                 |        |
| COOKIE         | 测试代码使用的cookie         |                     | 测试代码使用 |
| USER_AGENT     | 测试代码使用的ua             |                     | 测试代码使用 |
| WAITING_TIME   | 配置当客户端登录时相关各个服务启动等待时间 | 5000毫秒              |        |              
## 插件
### 安装
安装插件只需要在配置文件（`PLUGIN_PATH`对应的 JSON 文件，默认`config/plugins.json`）里写入插件名称就好了，配置文件格式如下：
```json
{
  "plugins": [
    "写插件名就好了",
    "也可以写相对路径，这都会被转换为相对于配置Json所在文件夹的路径"
  ]
}
```
### 开发
当前 browser 侧已经支持一套基础插件机制，插件可以做两类事情：
- 注册新的 `Worker`，让后端请求中的 `workers` 数组支持新的 `type`
- 在服务启动时给 `fastify` 增加自己的路由、日志或其他初始化逻辑

### 当前接口
当前插件入口代码在 `src/PluginAPI.ts`，对插件开发最重要的是下面几个接口：

- `registerWorker(name, workerFactory)`：注册一个新的 worker 类型(对应backend中的BrowseAction类)
- `Worker`：所有自定义 worker 的基类
- `ServerPlugin`：插件默认导出的对象类型
- `onServerInit(context)`：服务启动时调用
- `onServerClose(context)`：服务关闭时调用

插件只要在启动时调用 `registerWorker(...)`，主程序后续就能通过请求里的 `type` 找到你写的 worker，从而可以执行你自己的爬取策略。

### 打包
插件需要打包为Node JS包，这样主程序才能加载

- 先把插件编译成 JavaScript
- 再让主项目能 `require("你的插件名")`

当前实现只支持写插件名字符串，还**不支持**在这个配置文件里给每个插件单独传参数。

### 最低要求
一个可用插件需要默认导出一个对象，这个对象至少要有 `onServerInit`，如：
```ts
import { ServerPlugin } from "../../src/PluginAPI";

const plugin: ServerPlugin = {
  name: "my-browser-plugin",
  async onServerInit(context) {
    context.logger.info("插件启动成功");
  }
};

export default plugin;
```

- `name` 是插件名字，不写也能运行，但建议写
- `onServerInit` 会在服务启动时执行一次
- `context.logger` 是 fastify 的日志对象，可以直接打日志

### 新的 Worker
注册Worker的过程就是调用`registerWorker`方法，注册成功后，backend发送工作请求时`type`和`info`对应你新注册的Worker就可以执行对应逻辑了

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
        "prefix": "页面标题："
      }
    }
  ]
}
```

主程序就会自动执行你注册的 worker。

### 当前版本的能力边界
下面这些是**现在已经支持**的：

- 插件启动时初始化
- 插件关闭时清理资源
- 注册自定义 worker
- 注册自定义 HTTP 路由
## 警告
本部分除开远程桌面部分（这部分安全也有赖于nginx辅助）没有任何加密措施，数据均明文传输，要么部署时与其余后端部分放在一个compose或者局域网内，要么就请自行做好安全防护！使用者也请明确你的登录地址是安全的！

如果出于不管何种原因，必须分开，请一定保证此部分的容器上层由Nginx代理，这部分涉及均基于此，并且可固定一个访问接口，便于访问
