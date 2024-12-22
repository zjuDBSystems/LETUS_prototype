cd ../
./build.sh --build-type debug


cd exps/
# ${PWD}/../build_debug/bin/ycsb_simple 
${PWD}/../build_debug/bin/ycsb_simple -datapath ${PWD}/data/ -idxpath ${PWD}/data -respath ${PWD}/results/ -confpath ${PWD}/../workload/configures/workloada.spec