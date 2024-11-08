CMAKE_MINIMUM_REQUIRED(VERSION 3.12)

PROJECT(letus_prototype)

set(PROTOBUF_PROTOC_EXECUTABLE "/usr/bin/protoc")
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

find_package(Protobuf 2.6.1 REQUIRED)
message(STATUS "${PROTOBUF_PROTOC_EXECUTABLE}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2 -pthread -lssl -Wall -Wno-unused-function -fPIC")

add_subdirectory(workload)

