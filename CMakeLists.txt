cmake_minimum_required(VERSION 3.10)
project(kocka)

set(PUBLIC_DIR ${CMAKE_SOURCE_DIR}/public/)
include_directories(PUBLIC_DIR)

add_subdirectory(httpsrv)
add_subdirectory(gameclient)