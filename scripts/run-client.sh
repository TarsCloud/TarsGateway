#!/bin/bash

#env

echo "run-client.sh"

EXE_PATH=bin

echo "client: ${EXE_PATH}/HttpClient"

#${EXE_PATH}/HttpClient --count=10000 --thread=2 --call=wupsync
#${EXE_PATH}/HttpClient --count=30000 --thread=3 --call=wupasync --flow=10000
${EXE_PATH}/HttpClient --count=30000 --thread=3 --call=httpasync --flow=10000
#${EXE_PATH}/HttpClient --count=100000 --thread=5 --call=wupasync --flow=1000
#${EXE_PATH}/HttpClient --count=10000 --thread=2 --call=synchttp

sleep 1



