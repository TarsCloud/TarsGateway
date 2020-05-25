cmake_minimum_required(VERSION 2.8)

project(GetwayServer)

#link_libraries(profiler)

gen_server("Base" "GetwayServer")

target_link_libraries(GetwayServer mysqlclient)
