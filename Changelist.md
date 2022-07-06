
## v1.3.1 (20220706)

### en

- Fix the web platform k8s parameter error
- Fix GatewayServer _terminate not init bug
### cn

- 修复web平台k8s参数错误
- 修复GatewayServer _terminate 未初始化的bug

## v1.3.0 (20220207)

### en

- use base-compiler-stretch
- support tarscloud market
- Gateway web platform independence

### cn

- actions 使用 stretch 来编译
- 支持 tarscloud 服务市场
- 网关web平台独立 

## v1.2.1

### en

- option && monitor request http response header can be configured
- fix values ProxyObj isTars: false
- fix the bug that the HTTP header does not return under abnormal conditions in the cross domain request mode
- Fix the protocol parsing bug when the authentication service and business service are in the same obj mode

### cn

- 支持增加 http 响应头的配置化(主要为了支持跨域请求)
- 修复 value.yaml 中的 ProxyObj 配置的错误
- 修复跨域请求模式下, 在异常情况下, http 头没有返回的 bug
- 修复鉴权服务和业务服务是同一个 obj 的模式下, 协议解析的 bug

## v1.2.0

### en

- Increase authentication capability, The gateway authenticates the service interface according to the configuration, so that each interface does not need to authenticate itself

### cn

- 增加鉴权能力, 网关根据配置去业务的接口鉴权, 这样不需要业务自己每个接口都自己鉴权

## v1.1.0

### en

- Update readme
- support call train
- Fix the file descriptor leak when the connection fails
- remove default path for tup request
- fix bug for: variable "terminate" random value, causing threads to occasionally not run

### cn

- 更新 readme
- 支持调用链
- 修复句柄泄露的 bug
- 修复变量未初始化导致线程未启动的 bug

## v1.0.4

### en

- Update readme
- conf id set as 0
- Fix tars call exception log

### cn

- 更新 readme
- 修复 db 配置中 conf id 为 0 时 bug
- 优化 tars 调用的异常日志

## v1.0.3

### en

- Update readme
- Fix compiler bug, add dl linker
- Fix the situation where http header is equal to ‘Expect: 100-continue’
- Optimize one click installation scripts
- test code add jsonCall

### cn

- 更新 readme
- 修复编译 bug
- 修复 http header 的 bug, 当 http header 中有‘Expect: 100-continue’
- 优化一键安装脚本
- 测试代码总增加 jsoncall

## v1.0.2

### en

- modify monitor.jsp to monitor.html
- add install tools

### cn

- 修改监控 url
- 添加一键安装脚本

## v1.0.1

### en

- delete unuseful files
- add json http response http header
- fix reginit bug;

### cn

- 删除不要的文件
- 添加 http+json 模式下响应头
- 修复 reginit 的 bug

## v1.0.0

### en

- first version

### cn

- 发布第一个版本
