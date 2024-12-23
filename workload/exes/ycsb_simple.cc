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
void ParseCommandLine(int argc, const char *argv[], string &data_path,
                      string &index_path, string &result_path,
                      string &config_path);

bool TransactionRead(DMMTrie *trie, uint64_t version, const string &key,
                     string &value, KVBuffer &unverified_keys) {
#ifdef DEBUG
  cout << "[TransactionRead]";
  cout << "key: " << key << endl;
#endif
  value = trie->Get(0, version, key);
  unverified_keys.Put(version, key, value);
  return true;
}
bool TransactionUpdate(DMMTrie *trie, uint64_t version, const string &key,
                       const string &value, KVBuffer &unverified_keys) {
#ifdef DEBUG
  cout << "[TransactionUpdate]";
  cout << "key: " << key << ",";
  cout << "value: " << value << endl;
#endif
  trie->Put(0, version, key, value);
  unverified_keys.Put(version, key, value);
  return true;
}
bool TransactionInsert(DMMTrie *trie, uint64_t version, const string &key,
                       const string &value, KVBuffer &unverified_keys) {
#ifdef DEBUG
  cout << "[TransactionInsert]";
  cout << "key: " << key << ",";
  cout << "value: " << value << endl;
#endif
  trie->Put(0, version, key, value);
  unverified_keys.Put(version, key, value);
  return true;
}
bool TransactionScan(DMMTrie *trie, uint64_t version, const string &key,
                     uint64_t len, KVBuffer &unverified_keys) {
#ifdef DEBUG
  cout << "[TransactionScan]";
  cout << "key: " << key << ",";
  cout << "len: " << len << endl;
#endif
  for (int i = 0; i < len; i++) {
    string k = to_string(stoul(key) + i);
    string v = trie->Get(0, version, k);
    unverified_keys.Put(version, k, v);
  }
  return true;
}

bool TransactionReadModifyWrite(DMMTrie *trie, uint64_t version,
                                const string &key, string &value,
                                KVBuffer &unverified_keys) {
#ifdef DEBUG
  cout << "[TransactionReadModifyWrite]";
  cout << "key: " << key << ",";
  cout << "value: " << value << endl;
#endif
  string value_read = trie->Get(0, version, key);
  unverified_keys.Put(version, key, value_read);
  trie->Put(0, version + 1, key, value);
  unverified_keys.Put(version + 1, key, value);
  value = value_read;
  return true;
}

uint64_t loading(DMMTrie *trie, Workload *wl) {
  int num_op = wl->GetRecordCount();
  int batch_size = wl->GetBatchSize();
  uint64_t version = 1;
  for (int i = 0; i < num_op; i++) {
    trie->Put(0, version, wl->NextSequenceKey(), wl->NextRandomValue());
    if (i % batch_size == 0) {
      trie->Commit(version);
      version++;
    }
  }
  return version - 1;
}

void transaction(DMMTrie *trie, Workload &wl, uint64_t version,
                 KVBuffer &unverified_keys) {
  int put_count = 0;
  int num_op = wl.GetOperationCount();
  int batch_size = wl.GetBatchSize();
  uint64_t current_version = version;
  bool status;
  uint64_t ver;
  string k, v;
  for (int i = 0; i < num_op; i++) {
    switch (wl.NextOperation()) {
      case READ:
        ver = current_version;
        k = wl.NextTransactionKey();
        v = "";
        status = TransactionRead(trie, ver, k, v, unverified_keys);
        break;
      case UPDATE:
        ver = current_version + 1;
        k = wl.NextTransactionKey();
        v = wl.NextRandomValue();
        status = TransactionUpdate(trie, ver, k, v, unverified_keys);
        put_count += 1;
        break;
      case INSERT:
        ver = current_version + 1;
        k = wl.NextSequenceKey();
        v = wl.NextRandomValue();
        put_count += 1;
        status = TransactionInsert(trie, ver, k, v, unverified_keys);
        break;
      case SCAN:
        ver = current_version;
        k = wl.NextTransactionKey();
        status =
            TransactionScan(trie, ver, k, wl.NextScanLength(), unverified_keys);
        break;
      case READMODIFYWRITE:
        status = TransactionReadModifyWrite(trie, ver, k, v, unverified_keys);
        break;
      default:
        throw utils::Exception("Operation request is not recognized!");
    }
    if (put_count % batch_size == 0) {
      // TODO: unverfied keys is only readable after commit
      current_version++;
      trie->Commit(current_version);
    }
  }
}

void verification(DMMTrie *trie, KVBuffer &unverified_keys) {
  vector<tuple<uint64_t, string, string>> results = unverified_keys.Read();
  for (auto result : results) {
    uint64_t version = get<0>(result);
    auto key = get<1>(result);
    auto value = get<2>(result);
    // string root_hash = trie->GetRootHash(0, version);
    // DMMTrieProof proof = trie->GetProof(0, version, key);
    // bool vs = trie->Verify(0, key, value, root_hash, proof);
    // TODO: verify
    cout << "ver: " << version << ", ";
    cout << "key: " << key << ", ";
    cout << "value: " << value << endl;
  }
}

int main(const int argc, const char *argv[]) {
  uint64_t record_count = 100000;
  uint64_t operation_count = 100000;
  uint64_t batch_size = 60;  // 500, 1000, 2000, 3000, 4000
  uint64_t key_len = 5;      // 32
  uint64_t value_len = 256;  // 256, 512, 1024, 2048
  string data_path = "data/";
  string index_path = ".";
  string result_path = "exps/results/";
  string config_path = "workloads/configures/workloada.spec";
  ParseCommandLine(argc, argv, data_path, index_path, result_path, config_path);

#ifdef DEBUG
  cout << "data_path: " << data_path << endl;
  cout << "index_path: " << index_path << endl;
  cout << "result_path: " << result_path << endl;
  cout << "config_path: " << config_path << endl;
#endif
  utils::Properties props;
  ifstream input(config_path);
  try {
    props.Load(input);
  } catch (const string &message) {
    cout << message << endl;
    exit(0);
  }
  // init database
  LSVPS *page_store = new LSVPS(index_path);
  VDLS *value_store = new VDLS(data_path);
  DMMTrie *trie = new DMMTrie(0, page_store, value_store);
  page_store->RegisterTrie(trie);
  // init workload
  Workload wl(props);

  // init a buffer for unverified keys
  // TODO: make it thread safe
  KVBuffer unverified_keys;
  // loading phase
  uint64_t version = loading(trie, &wl);
  // transaction phase
  // TODO: run this in one thread
  transaction(trie, wl, version, unverified_keys);
  // verification
  // TODO: run this in another thread
  verification(trie, unverified_keys);
}

void ParseCommandLine(int argc, const char *argv[], string &data_path,
                      string &index_path, string &result_path,
                      string &config_path) {
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
    } else {
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