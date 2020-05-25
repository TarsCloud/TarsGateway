#!/bin/bash

echo "run-http.sh"

SRC_PATH=$1

echo ${EXE_PATH} ${SRC_PATH}

EXE_PATH=bin

killall -9 WupProxyServer
killall -9 HelloServer
killall -9 HttpServer
killall -9 HttpClient



