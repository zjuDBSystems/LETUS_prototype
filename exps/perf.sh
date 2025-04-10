#!/bin/bash
mode=release

# 定义测试参数数组
key_size=32  # 20:SHA-1, 32:SHA-256
batch_sizes=(100 200 300 400 500)
value_sizes=(1024)
n_test=5
data_path="$PWD/../data/"
index_path="$PWD/../index"
echo "data_path: $data_path"
echo "index_path: $index_path"


# 检查必要的性能分析工具
check_tools() {
    if ! command -v perf &> /dev/null; then
        echo "Error: perf is not installed. Please install it first."
        exit 1
    fi
    if [ ! -d "./FlameGraph" ]; then
        echo "Error: FlameGraph directory not found. Please clone it first."
        exit 1
    fi
}

# 编译项目
cd ../
./build.sh --build-type ${mode} --cxx g++
# 创建结果目录
cd exps/
mkdir -p results
cd results
mkdir -p get_put_hashed_key
mkdir -p profiling
cd ..

# 定义测试参数数组
key_size=32  # 20:SHA-1, 32:SHA-256
batch_sizes=(500)
value_sizes=(1024)
n_test=3
data_path="$PWD/../data/"
index_path="$PWD/../index"
echo "data_path: $data_path"
echo "index_path: $index_path"

# 创建结果文件
echo "batch_size,value_size,n_test,get_latency,put_latency,get_throughput,put_throughput" > results/get_put_hashed_key_results.csv

# 运行测试
for batch_size in "${batch_sizes[@]}"; do
    for value_size in "${value_sizes[@]}"; do
        set -x
        # 清理数据文件夹
        rm -rf $data_path
        mkdir -p $data_path
        rm -rf $index_path
        mkdir -p $index_path
        
        result_path="$PWD/results/get_put_hashed_key/b${batch_size}v${value_size}.csv"
        echo "result_path: $result_path"
        echo "cmd: ../build_debug/bin/get_put_hashed_key -b $batch_size -v $value_size -n $n_test -d $data_path -i $index_path -r $result_path"
        
        # 使用perf进行性能分析
        sudo perf record -g -F 99 ../build_debug/bin/get_put_hashed_key -b $batch_size -v $value_size -k $key_size -n $n_test -d $data_path -i $index_path -r $result_path
        
        # 生成火焰图
        sudo perf script | ./FlameGraph/stackcollapse-perf.pl > perf.folded
        ./FlameGraph/flamegraph.pl perf.folded > results/profiling/flamegraph_${batch_size}_${value_size}.svg
        
        # 生成性能报告
        sudo perf report --stdio > results/profiling/perf_report_${batch_size}_${value_size}.txt
        
        # 保存结果
        echo "$batch_size,$value_size,$n_test,$put_latency,$get_latency,$put_throughput,$get_throughput" >> results/get_put_2_results.csv
        sleep 5
    done
done

# python3 plot.py get_put_hashed_key
