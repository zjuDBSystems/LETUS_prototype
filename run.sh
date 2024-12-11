#!/bin/bash
rm -rf data/*
rm -rf IndexFile/*
# 设置更大的堆栈大小以获取更好的调试信息
# ulimit -n 4096
# 添加 ASAN 选项以输出更详细的日志
export ASAN_OPTIONS=detect_leaks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1:detect_invalid_pointer_pairs=2
# 将输出重定向到文件
./build/bin/get_put_2 -n 300 2>asan.log