#!/bin/bash

echo "run-http.sh"

SRC_PATH=$1

echo ${EXE_PATH} ${SRC_PATH}

EXE_PATH=bin

killall -9 WupProxyServer
killall -9 HelloServer
killall -9 HttpServer

sleep 1

echo "start server: ${EXE_PATH}/WupProxyServer --config=${SRC_PATH}/conf/config.conf --wup=${SRC_PATH}/conf/WupProxyServer.conf &"
${EXE_PATH}/WupProxyServer --config=${SRC_PATH}/conf/config.conf --wup=${SRC_PATH}/conf/WupProxyServer.conf &
sleep 1

echo "start server: ${EXE_PATH}/HelloServer --config=${SRC_PATH}/test/HelloServer/config.conf &"
${EXE_PATH}/HelloServer --config=${SRC_PATH}/test/HelloServer/config.conf &
sleep 1

echo "start server: ${EXE_PATH}/HttpServer --config=${SRC_PATH}/test/HttpServer/config.conf &"
${EXE_PATH}/HttpServer --config=${SRC_PATH}/test/HttpServer/config.conf &


