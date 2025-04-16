#!/bin/bash
export ASAN_OPTIONS=detect_leaks=0
# 编译项目
cd ../
./build.sh

# 创建结果目录
cd exps/
mkdir -p results
cd results
mkdir -p micro_benchmark
cd ..

# 定义测试参数数组
# standalone evaluation
load_account=(20000000 40000000 80000000 100000000)
batch_sizes=(8000) 
value_sizes=(800)
# micro-benchmark evaluation
# load_account=(40000000)
# batch_sizes=(500 1000 2000 3000 4000 5000)
# value_sizes=(256 512 1024 2048)
num_transaction_version=20
load_batch_size=20000
key_size=65

data_path="$PWD/../data/"
index_path="$PWD/../index"
echo "data_path: $data_path"
echo "index_path: $index_path"

# 运行测试
for n_acc in "${load_account[@]}"; do
    for batch_size in "${batch_sizes[@]}"; do
        for value_size in "${value_sizes[@]}"; do
            set -x
            # 清理数据文件夹
            rm -rf $data_path
            mkdir -p $data_path
            rm -rf $index_  path
            mkdir -p $index_path
            
            result_path="$PWD/results/micro_benchmark/acc${n_acc}b${batch_size}v${value_size}.csv"
            echo $(date "+%Y-%m-%d %H:%M:%S") 
            echo "num account: ${n_acc}, batch_size: ${batch_size}, value_size: ${value_size}, key_size: ${key_size}" 
            # 运行测试并提取结果
            ../build_release/bin/micro_benchmark -a $n_acc -b $load_batch_size -t $num_transaction_version -z $batch_size -k $key_size -v $value_size -d $data_path -i $index_path -r $result_path
            
            sleep 5
            set +x
        done
    done
done

python3 plot.py micro_benchmark