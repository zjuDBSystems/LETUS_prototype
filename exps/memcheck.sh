# 编译项目
cd ../
./build.sh --build-type release --cxx g++
./build.sh --build-type debug --cxx g++
# 创建结果目录
cd exps/
mkdir -p results
cd results
mkdir -p get_put_hashed_key_k$key_size
cd ..

# 定义测试参数数组
key_size=32  # 20:SHA-1, 32:SHA-256
batch_size=500
value_size=1024
n_test=5
data_path="$PWD/../data/"
index_path="$PWD/../index"
echo "data_path: $data_path"
echo "index_path: $index_path"

result_path="$PWD/results/get_put_hashed_key_k${key_size}/b${batch_size}v${value_size}.csv"
echo "result_path: $result_path"
echo "cmd: ../build_release/bin/get_put_hashed_key  -b $batch_size -v $value_size -k $key_size -n $n_test -d $data_path -i $index_path -r $result_path"
# 运行测试并提取结果
rm -rf $data_path
mkdir -p $data_path
rm -rf $index_path
mkdir -p $index_path
valgrind --tool=memcheck --leak-check=full ../build_release/bin/get_put_hashed_key -b $batch_size -v $value_size -k $key_size -n $n_test -d $data_path -i $index_path -r $result_path
rm -rf $data_path
mkdir -p $data_path
rm -rf $index_path
mkdir -p $index_path
../build_debug/bin/get_put_hashed_key -b $batch_size -v $value_size -k $key_size -n $n_test -d $data_path -i $index_path -r $result_path &> results/memcheck_k$key_size.log
python3 count.py
rm -rf $data_path
mkdir -p $data_path
rm -rf $index_path
mkdir -p $index_path