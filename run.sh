rm -rf data/*;
# 将输出重定向到文件
./build/bin/get_put_2 -n 1000 2>asan.log
