CMAKE_MINIMUM_REQUIRED(VERSION 3.12)

PROJECT(letus_prototype)

find_package(OpenSSL 1.1 REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

set(CMAKE_CXX_COMPILER "clang++")
# 添加 AddressSanitizer 标志
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(lib/)
include_directories(workload/lib/)

add_executable(get_put "workload/exes/get_put.cc" src/DMMTrie.cpp src/LSVPS.cpp)
target_link_libraries(get_put OpenSSL::SSL OpenSSL::Crypto)




