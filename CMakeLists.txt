cmake_minimum_required(VERSION 2.8)

project(libpacketio)

set(CMAKE_C_FLAGS "-Iinclude -Wall -O3 -g -std=gnu11")

set(libHeaders ${libHeaders}
    include/packetio/packetio.h
)

set(libSrcs ${libSrcs}
    linux-tpacket.c
)

add_library(packetio ${libHeaders} ${libSrcs})

add_executable(packet-trace packet-trace.c)
target_include_directories(packet-trace PUBLIC include)
target_link_libraries(packet-trace LINK_PUBLIC packetio)
