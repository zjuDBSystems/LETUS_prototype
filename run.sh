#!/bin/bash

# 颜色定义
BLUE='\033[1;34m'
RED='\033[1;31m'
NC='\033[0m' # No Color

# 输出函数
print_status() {
    echo -e "${BLUE}[RUN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 检查第一个参数是否为运行类型(debug/release)
if [ -z "$1" ] || { [ "$1" != "debug" ] && [ "$1" != "release" ] && [ "$1" != "DEBUG" ] && [ "$1" != "RELEASE" ]; }; then
    # 如果第一个参数不是运行类型,则使用默认的release模式,并保留所有参数
    RUN_TYPE="release"
else
    # 如果第一个参数是运行类型,则取该参数并移除
    RUN_TYPE=$1
    shift
fi

# 转换为小写
RUN_TYPE_LOWER=$(echo "$RUN_TYPE" | tr '[:upper:]' '[:lower:]')

print_status "Run type: ${RUN_TYPE_LOWER}"

# 设置构建目录
BUILD_DIR="build_${RUN_TYPE_LOWER}"

# 验证目录和可执行文件
if [ ! -d "${BUILD_DIR}" ]; then
    print_error "Build directory '${BUILD_DIR}' not found"
    print_error "Please run './build.sh ${RUN_TYPE_LOWER}' first"
    exit 1
fi

# 设置可执行文件路径
EXECUTABLE="${BUILD_DIR}/bin/get_put_2"

if [ ! -f "${EXECUTABLE}" ]; then
    print_error "Executable 'get_put_2' not found in '${BUILD_DIR}/bin/'"
    print_error "Please run './build.sh ${RUN_TYPE_LOWER}' first"
    exit 1
fi

if [ ! -x "${EXECUTABLE}" ]; then
    print_error "Executable '${EXECUTABLE}' exists but is not executable"
    print_error "Try running: chmod +x '${EXECUTABLE}'"
    exit 1
fi

# 运行程序
if [ "$RUN_TYPE_LOWER" = "debug" ]; then
    print_status "Running in DEBUG mode with ASAN enabled"
    
    # ASAN 配置
    export ASAN_OPTIONS="detect_leaks=1:\
detect_stack_use_after_return=1:\
check_initialization_order=1:\
strict_init_order=1:\
detect_invalid_pointer_pairs=2"
    
    # 创建日志目录
    mkdir -p logs
    ASAN_LOG="logs/asan_$(date '+%Y%m%d_%H%M%S').log"
    
    print_status "ASAN output will be written to: ${ASAN_LOG}"
    if ! "${EXECUTABLE}" "$@" 2>"${ASAN_LOG}"; then
        print_error "Program crashed. Check ${ASAN_LOG} for details"
        exit 1
    fi
else
    print_status "Running in RELEASE mode"
    if ! "${EXECUTABLE}" "$@"; then
        print_error "Program crashed"
        exit 1
    fi
fi

print_status "Program finished successfully"
