export PATH=/usr/bin:/usr/include:/usr/local/include:/usr/lib/x86_64-linux-gnu:/usr/lib:/usr/local/sbin:/usr/local/lib:/usr/local/bin:/usr/sbin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin;
export LD_LIBRARY_PATH=/usr/include:/usr/local/lib:/usr/local/include:/usr/lib/x86_64-linux-gnu;

rootdir=/home/xinyu.chen/LETUS_prototype;
mkdir -p ${rootdir}/build; 
cd ${rootdir}/build; 
rm -rf *; 
which protoc;
cmake  -DCMAKE_VERBOSE_MAKEFILE=ON ..; 
make;