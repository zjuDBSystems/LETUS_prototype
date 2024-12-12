#!/bin/bash

# 添加颜色输出函数
print_status() {
    echo -e "\033[1;34m[BUILD]\033[0m $1"
}

print_error() {
    echo -e "\033[1;31m[ERROR]\033[0m $1"
}

# 设置默认构建类型
BUILD_TYPE=${1:-Release}

# 验证构建类型
case $(echo "$BUILD_TYPE" | tr '[:upper:]' '[:lower:]') in
    debug|release)
        print_status "Build type: ${BUILD_TYPE}"
        ;;
    *)
        print_error "Invalid build type. Use 'debug' or 'release'"
        exit 1
        ;;
esac

# 根据构建类型设置目录
BUILD_DIR="build_$(echo "$BUILD_TYPE" | tr '[:upper:]' '[:lower:]')"

# 清理旧的构建目录
print_status "Cleaning old build directories..."
rm -rf ${BUILD_DIR}/ data/

# 创建必要的目录
print_status "Creating directories..."
mkdir -p ${BUILD_DIR}/ data/

# 进入构建目录并保存项目根目录
PROJECT_ROOT=$(pwd)
cd "${BUILD_DIR}" || {
    print_error "Failed to enter build directory"
    exit 1
}

# 运行 CMAKE
print_status "Running CMAKE..."
if ! cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCMAKE_VERBOSE_MAKEFILE=ON "${PROJECT_ROOT}"; then
    print_error "CMAKE configuration failed"
    exit 1
fi

# 编译项目
print_status "Building project..."
if ! make -j$(nproc); then
    print_error "Build failed"
    exit 1
fi

print_status "Build completed successfully!"