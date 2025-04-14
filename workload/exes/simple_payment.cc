#include <sys/time.h>
#include <unistd.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "DMMTrie.hpp"
#include "LSVPS.hpp"
#include "generator.hpp"

std::string BuildKeyName(uint64_t key_num, int key_len) {
  std::string key_num_str = std::to_string(key_num);
  int zeros = key_len - key_num_str.length();
  zeros = std::max(0, zeros);
  std::string key_name = "";
  return key_name.append(zeros, '0').append(key_num_str);
}

int main(int argc, char** argv) {
  int num_accout = 40000000;  // 40,000,000(40M) 2,000,000(2M)
  int load_batch_size = 5000;
  int num_txn = 10000;
  int txn_batch_size = 1000;
  int key_len = 64;
  std::string data_path = "data/";
  std::string index_path = "index";
  std::string result_path = "exps/results/test.csv";

  // init database
  LSVPS* page_store = new LSVPS(index_path);
  VDLS* value_store = new VDLS(data_path);
  DMMTrie* trie = new DMMTrie(0, page_store, value_store);
  page_store->RegisterTrie(trie);

  // load data
  int num_load_version = num_accout / load_batch_size;
  int version = 1;
  CounterGenerator key_generator(1);
  for (; version <= num_load_version; version++) {
    auto start = chrono::system_clock::now();
    for (int i = 0; i < load_batch_size; i++) {
      std::string key = BuildKeyName(key_generator.Next(), key_len);
      std::string val = std::to_string(10);
      trie->Put(0, version, key, val);
    }
    trie->Commit(version);
    auto end = chrono::system_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
    double load_latency = double(duration.count()) *
                          chrono::microseconds::period::num /
                          chrono::microseconds::period::den;
    std::cout << "version " << version << ", load latnecy:" << load_latency
              << ","
              << "put throughput:" << load_batch_size / load_latency
              << std::endl;
  }

  std::cout << "load " << num_accout << " accounts, current version is "
            << version << std::endl;
  // transaction
  int num_txn_version = num_txn / txn_batch_size;
  UniformGenerator txn_key_generator(1, num_accout);
  for (; version <= num_load_version + num_txn_version; version++) {
    auto start = chrono::system_clock::now();
    for (int i = 0; i < txn_batch_size; i++) {
      std::string key_send = BuildKeyName(txn_key_generator.Next(), key_len);
      std::string key_recv = BuildKeyName(txn_key_generator.Next(), key_len);
      DMMTrieProof proof_send = trie->GetProof(0, version - 1, key_send);
      DMMTrieProof proof_recv = trie->GetProof(0, version - 1, key_recv);
      int value_send = std::stoi(proof_send.value);
      int value_recv = std::stoi(proof_recv.value);
      if (value_send > 0) {
        value_send -= 1;
        value_recv += 1;
      }
      trie->Put(0, version, key_send, std::to_string(value_send));
      trie->Put(0, version, key_recv, std::to_string(value_recv));
    }

    trie->Commit(version);
    auto end = chrono::system_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
    double txn_latency = double(duration.count()) *
                         chrono::microseconds::period::num /
                         chrono::microseconds::period::den;
    std::cout << "transaction, version " << version
              << " latnecy:" << txn_latency << ","
              << "transaction throughput:" << txn_batch_size / txn_latency
              << std::endl;
  }
  std::cout << "process " << num_txn << " transactions, current version is "
            << version << std::endl;

  return true;
}