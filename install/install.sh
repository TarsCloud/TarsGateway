#!/bin/bash

TARS_WEB_HOST=$1
TARS_WEB_TOKEN=$2
GATEWAYSERVER_IP=$3
TARS_DB_HOST=$4

WORKDIR=$(cd $(dirname $0); cd ..; pwd)

if [ $# -eq 4 ]; then
    TARS_CPP_PATH="/usr/local/tars/cpp"
elif [ $# -eq 5 ]; then
    TARS_CPP_PATH=$5
else
    echo "Usage:";
    echo "  $0 webhost tarstoken serverip tarsdbip";
    echo "  $0 webhost tarstoken serverip tarsdbip tarscpp";
    echo "Description:";
    echo "  webhost: tars web admin host";
    echo "  tarstoken: can fetch from http://webhost:3001/auth.html#/token";
    echo "  serverip: ip address of GatewayServer will be installed";
    echo "  tarsdbip: ip address of tars-framework's db";
    echo "  tarscpp: the path which tarscpp has installed";
    exit 1
fi

### check tarscpp environment
if [ ! -d ${TARS_CPP_PATH} ]; then
    echo "tarscpp not exits, please install fisrt"
    echo "Goto https://tarscloud.github.io/TarsDocs_en/env/tarscpp.html"
    exit 1
fi


### check os environment
OSNAME=`uname`
if [[ "$OSNAME" == "Windows_NT" ]]; then
    echo "TarsGateway don't support windows";
    exit 1
fi

function LOG_INFO()
{
	local msg=$(date +%Y-%m-%d" "%H:%M:%S);

	for p in $@
	do
		msg=${msg}" "${p};
	done

	echo -e "\033[32m $msg \033[0m"
}

function LOG_ERROR()
{
	local msg=$(date +%Y-%m-%d" "%H:%M:%S);

	for p in $@
	do
		msg=${msg}" "${p};
	done

	echo -e "\033[31m $msg \033[0m"
}

#输出配置信息
LOG_INFO "===>install tars.tarsgateway >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>";
LOG_INFO "PARAMS:        "$*
LOG_INFO "OS:            "$OSNAME
LOG_INFO "TarsCpp:       "$TARS_CPP_PATH
LOG_INFO "WebHost:       "$TARS_WEB_HOST
LOG_INFO "Token:         "$TARS_WEB_TOKEN
LOG_INFO "Tars-DB:       "$TARS_DB_HOST
LOG_INFO "===<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< print envirenment finish.\n";

################################################################################

function create_db()
{
    cd $WORKDIR/install
    mysql -h ${TARS_DB_HOST} -utars -ptars2015 -e "create database IF NOT EXISTS db_base"
    mysql -h ${TARS_DB_HOST} -utars -ptars2015 db_base < db_base.sql
}

function build_server()
{
    rm -rf $WORKDIR/build
    mkdir $WORKDIR/build && cd $WORKDIR/build && cmake .. && make GatewayServer && make GatewayServer-tar
}

function build_webconf()
{
    cd $WORKDIR

    LOG_INFO "===>install GatewayServer server:\n";
    sed -i "s/host_ip/$GATEWAYSERVER_IP/g" install/server.json
    curl -s -X POST -H "Content-Type: application/json" http://${TARS_WEB_HOST}/api/deploy_server?ticket=${TARS_WEB_TOKEN} -d@install/server.json|echo

    LOG_INFO "===>add GatewayServer.conf:\n";
    sed -i "s/db.tars.com/$TARS_DB_HOST/g" install/config.json
    curl -s -X POST -H "Content-Type: application/json" http://${TARS_WEB_HOST}/api/add_config_file?ticket=${TARS_WEB_TOKEN} -d@install/config.json|echo

    LOG_INFO "====> build_webconf finish!\n";
}

function upload_server()
{
    cd $WORKDIR/build

    LOG_INFO "===>upload GatewayServer server:\n"
    curl -s http://${TARS_WEB_HOST}/api/upload_and_publish?ticket=${TARS_WEB_TOKEN} -Fsuse=@GatewayServer.tgz -Fapplication=tars -Fmodule_name=GatewayServer -Fcomment=auto-upload|echo
    LOG_INFO "====> upload server finish!\n";
}

function check_result()
{
    sleep 5
    LOG_INFO "===>fetch http://${GATEWAYSERVER_IP}:8200/monitor/monitor.jsp to check the gateway in installed ok!\n" 
    result=`curl -sIL -w "%{http_code}" -o /dev/null http://${GATEWAYSERVER_IP}:8200/monitor/monitor.jsp`
    if [ $result -eq 200 ]; then
        LOG_INFO "test result is:"$result
        LOG_INFO "install success!";
    else
        LOG_ERROR "test result is:"$result
        LOG_ERROR "install fail!";
    fi
}

create_db
build_server
build_webconf
upload_server
check_result
