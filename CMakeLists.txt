cmake_minimum_required(VERSION 3.5)
project(audiodecoder.asap)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

find_package(Kodi REQUIRED)

include_directories(${KODI_INCLUDE_DIR}/..
                    ${PROJECT_SOURCE_DIR}/lib/asap-3.1.5)

set(ASAP_SOURCES src/ASAPCodec.cpp
                 lib/asap-3.1.5/asap.c)

build_addon(audiodecoder.asap ASAP DEPLIBS)

include(CPack)
