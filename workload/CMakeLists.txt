

FIND_LIBRARY(jsoncpp REQUIRED)
add_executable(trace_replay "exes/trace_replay.cc")
# ADD_DEPENDENCIES(trace_replay dist)
TARGET_LINK_LIBRARIES(trace_replay jsoncpp)

# set_target_properties(trace_replay PROPERTIES LINK_FLAGS "${LINK_FLAGS}")