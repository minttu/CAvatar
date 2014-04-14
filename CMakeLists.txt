cmake_minimum_required(VERSION 2.6)
project(cavatar)

set(CMAKE_C_COMPILER, clang)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(LibEvent)
find_package(OpenSSL)
find_package(ImageMagick COMPONENTS Magick++ MagickWand)

add_definitions(-std=c99 -O2 -Wall -DMAGICKCORE_HDRI_ENABLE=1 -DMAGICKCORE_QUANTUM_DEPTH=16)

add_executable(cavatar cavatar.c)
include_directories(${ImageMagick_INCLUDE_DIRS})
target_link_libraries(cavatar ${ImageMagick_LIBRARIES})
target_link_libraries(cavatar ${LIBEVENT_LIB})
target_link_libraries(cavatar crypto)
target_link_libraries(cavatar ssl)
target_link_libraries(cavatar evhtp)
target_link_libraries(cavatar pthread)
target_link_libraries(cavatar event_openssl)

add_library(util util.c)
target_link_libraries(cavatar ${CORELIBS} util)