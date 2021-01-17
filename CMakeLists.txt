cmake_minimum_required(VERSION 3.5)
project(audiodecoder.asap)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

find_package(Kodi REQUIRED)

if(WIN32)
  add_definitions("/wd4996")
endif()

include_directories(${KODI_INCLUDE_DIR}/..
                    ${PROJECT_SOURCE_DIR}/lib/asap-code)

set(ASAP_SOURCES src/ASAPCodec.c
                 lib/asap-code/asap.c)

set(ASAP_HEADERS )

add_executable(audiodecoder.asap.bin ${ASAP_SOURCES} ${ASAP_HEADERS})
target_link_libraries(audiodecoder.asap.bin PRIVATE /home/alwin/Development/Kodi/kodi-Matrix.build/build/kodidevkit/libkodidevkit.so rt pthread)


set(ASAP_ADDITIONAL_BINARY_EXE ${CMAKE_BINARY_DIR}/audiodecoder.asap.bin)

build_addon(audiodecoder.asap ASAP DEPLIBS)

include(CPack)
