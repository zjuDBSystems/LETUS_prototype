CMAKE_MINIMUM_REQUIRED(VERSION 3.12)

PROJECT(letus_prototype)

find_library(jsoncpp REQUIRED)

set(CMAKE_CXX_COMPILER "/usr/bin/g++-8")
# set(CMAKE_CXX_COMPILER "clang")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O2 -pthread -lssl -Wall -Wno-unused-function -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

aux_source_directory(lib/ letus_lib)
include_directories(lib/)

add_executable(trace_replay "workload/exes/trace_replay.cc" ${letus_lib})
target_link_libraries(trace_replay jsoncpp)


# set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)



