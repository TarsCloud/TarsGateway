#!/bin/bash

echo "run-http.sh"

SRC_PATH=$1

echo ${EXE_PATH} ${SRC_PATH}

EXE_PATH=bin

killall -9 HttpServer

echo "start server: ${EXE_PATH}/HttpServer --config=${SRC_PATH}/test/HttpServer/config.conf &"
${EXE_PATH}/HttpServer --config=${SRC_PATH}/test/HttpServer/config.conf &


