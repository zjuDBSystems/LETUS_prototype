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

inline char RandomPrintChar() { return rand() % 94 + 33; }

std::string BuildKeyName(uint64_t key_num, int key_len) {
  std::string key_num_str = std::to_string(key_num);
  int zeros = key_len - key_num_str.length();
  zeros = std::max(0, zeros);
  std::string key_name = "";
  return key_name.append(zeros, '0').append(key_num_str);
}

int main(int argc, char** argv) {
  int num_accout = 500000000;  // 40,000,000(40M) 2,000,000(2M)
  int load_batch_size = 5000;
  int num_txn_version = 10;
  int txn_batch_size = 600;
  int key_len = 9;
  int value_len = 10;
  std::string data_path = "data/";
  std::string index_path = "index";
  std::string result_path = "exps/results/test.csv";

  int opt;
  while ((opt = getopt(argc, argv, "a:b:t:z:k:v:d:i:r:")) != -1) {
    switch (opt) {
      case 'a':  // num_accout
      {
        char* strtolPtr;
        num_accout = strtoul(optarg, &strtolPtr, 10);
        if ((*optarg == '\0') || (*strtolPtr != '\0') || (num_accout <= 0)) {
          std::cerr << "option -b requires a numeric arg\n" << std::endl;
        }
        break;
      }

      case 'b':  // load_batch_size
      {
        char* strtolPtr;
        load_batch_size = strtoul(optarg, &strtolPtr, 10);
        if ((*optarg == '\0') || (*strtolPtr != '\0') ||
            (load_batch_size <= 0)) {
          std::cerr << "option -n requires a numeric arg\n" << std::endl;
        }
        break;
      }

      case 't':  // num_txn
      {
        char* strtolPtr;
        num_txn_version = strtoul(optarg, &strtolPtr, 10);
        if ((*optarg == '\0') || (*strtolPtr != '\0') ||
            (num_txn_version <= 0)) {
          std::cerr << "option -k requires a numeric arg\n" << std::endl;
        }
        break;
      }

      case 'z':  // txn_batch_size
      {
        char* strtolPtr;
        txn_batch_size = strtoul(optarg, &strtolPtr, 10);
        if ((*optarg == '\0') || (*strtolPtr != '\0') ||
            (txn_batch_size <= 0)) {
          std::cerr << "option -k requires a numeric arg\n" << std::endl;
        }
        break;
      }

      case 'k':  // length of key.
      {
        char* strtolPtr;
        key_len = strtoul(optarg, &strtolPtr, 10);
        if ((*optarg == '\0') || (*strtolPtr != '\0') || (key_len <= 0)) {
          std::cerr << "option -k requires a numeric arg\n" << std::endl;
        }
        break;
      }

      case 'v':  // length of value.
      {
        char* strtolPtr;
        value_len = strtoul(optarg, &strtolPtr, 10);
        if ((*optarg == '\0') || (*strtolPtr != '\0') || (value_len <= 0)) {
          std::cerr << "option -v requires a numeric arg\n" << std::endl;
        }
        break;
      }

      case 'd':  // data path
      {
        data_path = optarg;
        break;
      }

      case 'i':  // index path
      {
        index_path = optarg;
        break;
      }

      case 'r':  // result path
      {
        result_path = optarg;
        break;
      }

      default:
        std::cerr << "Unknown argument " << argv[optind] << std::endl;
        break;
    }
  }

  // init database
  LSVPS* page_store = new LSVPS(index_path);
  VDLS* value_store = new VDLS(data_path);
  DMMTrie* trie = new DMMTrie(0, page_store, value_store);
  page_store->RegisterTrie(trie);
  // prepare result file
  ofstream rs_file;
  rs_file.open(result_path, ios::trunc);
  rs_file << "version,get_latency,put_latency,get_throughput,put_throughput"
          << std::endl;
  rs_file.close();
  rs_file.open(result_path, ios::app);

  // load data
  key_len += key_len % 2 ? 0 : 1;  // make sure key_len is odd
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
    if (version % 200 == 0) {
      std::cout << "version " << version << ", load latnecy:" << load_latency
                << ","
                << "load throughput:" << load_batch_size / load_latency
                << std::endl;
    }
  }

  std::cout << "load " << num_accout << " accounts, current version is "
            << version << std::endl;
  // transaction
  int num_txn = num_txn_version * txn_batch_size;
  int random_keys[num_txn];
  UniformGenerator active_judger(0, 1000);  // if access active accounts
  UniformGenerator active_key_generator(num_accout * 0.8, num_accout);
  UniformGenerator inactive_key_generator(1, num_accout * 0.8);
  for (int i = 0; i < num_txn; i++) {
    if (active_judger.Next() < 800) {
      random_keys[i] = active_key_generator.Next();
    } else {
      random_keys[i] = inactive_key_generator.Next();
    }
  }
  int txn_key_id = 0;
  for (; version <= num_load_version + num_txn_version; version++) {
    // test get
    auto start = chrono::system_clock::now();
    for (int i = 0; i < txn_batch_size; i++) {
      std::string key = BuildKeyName(random_keys[txn_key_id + i], key_len);
      DMMTrieProof proof_send = trie->GetProof(0, version - 1, key);
    }
    auto end = chrono::system_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
    double get_latency = double(duration.count()) *
                         chrono::microseconds::period::num /
                         chrono::microseconds::period::den;
    std::cout << "version " << version << " get latnecy:" << get_latency << ","
              << "get throughput:" << txn_batch_size / get_latency << std::endl;
    // test put
    start = chrono::system_clock::now();
    for (int i = 0; i < txn_batch_size; i++) {
      std::string key = BuildKeyName(random_keys[txn_key_id + i], key_len);
      std::string val = "";
      val = val.append(value_len, RandomPrintChar());
      trie->Put(0, version, key, val);
    }
    trie->Commit(version);
    end = chrono::system_clock::now();
    duration = chrono::duration_cast<chrono::microseconds>(end - start);
    double put_latency = double(duration.count()) *
                         chrono::microseconds::period::num /
                         chrono::microseconds::period::den;
    std::cout << "version " << version << " put latnecy:" << put_latency << ","
              << "put throughput:" << txn_batch_size / put_latency << std::endl;
    txn_key_id += txn_batch_size;

    rs_file << version << "," << get_latency << "," << put_latency << ","
            << txn_batch_size / get_latency << ","
            << txn_batch_size / put_latency << std::endl;
  }
  std::cout << "process " << num_txn << " get, current version is " << version
            << std::endl;
  rs_file.close();
  return true;
}