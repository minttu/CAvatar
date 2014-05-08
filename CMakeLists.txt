cmake_minimum_required(VERSION 2.6)
project(cavatar)

set(CMAKE_C_COMPILER, clang)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(LibEvent REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(Libevhtp REQUIRED)
find_package(PNG 1.6 REQUIRED)

add_definitions(-D_GNU_SOURCE -std=c99 -O2 -Wall)

add_executable(cavatar cavatar.c)
target_link_libraries(cavatar ${EVENT_LIBRARIES})
target_link_libraries(cavatar ${OPENSSL_LIBRARIES})
target_link_libraries(cavatar ${EVHTP_LIBRARIES})
target_link_libraries(cavatar pthread)
target_link_libraries(cavatar event_openssl)

add_library(util util.c)
add_library(helper_routes helper_routes.c)
add_library(gen gen.c)
target_link_libraries(gen ${PNG_LIBRARIES})
target_link_libraries(gen ${EVHTP_LIBRARIES})

target_link_libraries(cavatar ${CORELIBS} util helper_routes gen)