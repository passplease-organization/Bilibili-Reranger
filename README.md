是[English](README_en.md)

BrowserManager文档： [中文](browser/README.md) / [English](browser/README_en.md)

# 服务后端

## 架构
### 整体架构
这是整个项目的后端代码，架构基于C++实现，选择原因无他，只是因为我想学习一下C++怎么编程。第三方库：
1. [nlohmann-json](https://github.com/nlohmann/json)处理Json序列化/反序列化
2. [cpr](https://github.com/libcpr/cpr)进行B站API的请求与AI调用
3. [CURL](https://github.com/curl/curl)负责底层HTTP请求支持
4. [Boost](https://github.com/boostorg/boost)实现端口监听与HTTP处理（asio/beast/url）
5. [libsodium](https://github.com/jedisct1/libsodium)完成登录与加密相关功能
6. [OpenSSL](https://github.com/openssl/openssl)处理RSA公钥加密与密钥交换
7. [libpqxx](https://github.com/jtv/libpqxx)处理PostgreSQL访问（用于客户端数据与Cookie持久化）

功能实现主要依靠插件，主程序只是一个框架，只是处理各种琐事，具体视频的去留都是插件决定。而具体的爬取操作也不是C++代码进行的，而是**BrowserManager**所在的另一个容器进行的，C++代码主要负责与**BrowserManager**的交互。
> 在2026年1月28日前基于B站逆向完成，后来[参考文献](https://github.com/SocialSisterYi/bilibili-API-collect)吃律师函了，并且考虑到兼容其他平台，最终更改使用此架构

### **BrowserManager**部分架构
#### 后端发送工作内容
此时每一个客户端的Handler都对应一个后端浏览器，对应关系会保存到数据库中，数据也据此长期储存。不过写新handler只需要提交`workers`就可以了，其他已包装在`api/webAPIs/browse.cpp`中。
通信Json格式：
```json5
{
  "clientID": "客户端的ID",
  "platform": "handler支持的平台",
  "context": {
    // 浏览器需要的基础信息
  },
  "workers": [// 有时忽略后面的维护参数
    // 每一个worker对应一个
    {
      "type": "工作类型名字",
      "info": {
        // worker内部自定json
      }
    }
  ],
  "mode": "closeWorker" // 可无，没有时以请求路径为准
}
```
#### 浏览器返回工作结果
```json5
{
  "ok": true,// 永远有的，表示浏览器执行有没有成功
  "error": {// 如果报错就可能有
    "name": "",
    "message": ""
  },
  "back": [// 永远有，只不过有一些请求会是空的
    {
      // 每一个worker工作的结果，如果每一步结果过多，会是数组
    },
    [
      // worker结果过多的情况
    ],
    [
      // DoWhile节点特殊，内部区别循环结果都包在这里面
      [
        // 每一个都是一次循环的内容，结构和最外层一致
      ]
    ]
  ]
}
```
## 使用
强烈建议使用docker部署，使用打包镜像直接部署，使用下述命令
```bash
docker run -p 23223:23223 -e BROWSE_URL=<your_browse_manager_url> -v <path_to_config>:/bilibili-backend/config -v <path_to_plugin>:/bilibili-backend/plugins docker.io/noname602/bilibili_reranger:latest
```
`BROWSE_URL`是必须的，代表程序的浏览器组地址（使用docker-compose部署默认地址内部地址为`http://browser:3000`），`config`文件夹储存的是配置文件，`plugins`文件夹是存储的加载的插件，默认有一个`EXAMPLE PLUGIN`插件
## 插件
代码中有[示例插件](plugins/ExamplePlugin/main.cpp)，提供接口在[API动态链接库](api/interface.h)中，[API动态链接库](api)中的以C程序导出的方法都是可用的，方便做插件。
## URL请求
### 相关Bug
最重要bug：程序使用的[Boost](https://github.com/boostorg/boost)库进行`url`参数解析，在我测试时发现，有时其`url`中的参数解析会出现错误，导致解析完全错误得不到参数。且在IDE或者命令行和docker等不同方法启动程序表现还不同，故有时请求错误可能是后端解析错误所致。为避免此bug，计划将受影响的部分参数从`url`中移动至`body`中。
### 支持请求
全部支持的URL请求：

| URL路径         | 意义                                 | URL参数（除`id`默认必要外，未写参数即不支持）                                    | `Body`参数（一般加密）                                                                     | 其他                                | 状态 |
|---------------|------------------------------------|---------------------------------------------------------------|------------------------------------------------------------------------------------|-----------------------------------|----|
| /all_category | 获取注册了的全部类型                         | 无额外参数                                                         |                                                                                    | 需要在设置平台后调用，否则就会返回`null`           |    |
| /login        | 登录要爬的平台（获取COOKIE等）                 | 无`id`以外参数代表获取全部支持平台<br/>`test`：此项覆盖所有其他参数，用于测试是否需要登录，`true`启用 | `platform`：要设置的平台（每次必须，没有就是获取全部平台）<br/>`screen`：屏幕的大小（详细可查看`browser/src/login.ts`） | 根据支持的平台不同和插件不同可以有变化，多余参数可以由插件自行获取 |    |
| /key          | 和后端交换加密秘钥                          | 无任何参数                                                         | `key`：前端对称加密秘钥（成功会返回加密的`id`参数）<br/>什么都不传表示获取非对称加密公钥<br/>`admin`：管理员id              | 初次建立连接使用非对称加密，建立后使用对称加密方法通信       |    |
| /test         | 前端检测各种信息                           | `id`（必要）                                                      | `key`：对称加密后的秘钥，用于客户端重新登录后的检测密钥正确性                                                  | 返回500表示失败，返回200为成功                |    |
| /init         | 初始化爬虫，若有登录`token`会首先收集登录信息并存储在数据库中 | `token`：`/login`返回url中的`token`参数（无则不执行收集登录信息相关逻辑）             | 无额外参数                                                                              | 需在login之后调用                       |    |
| /set          | 设置参数                               | `platform`：设置工作平台                                             | 无支持参数                                                                              |                                   |    |

此外，返回值为500表示内部处理错误，具体信息请检查正文内容。且Body部分是整体进行加密（对整个Json进行），而非对Json各个部分分开加密。

爬取参数（对于其他URL路径）：

| 参数名        | 意义            | 其他                                                |
|------------|---------------|---------------------------------------------------|
| category   | 本次工作的类型       | 需配合`/set`使用，只会针对当前平台工作                            |
| id         | 后端用于区分客户端的值   | 除/key请求，其他默认均需要携带<br/>加密是对整个正文进行的，而非json中各个单独字段加密 |

### 请求流程
首先应连接`/key`先获取公钥，再交换对称密钥并储存`id`，随后连接`/login`获取开启登录浏览器，然后`/init`存储登录状态并初始化后端，然后就可以使用`category`获取不同类别视频了

## Admin
管理员可通过修改配置文件中的`admin_client_key`字段，自行配置管理员的秘钥，当前管理员有如下好处

| 类别   | 解释                                                                           | 如何设置         | 其他 |
|------|------------------------------------------------------------------------------|--------------|----|
| 数据同步 | 管理员验证后可共用同一个`Client ID`，故而在任何界面看到的都相同，不必反复登录。<br/>且链接永不超时，即使很长时间后其`ID`都不会被清除 | 通过`/key`请求设置 |
