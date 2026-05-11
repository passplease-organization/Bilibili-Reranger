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