#!/bin/bash
export ASAN_OPTIONS=detect_leaks=0
# 编译项目
cd ../
./build.sh

# 创建结果目录
cd exps/
mkdir -p results
cd results
mkdir -p put_get_hist_count
cd ..

# 定义测试参数数组
key_size=64
batch_sizes=(500 1000 2000 3000 4000 5000)
value_sizes=(256 512 1024 2048)
n_test=20
data_path="$PWD/../data/"
index_path="$PWD/../index"
echo "data_path: $data_path"
echo "index_path: $index_path"

# 运行测试
for batch_size in "${batch_sizes[@]}"; do
    for value_size in "${value_sizes[@]}"; do
        set -x
        # 清理数据文件夹
        rm -rf $data_path
        mkdir -p $data_path
        rm -rf $index_path
        mkdir -p $index_path
        
        result_path="$PWD/results/put_get_hist_count/b${batch_size}v${value_size}.csv"
        echo $(date "+%Y-%m-%d %H:%M:%S") >> results/test_get_put_hashed_key_k${key_size}_${mode}.log
        echo "batch_size: ${batch_size}, value_size: ${value_size}, key_size: ${key_size}" >> results/test_get_put_hashed_key_k${key_size}_${mode}.log
        # 运行测试并提取结果
        ../build_release/bin/put_get_hist_count -b $batch_size -k $key_size -v $value_size -n $n_test -d $data_path -i $index_path -r $result_path
        
        sleep 5
        set +x
    done
done

python3 plot.py put_get_hist_count