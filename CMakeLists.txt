cmake_minimum_required(VERSION 2.6)
project(cavatar)

set(CMAKE_C_COMPILER, clang)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(LibEvent)
find_package(OpenSSL)

add_definitions(-D_GNU_SOURCE -std=c99 -O2 -Wall)

add_executable(cavatar cavatar.c)
target_link_libraries(cavatar ${LIBEVENT_LIB})
target_link_libraries(cavatar crypto)
target_link_libraries(cavatar ssl)
target_link_libraries(cavatar evhtp)
target_link_libraries(cavatar pthread)
target_link_libraries(cavatar event_openssl)

add_library(util util.c)
add_library(helper_routes helper_routes.c)
add_library(gen gen.c)
target_link_libraries(gen png)
target_link_libraries(gen evhtp)
target_link_libraries(cavatar ${CORELIBS} util helper_routes gen)