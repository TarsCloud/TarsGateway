cmake_minimum_required(VERSION 2.8)

project(Base-VerifyDemoServer)

option(TAF_MYSQL "option for mysql" ON)
option(TAF_SSL "option for ssl" OFF)
option(TAF_GMSSL "option for ssl" OFF)
option(TAF_HTTP2 "option for http2" OFF)

if(WIN32)
include (c:\\tars\\cpp\\makefile\\tars-tools.cmake)
else()
include (/usr/local/tars/cpp/makefile/tars-tools.cmake)
endif()


set(CMAKE_BUILD_TYPE "Debug")
# set(CMAKE_BUILD_TYPE "Release")

set(TAF_WEB_HOST "" CACHE STRING "set web host")
IF (TAF_WEB_HOST STREQUAL "")
	set(TAF_WEB_HOST "http://tars.test.com")
ENDIF ()

include_directories(/usr/local/tars/cpp/thirdparty/include)
link_directories(/usr/local/tars/cpp/thirdparty/lib)

add_subdirectory(src)

