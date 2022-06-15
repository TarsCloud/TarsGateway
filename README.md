- [English](Gateway.en.md)

## 简介

TarsGateway 系统是基于 tars 框架开发的一套通用 api 网关, 它有两个服务组成:

- Base/GatewayServer: 实际的网关服务, 具体[请参考说明文档](./README.md)
- Base/GatewayWebServer: 网关对应管理平台, 它属于 TarsWeb 的扩展服务, 注意 >= TarsFramework:v3.1.0 & TarsWeb:v3.1.0 才可以使用

整个系统会依赖 mysql, 主要用于存储网关路由信息(http 转发), GatewayWebServer 启动时会自动创建相关表.

注意安装时, 两个服务必须安装在同一个应用名下!

## 支持说明

在< TarsWeb:v3.1.0 之前, 网关管理平台(GatewayWebServer)被内置在 TarsWeb 中, 之后版本为了提供 TarsWeb 的扩展性, TarsWeb 支持了服务插件化, 即你可以实现独立的 web 服务和 TarsWeb 整合到一起, 从而当各个子模块升级时无须升级 TarsWeb, 具体方式请参考 TarsWeb 相关的文档.

## 安装方式

推荐使用新版本 > TarsFramework:v3.1.0 时, 直接从云市场安装网关服务, 建议以容器方式启动网关, 这样不依赖操作系统 stdc++.so 的版本.

[容器方式启动业务方式请参考](https://doc.tarsyun.com/#/installation/service-docker.md)

## mysql 配置说明

在安装网关系统时, 需要依赖 mysql, 因此在安装注意配置依赖的 mysql 地址

- GatewayServer 请修改`GatewayServer.conf`
- GatewayWebServer 请修改`config.json`

数据库请使用同一个, 注意: 数据库`db_base以及相关的表`会被 GatewayWebServer 自动创建出来

## 网关功能说明

实际的网关服务的功能和配置(GatewayServer), [请参考说明文档](./Gateway.md)
