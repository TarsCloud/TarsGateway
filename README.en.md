- [中文文档](Gateway.md)

## Introduction

TarsGateway system is a set of general API gateway developed based on tar framework. It consists of two services:

- Base/GatewayServer: Actual gateway service.
- Base/GatewayWebServer: The gateway corresponds to the management platform, which belongs to the extended service of tarsweb. Note >= TarsFramework:v3.1.0 & TarsWeb:v3.1.0 can be used

## Supported

Before < tarsframework:v3.1.0 and < tarsweb:v3.1.0, the gateway management platform was built into tarsweb. In later versions, in order to provide the extensibility of tarsweb, tarsweb supports service plug-in, that is, you can realize the integration of independent web services and tarsweb. Therefore, when each sub module is upgraded, there is no need to upgrade tarsweb. For specific methods, please refer to the relevant tarsweb documents

## Mysql Configuration description

When installing the gateway system, you need to rely on MySQL. Therefore, pay attention to configuring the dependent MySQL address during installation

- Gatewayserver, please modify `GatewayServer conf`
- Gatewaywebserver, please modify `config json`

Please use the same database. Note: database `db_base` and related tables will be automatically created by the gatewaywebserver

## Gateway function description

Functions and configuration of the actual gateway service (gatewayserver), [please refer to the documentation](./Gateway.en.md)
