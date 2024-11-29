CMAKE_MINIMUM_REQUIRED(VERSION 3.12)

PROJECT(letus_prototype)

find_library(jsoncpp REQUIRED)
find_package(OpenSSL 1.1 REQUIRED)

set(CMAKE_CXX_COMPILER "clang++")
# set(CMAKE_CXX_COMPILER "/usr/bin/g++-8")
# set(CMAKE_CXX_COMPILER "clang")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O2 -pthread -lssl -Wall -Wno-unused-function -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fPIC")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

aux_source_directory(lib/ letus_lib)
# aux_source_directory(workload/lib/ workload_lib)
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(lib/)
include_directories(workload/lib/)

# add_executable(trace_replay "workload/exes/trace_replay.cc")
# add_executable(trace_replay "workload/exes/trace_replay.cc" ${letus_lib})
# target_link_libraries(trace_replay jsoncpp)

add_executable(get_put "workload/exes/get_put.cc")
target_link_libraries(get_put OpenSSL::SSL OpenSSL::Crypto)

# add_executable(simple "workload/exes/simple.cc")

add_library(letus STATIC ${letus_lib})
target_link_libraries(letus OpenSSL::SSL OpenSSL::Crypto)



