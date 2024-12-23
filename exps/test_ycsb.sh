cd ../
./build.sh --build-type debug


cd exps/
rm -rf data/
mkdir data/
# ${PWD}/../build_debug/bin/ycsb_simple 
${PWD}/../build_debug/bin/ycsb_simple -datapath ${PWD}/data/ -idxpath ${PWD}/data -respath ${PWD}/results/ -confpath ${PWD}/../workload/configures/myworkload.spec