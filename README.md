[English](README_en.md)

## 项目架构
1. 后端[backend](https://github.com/passplease-organization/Bilibili-Reranger/tree/backend)(还包含`browser`)
2. 前端[frontend](https://github.com/passplease-organization/Bilibili-Reranger/tree/frontend)

## 部署
拉取主分支
```shell
git clone -b master https://github.com/passplease-organization/Bilibili-Reranger
```
基于`example.env`创建`.env`并配置环境变量（其实可以不改），最后启动即可
```shell
docker compose up -d
```