cmake_minimum_required(VERSION 2.8)

project(GatewayServer)

#link_libraries(profiler)
set(CMAKE_BUILD_TYPE "Debug")

# set(CMAKE_EXE_LINKER_FLAGS "-static")
# set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")

gen_server("Base" "GatewayServer")

target_link_libraries(GatewayServer mysqlclient dl)
