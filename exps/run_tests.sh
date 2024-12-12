#!/bin/bash

# 编译项目
cd ../
./build.sh

# 创建结果目录
cd exps/
mkdir -p results

# 定义测试参数数组
batch_sizes=(10 20 30 50)
value_sizes=(256 512 1024 2048)

# 创建结果文件
echo "batch_size,value_size,put_latency,get_latency,put_throughput,get_throughput" > results/results.csv

# 运行测试
for batch_size in "${batch_sizes[@]}"; do
    for value_size in "${value_sizes[@]}"; do
        echo "Running test with batch_size=$batch_size, value_size=$value_size"
        # 运行测试并提取结果
        output=$(../build/bin/get_put -b $batch_size -v $value_size)
        
        # 使用awk提取平均延迟和吞吐量
        put_latency=$(echo "$output" | grep "latency:" | awk '{print $3}')
        get_latency=$(echo "$output" | grep "latency:" | awk '{print $6}')
        put_throughput=$(echo "$output" | grep "throughput:" | awk '{print $3}')
        get_throughput=$(echo "$output" | grep "throughput:" | awk '{print $6}')
        
        # 保存结果
        echo "$batch_size,$value_size,$put_latency,$get_latency,$put_throughput,$get_throughput" >> results/results.csv
    done
done

python3 plot.py