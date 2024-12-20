/**
 * A very simplified YCSB workload
 *  - only have one table, the table only have one field/column
 */

#include <fstream>
#include <iostream>
#include <string>
#include <utility>

#include <json/json.h>

#include "DMMTrie.hpp"
#include "LSVPS.hpp"
#include "kv_buffer.hpp"
#include "properties.hpp"
#include "workload.hpp"
#include "workload_utilis.hpp"
using namespace std;

void UsageMessage(const char *command);
bool StrStartWith(const char *str, const char *pre);
void ParseCommandLine(int argc, const char *argv[], std::string &data_path,
                      std::string &index_path, std::string &result_path,
                      std::string &config_path);

TransactionRead uint64_t loading(DMMTrie *trie, Workload *wl) {
  int num_op = wl->GetRecordCount();
  int batch_size = wl->GetBatchSize();
  uint64_t version = 0;
  for (int i = 0; i < num_op; i++) {
    trie->Put(0, version, wl->NextSequenceKey(), wl->NextRandomValue());
    if (i % batch_size == 0) {
      version++;
      trie->Commit(version);
    }
  }
  return version;
}

void transaction(DMMTrie *trie, Workload &wl, uint64_t version,
                 KVBuffer &unverified_keys;) {
  int num_op = wl->GetOperationCount();
  int batch_size = wl->GetBatchSize();
  uint64_t current_version = version;
  for (int i = 0; i < num_op; i++) {
    switch (wl.NextOperation()) {
      case READ:
        uint64_t ver = current_version;
        std::string k = wl.NextTransactionKey();
        std::string v;
        status = TransactionRead(trie, ver, k, &v);
        unverified_keys.put(ver, k, v);
        break;
      case UPDATE:
        uint64_t ver = current_version + 1;
        std::string k = wl.NextTransactionKey();
        std::string v = wl.NextRandomValue();
        status = TransactionUpdate(trie, ver, k, v);
        unverified_keys.put(ver, k, v);
        break;
      case INSERT:
        ver = current_version + 1;
        std::string k = wl.NextSequenceKey();
        std::string v = wl.NextRandomValue();
        status = TransactionInsert(trie, ver, k, v);
        unverified_keys.put(ver, k, v);
        break;
      case SCAN:
        ver = current_version;
        status = TransactionScan(ver, k, wl.NextScanLength());
        break;
      case READMODIFYWRITE:
        status = TransactionReadModifyWrite();
        break;
      default:
        throw utils::Exception("Operation request is not recognized!");
    }
    if (i % batch_size == 0) {
      current_version++;
      trie->Commit(version);
    }
  }
  return version;
}

void verification(DMMTrie *trie, KVBuffer &unverified_keys) {
  // TODO
}

int main(const int argc, const char *argv[]) {
  uint64_t record_count = 100000;
  uint64_t operation_count = 100000;
  uint64_t batch_size = 60;  // 500, 1000, 2000, 3000, 4000
  uint64_t key_len = 5;      // 32
  uint64_t value_len = 256;  // 256, 512, 1024, 2048
  std::string data_path = "data/";
  std::string index_path = ".";
  std::string result_path = "exps/results/";
  std::string config_path = "workloads/configures/workloada.spec";
  ParseCommandLine(argc, argv, data_path, index_path, result_path, config_path);

  utils::Properties props;
  ifstream input(config_path);
  try {
    props.Load(input);
  } catch (const string &message) {
    cout << message << endl;
    exit(0);
  }
#ifdef DEBUG
  std::cout << "data_path: " << data_path << std::endl;
  std::cout << "index_path: " << index_path << std::endl;
  std::cout << "result_path: " << result_path << std::endl;
  std::cout << "config_path: " << config_path << std::endl;
#endif
  // init database
  LSVPS *page_store = new LSVPS(index_path);
  VDLS *value_store = new VDLS(data_path);
  DMMTrie *trie = new DMMTrie(0, page_store, value_store);
  page_store->RegisterTrie(trie);
  // init workload
  Workload wl(props);

  // init a buffer for unverified keys
  KVBuffer unverified_keys;
  // loading phase
  uint64_t version = loading(trie, &wl);
  // transaction phase
  transaction(trie, &wl, version, unverified_keys);
  // verification
  verification(trie, unverified_keys);
}

void ParseCommandLine(int argc, const char *argv[], std::string &data_path,
                      std::string &index_path, std::string &result_path,
                      std::string &config_path) {
  int argindex = 1;
  while (argindex < argc && StrStartWith(argv[argindex], "-")) {
    if (strcmp(argv[argindex], "-datapath") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      data_path = argv[argindex];
      argindex++;
    } else if (strcmp(argv[argindex], "-idxpath") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      index_path = argv[argindex];
      argindex++;
    } else if (strcmp(argv[argindex], "-respath") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      result_path = argv[argindex];
      argindex++;
    } else if (strcmp(argv[argindex], "-confpath") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      config_path = argv[argindex];
      argindex++;
    }
    // else if (strcmp(argv[argindex], "-rcnt") == 0) {
    //   argindex++;
    //   if (argindex >= argc) {
    //     UsageMessage(argv[0]);
    //     exit(0);
    //   }
    //   char *end;
    //   record_count =
    //       strtoul(argv[argindex], &end, 10);    // 使用 strtoul 进行转换
    //   if (*end != '\0' || record_count <= 0) {  // 检查转换是否成功
    //     UsageMessage(argv[0]);
    //     exit(0);
    //   }
    //   argindex++;
    // }
    else {
      cout << "Unknown option '" << argv[argindex] << "'" << endl;
      exit(0);
    }
  }

  if (argindex == 1 || argindex != argc) {
    UsageMessage(argv[0]);
    exit(0);
  }
}

void UsageMessage(const char *command) {
  cout << "Usage: " << command << " [options]" << endl;
  cout << "options:" << endl;
  cout << "  -datapath: directory where data is stored" << endl;
  cout << "  -idxpath: directory where index is stored" << endl;
  cout << "  -respath: directory where result is stored" << endl;
  cout << "  -confpath: path of configure file" << endl;
}

bool StrStartWith(const char *str, const char *pre) {
  return strncmp(str, pre, strlen(pre)) == 0;
}