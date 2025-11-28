English Version: [README_en.md](README_en.md)

# 服务后端

## 架构
这是整个项目的后端代码，架构基于C++实现，选择原因无他，只是因为我想学习一下C++怎么编程。第三方库：
1. [nlohmann-json](https://github.com/nlohmann/json)处理各种Json文本
2. [cpr](https://github.com/libcpr/cpr)进行B站API的请求和测试代码
3. [CURL](https://github.com/curl/curl)进行B站API的请求
4. [boost](https://github.com/boostorg/boost)持续监听端口和处理网络请求

功能实现主要依靠插件，主程序只是一个框架，只是处理各种琐事，具体视频的去留都是插件决定。
## 使用
强烈建议使用docker部署，使用打包镜像直接部署，使用下述命令
```bash
docker run -p 23223:23223 -e COOKIE=<your_bilibili_cookie> -e USERAGENT=<browser_user_agent> -v <path_to_config>:/bilibili-backend/config -v <path_to_plugin>:/bilibili-backend/plugins docker.io/noname602/bilibili_reranger:latest
```
`COOKIE`和`USERAGENT`目前是必须的，`config`文件夹储存的是配置文件，`plugins`文件夹是存储的加载的插件，默认有一个`EXAMPLE PLUGIN`插件
## 插件
代码中有[实例插件](plugins/ExamplePlugin/main.cpp)，提供接口在[API动态链接库](api/interface.h)中，[API动态链接库](api)中的以C程序导出的方法都是可用的，方便做插件。