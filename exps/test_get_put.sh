#!/bin/bash

# 编译项目
cd ../
./build.sh

# 创建结果目录
cd exps/
mkdir -p results
cd results
mkdir -p get_put
cd ..

# 定义测试参数数组
batch_sizes=(500 1000 2000 3000 4000 5000)
value_sizes=(256 512 1024 2048)
n_test=10
data_path="$PWD/../data/"
index_path="$PWD/../"
echo "data_path: $data_path"
echo "index_path: $index_path"

# 创建结果文件
echo "batch_size,value_size,n_test,get_latency,put_latency,get_throughput,put_throughput" > results/get_put_results.csv

# 运行测试
for batch_size in "${batch_sizes[@]}"; do
    for value_size in "${value_sizes[@]}"; do
        result_path="$PWD/results/get_put/b${batch_size}v${value_size}.csv"
        echo "result_path: $result_path"
        echo "cmd: ../build_release/bin/get_put -b $batch_size -v $value_size -n $n_test -d $data_path -i $index_path -r $result_path"
        # 运行测试并提取结果
        output=$(../build_release/bin/get_put -b $batch_size -v $value_size -n $n_test -d $data_path -i $index_path -r $result_path)
        
        # 使用awk提取平均延迟和吞吐量
        put_latency=$(echo "$output" | grep "latency:" | awk '{print $3}')
        get_latency=$(echo "$output" | grep "latency:" | awk '{print $6}')
        put_throughput=$(echo "$output" | grep "throughput:" | awk '{print $3}')
        get_throughput=$(echo "$output" | grep "throughput:" | awk '{print $6}')
        
        # 保存结果
        echo "$batch_size,$value_size,$n_test,$put_latency,$get_latency,$put_throughput,$get_throughput" >> results/get_put_results.csv
    done
done

python plot.py get_put