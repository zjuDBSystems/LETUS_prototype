cd ..
./build.sh
cd gowrapper
gcc -o test_letus_lib.o test_letus_lib.c /home/xinyu.chen/LETUS_prototype/build_release/libletus.a -lstdc++ -lssl -lcrypto