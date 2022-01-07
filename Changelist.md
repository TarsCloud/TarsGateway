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