# LETUS_prototype
This is a prototype implementation of [LETUS: A Log-Structured Efficient Trusted Universal BlockChain Storage](https://doi.org/10.1145/3626246.3653390).

## Dependencies
* [OpenSSL](https://www.openssl.org/) >= 1.1.0

## Build
in `lib/VDLS.hpp` line 26 and line 46
change the directory to the absolute path of the data directory.

```
$ git clone https://github.com/kusakabe/LETUS_prototype.git
$ cd LETUS_prototype
$ ./build.sh
```

## Run
**put-then-get benchmark**
```
$ ./build/bin/get_put
```