## 支持一键安装（需要先具备tarscpp编译环境）：
```
    git clone https://github.com/TarsCloud/TarsGateway.git
    cd TarsGateway/install;
    ./install.sh tarsweb_base token server_ip tars_db_ip tars_db_port tars_db_user tars_db_pwd

```
## 安装参数如下：
* tarsweb_base              TarsWeb管理端的基础地址，例如：http://172.16.8.227:3000 （注意后面不要 /）
* token                     TarsWeb管理端的token，可以在tarsweb右上角点击用户中心-Token管理 获取
* server_ip                GatewayServer部署的ip，目前这里只支持一个，如果需要更多，后面直接在平台上面扩容即可。
* tars_db_ip               tarsdb 所在的数据库服务器ip。
* tars_db_port             tarsdb 端口。
* tars_db_user               tarsdb 用户名（需要有建库建表权限）。
* tars_db_pwd               tarsdb 密码。

## 例如：
```
    ./install.sh 172.16.8.220:3000 036105e1ebfc13843b4db0edcd000b3d9f47b13928423f0443df54d20ca65855 172.16.8.220 172.16.8.221 3306 tars tars2015
```
## 验证安装结果：
在浏览器打开 http://${server_ip}:8200/monitor/monitor.html , 如果能正常显示 hello TupMonitorxxx 就表示安装成功。