cmake_minimum_required(VERSION 2.8)

project(WupProxyServer)

link_libraries(profiler)

gen_server("Base" "HttpClient")

