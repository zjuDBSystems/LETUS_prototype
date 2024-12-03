rm -rf data/*;
cd build/;
rm -rf ./*;
cmake  -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_BUILD_TYPE=Debug ..; 
make;