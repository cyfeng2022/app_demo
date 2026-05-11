cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(CROSS_COMPILE "Enable cross compilation for RK3588" OFF)

if(CROSS_COMPILE)
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/toolchain_rk3588.cmake)
    set(CROSS_PREFIX aarch64-none-linux-gnu-)
else()
    set(CROSS_PREFIX "")
endif()

project(app_exe)

set(INC_DIR ${CMAKE_SOURCE_DIR}/inc)
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(LIB_DIR ${CMAKE_SOURCE_DIR}/lib)

if(CROSS_COMPILE)
    set(LIB_AARCH64_DIR ${CMAKE_SOURCE_DIR}/lib_aarch64)
endif()

include_directories(${INC_DIR})
include_directories(${CMAKE_SOURCE_DIR}/lib_sources/spdlog/include)

if(CROSS_COMPILE)
    include_directories(${LIB_AARCH64_DIR}/include)
else()
    include_directories(${LIB_DIR}/include)
endif()

find_package(Threads REQUIRED)

file(GLOB SOURCES "${SRC_DIR}/*.cpp")

add_executable(app_exe ${SOURCES})

if(CROSS_COMPILE)
    target_link_libraries(app_exe
        ${LIB_AARCH64_DIR}/lib/libjsoncpp.so
        ${LIB_AARCH64_DIR}/lib/libpaho-mqtt3a.so
        ${LIB_AARCH64_DIR}/lib/libpaho-mqttpp3.so
        Threads::Threads
    )
else()
    target_link_libraries(app_exe
        ${LIB_DIR}/lib/libjsoncpp.so
        ${LIB_DIR}/lib/libpaho-mqtt3a.so
        ${LIB_DIR}/lib/libpaho-mqttpp3.so
        Threads::Threads
    )
endif()
