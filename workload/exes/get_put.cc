/**
 * set the number of keys as 40M (40*1024*1024),
 * the key size as 32B,
 * vary the batch size from 500 to 4000 (5000, 10000, 20000, 30000, 40000),
 * vary the value size from 256 B to 2 KB (256, 512, 1024, 2048)
 */

#include <sys/time.h>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

#include "DMMTrie.hpp"
#include "LSVPS.hpp"
#include "generator.hpp"

// common values
uint64_t MAX_KEY = 99999;
ZipfianGenerator key_generator(1, MAX_KEY);

struct Task {
  std::vector<int> ops;
  std::vector<std::string> keys;
  std::vector<std::string> values;
};

inline char RandomPrintChar() { return rand() % 94 + 33; }

std::string BuildKeyName(uint64_t key_num, int key_len) {
  std::string key_num_str = std::to_string(key_num);
  int zeros = key_len - key_num_str.length();
  zeros = std::max(0, zeros);
  std::string key_name = "";
  return key_name.append(zeros, '0').append(key_num_str);
}

int taskGenerator(int tlen, int key_len, int value_len, Task& put_task,
                  Task& get_task) {
  timeval t0;
  gettimeofday(&t0, NULL);
  srand(t0.tv_sec * 10000000000 + t0.tv_usec * 10000);

  for (int j = 0; j < tlen; j++) {
    std::string key = BuildKeyName(key_generator.Next(), key_len);
    std::string val = "";
    val = val.append(value_len, RandomPrintChar());
    int op, n = 0;

    put_task.keys.emplace_back(key);
    put_task.values.emplace_back(val);
    put_task.ops.emplace_back(1);
    get_task.keys.emplace_back(key);
    get_task.values.emplace_back(val);
    get_task.ops.emplace_back(1);
  }
  return 0;
}

int main(int argc, char** argv) {
  int batch_size = 5000;  //
  int n_test = 10;
  int key_len = 5;      // 32
  int value_len = 256;  // 256, 512, 1024, 2048
  // init tasks
  Task* put_tasks = new Task[n_test];
  Task* get_tasks = new Task[n_test];
  for (int i = 0; i < n_test; i++) {
    if (taskGenerator(batch_size, key_len, value_len, put_tasks[i],
                      get_tasks[i])) {
      std::cerr << "fail to generate task!" << std::endl;
      return 1;
    }
  }

  // init database
  LSVPS* page_store = new LSVPS();
  VDLS* value_store = new VDLS();
  DMMTrie* trie = new DMMTrie(0, page_store, value_store);
  page_store->RegisterTrie(trie);

  // start test
  double put_latency_sum = 0;
  double get_latency_sum = 0;
  for (int i = 0; i < n_test; i++) {
    // put
    auto keys = put_tasks[i].keys;
    auto values = put_tasks[i].values;
    auto start = chrono::system_clock::now();
    for (int i = 0; i < keys.size(); i++) {
      string key = keys[i];
      string value = values[i];
      std::cout << "Hello" << std::endl;
      std::cout << "PUT:" << key << "," << value << std::endl;
      trie->Put(0, 1, key, value);
    }
    auto end = chrono::system_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
    double put_latency = double(duration.count()) *
                         chrono::microseconds::period::num /
                         chrono::microseconds::period::den;
    // get
    keys = get_tasks[i].keys;
    values = get_tasks[i].values;
    start = chrono::system_clock::now();
    for (int i = 0; i < keys.size(); i++) {
      std::string key = keys[i];
      std::cout << "GET:" << key << std::endl;
      trie->Get(0, 1, key);
    }
    end = chrono::system_clock::now();
    duration = chrono::duration_cast<chrono::microseconds>(end - start);
    double get_latency = double(duration.count()) *
                         chrono::microseconds::period::num /
                         chrono::microseconds::period::den;

    put_latency_sum += put_latency;
    get_latency_sum += get_latency;
    std::cout << "test " << i << ": ";
    std::cout << "put latency=" << put_latency << " s, ";
    std::cout << "get latency=" << get_latency << " s, ";
    std::cout << std::endl;
  }
  std::cout << "average: ";
  std::cout << "put latency=" << put_latency_sum / n_test << " s, ";
  std::cout << "get latency=" << get_latency_sum / n_test << " s, ";
  std::cout << std::endl;

  return 0;
}