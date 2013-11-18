project(audiodecoder.asap)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(xbmc REQUIRED)

include(ExternalProject)

include_directories(${XBMC_INCLUDE_DIR}
                    ${PROJECT_SOURCE_DIR}/lib/asap-3.1.5)

set(ASAP_SOURCES src/ASAPCodec.cpp
                 lib/asap-3.1.5/asap.c)

set(DEPLIBS ${PROJECT_SOURCE_DIR}/lib/asap-3.1.5/libasap.a)

build_addon(audiodecoder.asap ASAP DEPLIBS)
add_dependencies(audiodecoder.asap asap)

include(CPack)
