cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(kocka)

set(PUBLIC_DIR ${CMAKE_SOURCE_DIR}/public/)

add_subdirectory(httpsrv)
add_subdirectory(gameclient)