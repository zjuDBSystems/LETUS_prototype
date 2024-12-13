# LETUS_prototype
This is a prototype implementation of [LETUS: A Log-Structured Efficient Trusted Universal BlockChain Storage](https://doi.org/10.1145/3626246.3653390).

## Dependencies
* [OpenSSL](https://www.openssl.org/) >= 1.1.0

## Build
build for release
```
$ ./build.sh
```
build for debug
```
$ ./build.sh debug
```

## Run
This command will run the get_put test with batch_size=10 (`-b 10`), value_size=5 (`-v 5`) for 1 version (`-n 1`).
```
$ ./run.sh -b 10 -v 5 -n 1 2>run.log
```
Note: in current state, the execution result is still `[ERROR] Program crashed`.

## Run experiments
This command will run experiments to scan batch size in (10,20,...,60), value length in (256B, 512B, 1KB, 2KB).
The execution will produce a plot of latency and throughput in `exps/results`
```
$ cd exps/
$ ./test_get_put_2.sh 2> get_put_2.log
```