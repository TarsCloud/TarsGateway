- [返回](README.md)

## 安装

注意: 如果可以尽量使用市场安装, 源码安装模式不方便升级!

### 支持一键安装（需要先具备 tarscpp 编译环境, 版本>=v2.4.4）：

```
    git clone https://github.com/TarsCloud/TarsGateway.git
    cd TarsGateway/install;

    ./install.sh tarsweb_base token node_ip gateway_db_ip gateway_db_port gateway_db_user gateway_db_pwd

```

### 安装参数如下：

- tarsweb_base TarsWeb 管理端的基础地址，例如：http://172.16.8.227:3000 （注意后面不要 /）
- token TarsWeb 管理端的 token，可以通过管理端获取 http://${webhost}/auth.html#/token
- node_ip GatewayServer 部署的 ip，目前这里只支持一个，如果需要更多，后面直接在平台上面扩容即可。
- gateway_db_ip gateway db 所在的数据库服务器 ip。
- gateway_db_port gateway db 端口。
- gateway_db_user gateway db 用户名（需要有建库建表权限）。
- gateway_db_pwd gateway db 密码。

注意:

- Gateway 会依赖 db, 它的 sql 放在 install/db_base.sql, 安装时会创建该 db, 注意你也需要保证 web 平台能访问到你的网关 DB
- 用脚本一键部署时, 默认只安装了一台节点, 有需要你在 web 平台上自己扩容部署即可

### 例如：

```
    ./install.sh http://172.16.8.220:3000 036105e1ebfc13843b4db0edcd000b3d9f47b13928423f0443df54d20ca65855 172.16.8.220 172.16.8.221 3306 tars tars2015
```

### 验证安装结果：

在浏览器打开 http://${server_ip}:8200/monitor/monitor.html , 如果能正常显示 hello TupMonitorxxx 就表示安装成功。
