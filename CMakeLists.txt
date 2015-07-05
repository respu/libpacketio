cmake_minimum_required(VERSION 2.8)

project(libpacketio)

option(BUILD_LINUX_TPACKET "Build Linux tpacket" ON)
option(BUILD_NETMAP "Build Linux tpacket" OFF)

set(libHeaders ${libHeaders}
    include/packetio/packetio.h
)

if(BUILD_LINUX_TPACKET)
    set(libSrcs ${libSrcs} linux-tpacket.c) 
endif(BUILD_LINUX_TPACKET)

if(BUILD_NETMAP)
    set(includes ${includes} -I${NETMAP_DIR}/sys)
    set(libSrcs ${libSrcs} netmap.c) 
endif(BUILD_NETMAP)

set(CMAKE_C_FLAGS "-Iinclude ${includes} -Wall -O3 -g -std=gnu11")

add_library(packetio ${libHeaders} ${libSrcs})

add_executable(packet-trace packet-trace.c)
target_include_directories(packet-trace PUBLIC include)
target_link_libraries(packet-trace LINK_PUBLIC packetio)
