# rm -rf data/*;
cd build/;
rm -rf ./*;
cmake  -DCMAKE_VERBOSE_MAKEFILE=ON ..; 
make;