CMAKE_MINIMUM_REQUIRED(VERSION 3.12)

PROJECT(letus_prototype)
find_package(OpenSSL 1.1 REQUIRED)
# 查找glibc
find_package(PkgConfig REQUIRED)
pkg_check_modules(GNUC standard glibc >= 2.34)

# 查找glibc
# find_package(PkgConfig REQUIRED)
pkg_check_modules(GNUC standard glibc >= 2.34)

include_directories(${OPENSSL_INCLUDE_DIR})

set(CMAKE_CXX_COMPILER "clang++")
# set(CMAKE_CXX_COMPILER "g++")
# set(CMAKE_CXX_COMPILER "clang")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O2 -pthread -lssl -Wall -Wno-unused-function -fPIC")
# 添加 AddressSanitizer 标志
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g -fPIC -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

aux_source_directory(lib/ letus_lib)
aux_source_directory(src/ letus_src)
# aux_source_directory(workload/lib/ workload_lib)

include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(lib/)
include_directories(workload/lib/)

# add_executable(trace_replay "workload/exes/trace_replay.cc")
# add_executable(trace_replay "workload/exes/trace_replay.cc" ${letus_lib})
# target_link_libraries(trace_replay jsoncpp)

add_executable(get_put "workload/exes/get_put.cc" ${letus_src})
target_link_libraries(get_put OpenSSL::SSL OpenSSL::Crypto ${GNUC_LIBRARIES})
<<<<<<< HEAD
=======
add_executable(get_put_2 "workload/exes/get_put_2.cc" ${letus_src})
target_link_libraries(get_put_2 OpenSSL::SSL OpenSSL::Crypto ${GNUC_LIBRARIES})
>>>>>>> dev

# add_executable(simple "workload/exes/simple.cc")

add_library(letus STATIC ${letus_lib} ${letus_src})
target_link_libraries(letus OpenSSL::SSL OpenSSL::Crypto)




