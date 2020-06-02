cmake_minimum_required(VERSION 2.8)

project(proxy-server)

option(TARS_MYSQL "option for mysql" ON)
option(TARS_SSL "option for ssl" OFF)
option(TARS_HTTP2 "option for http2" OFF)
option(TARS_GPREF "option for gpref" OFF)

if(WIN32)
    include (c:\\tars\\cpp\\makefile\\tars-tools.cmake)
else()
    include (/usr/local/tars/cpp/makefile/tars-tools.cmake)
endif()

set(TARS_WEB_SERVER "root@172.16.8.227")
set(TARS_WEB_HOST "172.16.8.227:3000")

include_directories(/usr/local/tars/cpp/thirdparty/include)
link_directories(/usr/local/tars/cpp/thirdparty/lib)

add_subdirectory(src)
add_subdirectory(test)

set(WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set(SOURCE_DIR ${CMAKE_SOURCE_DIR})

if(WIN32)

    add_custom_target(run-http
            WORKING_DIRECTORY ${WORKING_DIRECTORY}
            USES_TERMINAL
            COMMAND ../scripts/run-http.bat  ${SOURCE_DIR}
            COMMENT "call run http")

    add_custom_target(run-kill
            WORKING_DIRECTORY ${WORKING_DIRECTORY}
            USES_TERMINAL
            COMMAND ../scripts/run-kill.bat  ${SOURCE_DIR}
            COMMENT "call run kill")
else(WIN32)
    add_custom_target(run-server
            WORKING_DIRECTORY ${WORKING_DIRECTORY}
            USES_TERMINAL
            COMMAND ../scripts/run-server.sh  ${SOURCE_DIR}
            COMMENT "call run http server")

    add_custom_target(run-http
            WORKING_DIRECTORY ${WORKING_DIRECTORY}
            COMMAND ../scripts/run-http.sh  ${SOURCE_DIR}
            COMMENT "call run http")

    add_custom_target(run-client
            WORKING_DIRECTORY ${WORKING_DIRECTORY}
            COMMAND ../scripts/run-client.sh  ${SOURCE_DIR}
            COMMENT "call run client")

    add_custom_target(run-kill
            WORKING_DIRECTORY ${WORKING_DIRECTORY}
            USES_TERMINAL
            COMMAND ../scripts/run-kill.sh  ${SOURCE_DIR}
            COMMENT "call run kill")
endif(WIN32)