#!/bin/bash

set -e

PROJECT_DIR=/home/cyf/app_demo/project_demo
BUILD_DIR=${PROJECT_DIR}/build
LIB_DIR=${PROJECT_DIR}/lib
LIB_AARCH64_DIR=${PROJECT_DIR}/lib_aarch64

show_help() {
    echo "Usage: ./build.sh [mode]"
    echo ""
    echo "Modes:"
    echo "  local     Build for local x86_64 (default)"
    echo "  cross     Cross compile for RK3588 aarch64"
    echo "  clean     Clean build directory"
    echo ""
    echo "Examples:"
    echo "  ./build.sh          # Local build"
    echo "  ./build.sh cross    # Cross compilation"
    echo "  ./build.sh clean    # Clean build directory"
}

clean() {
    echo "Cleaning build directory..."
    rm -rf ${BUILD_DIR}
    mkdir -p ${BUILD_DIR}
    echo "Done."
}

local_build() {
    echo "=========================================="
    echo "Building for LOCAL (x86_64)..."
    echo "=========================================="

    cd ${BUILD_DIR}
    cmake ..
    make -j$(nproc)

    echo ""
    echo "=========================================="
    echo "Build completed!"
    echo "=========================================="
    file app_exe
    ls -lh app_exe
}

cross_build() {
    echo "=========================================="
    echo "Building for RK3588 (aarch64)..."
    echo "=========================================="

    if [ ! -f "${LIB_AARCH64_DIR}/lib/libjsoncpp.so" ]; then
        echo "Error: aarch64 libraries not found in ${LIB_AARCH64_DIR}"
        echo "Please run ./build_libraries.sh first to build libraries."
        exit 1
    fi

    cd ${BUILD_DIR}
    cmake -DCROSS_COMPILE=ON ..
    make -j$(nproc)

    echo ""
    echo "=========================================="
    echo "Cross compilation completed!"
    echo "=========================================="
    file app_exe
    ls -lh app_exe
}

if [ ! -d "$BUILD_DIR" ]; then
    mkdir -p $BUILD_DIR
fi

case "${1:-local}" in
    local)
        local_build
        ;;
    cross)
        cross_build
        ;;
    clean)
        clean
        ;;
    help|-h|--help)
        show_help
        ;;
    *)
        echo "Unknown option: $1"
        echo ""
        show_help
        exit 1
        ;;
esac
