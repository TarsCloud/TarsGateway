- [English](Gateway.en.md)

## 简介

TarsGateway 系统是基于 tars 框架开发的一套通用 api 网关, 它有两个服务组成:

- Base/GatewayServer: 实际的网管服务, 具体[请参考说明文档](./README.md)
- Base/GatewayWebServer: 网关对应管理平台, 它属于 TarsWeb 的扩展服务, 注意 >= TarsFramework:v3.0.10 & TarsWeb:v3.0.2 才可以使用

## 支持说明

在 < TarsFramework:v3.0.9, < TarsWeb:v3.0.1 之前, 网关管理平台被内置在 TarsWeb 中, 之后版本为了提供 TarsWeb 的扩展性, TarsWeb 支持了服务插件化, 即你可以实现独立的 web 服务和 TarsWeb 整合到一起, 从而当各个子模块升级时, 无须升级 TarsWeb, 具体方式请参考 TarsWeb 相关的文档.
