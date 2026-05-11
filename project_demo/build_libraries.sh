#!/bin/bash

set -e

TOOLCHAIN_PATH=/home/cyf/tools/cross_compile/rk3588/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu

export CC=${TOOLCHAIN_PATH}/bin/aarch64-none-linux-gnu-gcc
export CXX=${TOOLCHAIN_PATH}/bin/aarch64-none-linux-gnu-g++
export AR=${TOOLCHAIN_PATH}/bin/aarch64-none-linux-gnu-ar
export RANLIB=${TOOLCHAIN_PATH}/bin/aarch64-none-linux-gnu-ranlib
export LD=${TOOLCHAIN_PATH}/bin/aarch64-none-linux-gnu-ld

BUILD_DIR=/home/cyf/app_demo/project_demo/lib_aarch64
LIB_SOURCES=/home/cyf/app_demo/project_demo/lib_sources
TOOLCHAIN_FILE=${LIB_SOURCES}/cmake_aarch64.cmake

mkdir -p ${BUILD_DIR}/{include,lib}

cat > ${TOOLCHAIN_FILE} << 'EOF'
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER aarch64-none-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-none-linux-gnu-g++)
set(CMAKE_AR aarch64-none-linux-gnu-ar)
set(CMAKE_RANLIB aarch64-none-linux-gnu-ranlib)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
EOF

echo "=========================================="
echo "Building libraries for aarch64..."
echo "=========================================="

cd ${LIB_SOURCES}

build_jsoncpp() {
    echo ""
    echo "[1/3] Building JsonCpp..."
    rm -rf jsoncpp/build_aarch64
    mkdir -p jsoncpp/build_aarch64
    cd jsoncpp/build_aarch64
    cmake .. \
        -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE} \
        -DCMAKE_INSTALL_PREFIX=${BUILD_DIR} \
        -DCMAKE_BUILD_TYPE=Release \
        -DJSONCPP_WITH_TESTS=OFF \
        -DJSONCPP_WITH_POST_BUILD=OFF \
        -DBUILD_SHARED_LIBS=ON
    make -j$(nproc)
    make install
    cd ../..
}

build_paho_c() {
    echo ""
    echo "[2/3] Building Eclipse Paho C..."
    rm -rf paho.mqtt.c/build_aarch64
    mkdir -p paho.mqtt.c/build_aarch64
    cd paho.mqtt.c/build_aarch64
    cmake .. \
        -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE} \
        -DCMAKE_INSTALL_PREFIX=${BUILD_DIR} \
        -DCMAKE_BUILD_TYPE=Release \
        -DPAHO_WITH_SSL=OFF \
        -DPAHO_BUILD_STATIC=ON \
        -DPAHO_BUILD_SHARED=ON
    make -j$(nproc)
    make install
    cd ../..
}

build_paho_cpp() {
    echo ""
    echo "[3/3] Building Eclipse Paho C++..."
    rm -rf paho.mqtt.cpp/build_aarch64
    mkdir -p paho.mqtt.cpp/build_aarch64
    cd paho.mqtt.cpp/build_aarch64
    cmake .. \
        -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE} \
        -DCMAKE_INSTALL_PREFIX=${BUILD_DIR} \
        -DCMAKE_BUILD_TYPE=Release \
        -DPAHO_BUILD_STATIC=OFF \
        -DPAHO_BUILD_SHARED=ON \
        -DPAHO_WITH_SSL=OFF \
        -DCMAKE_PREFIX_PATH=${BUILD_DIR}
    make -j$(nproc)
    make install
    cd ../..
}

if [ -d "jsoncpp" ]; then
    build_jsoncpp
else
    echo "JsonCpp source not found, skipping..."
fi

if [ -d "paho.mqtt.c" ]; then
    build_paho_c
else
    echo "Paho C source not found, skipping..."
fi

if [ -d "paho.mqtt.cpp" ]; then
    build_paho_cpp
else
    echo "Paho C++ source not found, skipping..."
fi

echo ""
echo "=========================================="
echo "Build completed!"
echo "Libraries installed to: ${BUILD_DIR}"
echo "=========================================="

echo ""
echo "Libraries:"
ls -la ${BUILD_DIR}/lib/

echo ""
echo "Headers:"
ls -la ${BUILD_DIR}/include/
