/**
 * set the number of keys as 40M (40*1024*1024),
 * the key size as 32B,
 * vary the batch size from 500 to 4000 (5000, 10000, 20000, 30000, 40000),
 * vary the value size from 256 B to 2 KB (256, 512, 1024, 2048)
 */

#include <sys/time.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <string>
#include <vector>

#include "DMMTrie.hpp"
#include "LSVPS.hpp"
#include "generator.hpp"

// common values
CounterGenerator key_generator(1);

std::tuple<double, double> getMemoryUsage() {
  std::ifstream statm("/proc/self/statm");
  unsigned int pmem_pages = 0;
  unsigned int vmem_pages = 0;
  statm >> pmem_pages;
  statm >> vmem_pages;
  statm.close();
  // 4KB per page
  return {static_cast<double>(pmem_pages * 4),
          static_cast<double>(vmem_pages * 4)};
}

struct Task {
  std::vector<int> ops;
  std::vector<u_int64_t> versions;
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

int taskGenerator(int tlen, int key_len, int value_len, int task_i,
                  Task& put_task, Task& get_task) {
  timeval t0;
  gettimeofday(&t0, NULL);
  srand(t0.tv_sec * 10000000000 + t0.tv_usec * 10000);
  uint64_t version = task_i + 1;
  for (int j = 0; j < tlen; j++) {
    std::string key = BuildKeyName(key_generator.Next(), key_len);
    std::string val = "";
    val = val.append(value_len, RandomPrintChar());

    put_task.keys.emplace_back(key);
    put_task.values.emplace_back(val);
    put_task.ops.emplace_back(1);
    put_task.versions.emplace_back(version);
    get_task.keys.emplace_back(key);
    get_task.values.emplace_back(val);
    get_task.ops.emplace_back(0);
    get_task.versions.emplace_back(version);
    for (int i = 0; i < get_task.keys.size(); i++) {
      if (get_task.keys[i] == key) {
        get_task.values[i] = val;
      }
    }
  }
  return 0;
}

int main(int argc, char** argv) {
  int batch_size = 60;  //
  int n_test = 200;
  int key_len = 5;    // 32
  int value_len = 5;  // 256, 512, 1024, 2048
  // std::string data_path = "/Users/ldz/Code/miniLETUS/data/";
  // std::string index_path = "/Users/ldz/Code/miniLETUS/";
  // std::string result_path = "/Users/ldz/Code/miniLETUS/exps/results/";
  std::string data_path = "data/";
  std::string index_path = "index";
  std::string result_path = "exps/results/test.csv";

  int opt;
  while ((opt = getopt(argc, argv, "b:n:k:v:d:r:i:")) != -1) {
    switch (opt) {
      case 'b':  // batch size
      {
        char* strtolPtr;
        batch_size = strtoul(optarg, &strtolPtr, 10);
        if ((*optarg == '\0') || (*strtolPtr != '\0') || (batch_size <= 0)) {
          std::cerr << "option -b requires a numeric arg\n" << std::endl;
        }
        break;
      }

      case 'n':  // n test
      {
        char* strtolPtr;
        n_test = strtoul(optarg, &strtolPtr, 10);
        if ((*optarg == '\0') || (*strtolPtr != '\0') || (n_test <= 0)) {
          std::cerr << "option -n requires a numeric arg\n" << std::endl;
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

  // init tasks
  Task* put_tasks = new Task[n_test];
  Task* get_tasks = new Task[n_test];
  for (int i = 0; i < n_test; i++) {
    if (taskGenerator(batch_size, key_len, value_len, i, put_tasks[i],
                      get_tasks[i])) {
      std::cerr << "fail to generate task!" << std::endl;
      return 1;
    }
  }

  // init database
  LSVPS* page_store = new LSVPS(index_path);
  VDLS* value_store = new VDLS(data_path);
  DMMTrie* trie = new DMMTrie(0, page_store, value_store);
  page_store->RegisterTrie(trie);
  ofstream rs_file;
  rs_file.open(result_path, ios::trunc);
  rs_file << "version,op,latency,throughput,pmem(kB),vmem(kB)" << std::endl;
  rs_file.close();
  rs_file.open(result_path, ios::app);

  // start test
  double put_latency_l[n_test];
  double get_latency_l[n_test];
  double put_pmem_l[n_test];
  double put_vmem_l[n_test];
  double wrong_cnt = 0;
  for (int j = 0; j < n_test; j++) {
    // put
    auto keys = put_tasks[j].keys;
    auto values = put_tasks[j].values;
    auto versions = put_tasks[j].versions;
    auto start = chrono::system_clock::now();
    for (int i = 0; i < keys.size(); i++) {
      std::string key = keys[i];
      std::string value = values[i];
      uint64_t version = versions[i];
#ifdef DEBUG
      std::cout << i << " PUT:" << key << "," << value << ", v" << version
                << std::endl;
#endif
      trie->Put(0, version, key, value);
    }
    trie->Commit(j + 1);
    auto end = chrono::system_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
    put_latency_l[j] = double(duration.count()) *
                       chrono::microseconds::period::num /
                       chrono::microseconds::period::den;
    auto [pmem, vmem] = getMemoryUsage();
    put_pmem_l[j] = pmem;
    put_vmem_l[j] = vmem;
    rs_file << j + 1 << ","
            << "put," << put_latency_l[j] << ","
            << batch_size / put_latency_l[j] << "," << pmem << "," << vmem
            << endl;
  }
  // shuffle get_tasks
  // 使用随机数生成器
  // std::random_device rd;
  uint64_t seed = 1882;
  // uint64_t seed =
  // std::chrono::system_clock::now().time_since_epoch().count();
  std::mt19937 g(seed);
  std::shuffle(get_tasks, get_tasks + n_test, g);
  for (int j = 0; j < n_test; j++) {
    // get
    auto keys = get_tasks[j].keys;
    auto values = get_tasks[j].values;
    auto versions = get_tasks[j].versions;
    auto start = chrono::system_clock::now();
    for (int i = 0; i < keys.size(); i++) {
      std::string key = keys[i];
      std::string value = values[i];
      uint64_t version = versions[i];
#ifdef DEBUG
      std::cout << i << " GET:" << key << "," << value << ", v" << version
                << std::endl;
#endif
      // std::string value_2 = trie->Get(0, version, key);
      DMMTrieProof proof = trie->GetProof(0, version, key);
      std::string value_2 = proof.value;
#ifdef DEBUG
      std::cout << "value = " << value_2 << std::endl;
#endif
      if (value != value_2) {
        wrong_cnt += 1;
      }
    }
    auto end = chrono::system_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
    get_latency_l[j] = double(duration.count()) *
                       chrono::microseconds::period::num /
                       chrono::microseconds::period::den;
    rs_file << j + 1 << ","
            << "get," << get_latency_l[j] << ","
            << batch_size / get_latency_l[j] << "," << 0 << "," << 0 << endl;
  }
  rs_file << "---summary---" << endl;
  rs_file << "version,latency,put_latency,get_throughput,put_throughput,"
             "put_pmem(kB),put_vmem(kB)"
          << std::endl;
  double put_latency_sum = 0;
  double get_latency_sum = 0;
  for (int j = 0; j < n_test; j++) {
    rs_file << j + 1 << ",";
    rs_file << get_latency_l[j] << ",";
    rs_file << put_latency_l[j] << ",";
    rs_file << batch_size / get_latency_l[j] << ",";
    rs_file << batch_size / put_latency_l[j] << ",";
    rs_file << put_pmem_l[j] << ",";
    rs_file << put_vmem_l[j] << std::endl;
    get_latency_sum += get_latency_l[j];
    put_latency_sum += put_latency_l[j];
  }
  std::cout << "latency: ";
  std::cout << "put= " << put_latency_sum / n_test << " s, ";
  std::cout << "get= " << get_latency_sum / n_test << " s, ";
  std::cout << std::endl;
  std::cout << "throughput: ";
  std::cout << "put= " << batch_size / (put_latency_sum / n_test) << " ops, ";
  std::cout << "get= " << batch_size / (get_latency_sum / n_test) << " ops, ";
  std::cout << std::endl;
  std::cout << "wrong count = " << wrong_cnt << std::endl;
  rs_file.close();

  return 0;
}