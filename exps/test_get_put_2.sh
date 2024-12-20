#!/bin/bash
export ASAN_OPTIONS=detect_leaks=0
# 编译项目
cd ../
./build.sh

# 创建结果目录
cd exps/
mkdir -p results
cd results
mkdir -p get_put_2
cd ..

# 定义测试参数数组
batch_sizes=(500 1000 2000 4000)
value_sizes=(256 512 1024 2048)
n_test=10
data_path="$PWD/../data/"
index_path="$PWD/../"
echo "data_path: $data_path"
echo "index_path: $index_path"

# 创建结果文件
echo "batch_size,value_size,n_test,get_latency,put_latency,get_throughput,put_throughput" > results/get_put_2_results.csv

# 运行测试
for batch_size in "${batch_sizes[@]}"; do
    for value_size in "${value_sizes[@]}"; do
        # 清理数据文件夹
        rm -rf $data_path
        mkdir -p $data_path
        rm -rf "${index_path}/IndexFile/"
        mkdir -p "${index_path}/IndexFile/"
        
        result_path="$PWD/results/get_put_2/b${batch_size}v${value_size}.csv"
        echo "result_path: $result_path"
        echo "cmd: ../build_release/bin/get_put_2 -b $batch_size -v $value_size -n $n_test -d $data_path -i $index_path -r $result_path"
        # 运行测试并提取结果
        ../build_release/bin/get_put_2 -b $batch_size -v $value_size -n $n_test -d $data_path -i $index_path -r $result_path
        # output=$(../build_release/bin/get_put_2 -b $batch_size -v $value_size -n $n_test -d $data_path -i $index_path -r $result_path)
        
        # 使用awk提取平均延迟和吞吐量
        # put_latency=$(echo "$output" | grep "latency:" | awk '{print $3}')
        # get_latency=$(echo "$output" | grep "latency:" | awk '{print $6}')
        # put_throughput=$(echo "$output" | grep "throughput:" | awk '{print $3}')
        # get_throughput=$(echo "$output" | grep "throughput:" | awk '{print $6}')
        
        # 保存结果
        echo "$batch_size,$value_size,$n_test,$put_latency,$get_latency,$put_throughput,$get_throughput" >> results/get_put_2_results.csv
        sleep 5
    done
done

python3 plot.py get_put_2