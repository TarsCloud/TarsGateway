-- [Back](./Gateway.en.md)

# Installation

## Support one-click installation (tarscpp compilation environment is required, version>=v2.4.4):

```
    git clone https://github.com/TarsCloud/TarsGateway.git
    cd TarsGateway/install;

    ./install.sh tarsweb_base token node_ip gateway_db_ip gateway_db_port gateway_db_user gateway_db_pwd

```

## The installation parameters are as follows:

- tarsweb_base The base address of TarsWeb management end, for example: http://172.16.8.227:3000 (Be careful not to add / in the end).
- token TarsWeb management end’s token, which can be obtained through the management end http://${webhost}/auth.html#/token
- Node_ip The IP deployed by GatewayServer, and currently only one is supported here. If you need more, you can expand on the platform later.
- gateway_db_ip the database server ip where gateway db is located.
- gateway_db_port gateway db port.
- gateway_db_user gateway db user name ( the permission to build database and table required).
- gateway_db_pwd gateway db password.

Note:

- Gateway depends on db and its SQL is placed in install/db_base.sql. During installation, the db will be created, and make sure that the web platform can access your gateway DB.
- When using a script to deploy with one click, only one node is installed by default, but you can expand and deploy on the web platform if you need to.

## For example:

```
    ./install.sh http://172.16.8.220:3000 036105e1ebfc13843b4db0edcd000b3d9f47b13928423f0443df54d20ca65855 172.16.8.220 172.16.8.221 3306 tars tars2015
```

## Verify the installation result:

Open http://${server_ip}:8200/monitor/monitor.html in the browser. If you can display hello TupMonitorxxx normally, the installation is successful.
