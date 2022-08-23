cmake_minimum_required(VERSION 2.8)

project(GatewayServer)

#link_libraries(profiler)
set(CMAKE_BUILD_TYPE "Debug")

gen_server("Base" "GatewayServer")

target_link_libraries(GatewayServer mysqlclient dl -static-gcc -static-libstdc++)
