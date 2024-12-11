rm -rf build/;
rm -rf data/;
mkdir -p build/;
mkdir -p data/;
cd build/;
cmake  -DCMAKE_VERBOSE_MAKEFILE=OFF ..; 
make;