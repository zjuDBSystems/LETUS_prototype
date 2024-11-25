#include <fstream>
#include <iostream>
#include <string>

#include <json/json.h>

#include "DMMTrie.hpp"
#include "LSVPS.hpp"

using namespace std;

int main(int argc, char** argv) {
  LSVPS* page_store = new LSVPS();
  VDLS* value_store = new VDLS();
  DMMTrie* trie = new DMMTrie(0, page_store, value_store);
  page_store->RegisterTrie(trie);

  string line;
  ifstream trace("/home/xinyu.chen/LETUS_prototype/workload/traces/log");
  if (!trace) {
    std::cerr << "failed to open trace file" << std::endl;
    return 1;
  }
  while (std::getline(trace, line)) {  // 逐行读取文件内容
    Json::Reader reader;
    Json::Value value;
    if (reader.parse(line, value)) {
      string op = value["op"].asString();
      cout << op << endl;
      if (op.compare("BatchGet") == 0) {
        auto keys = value["keys"];
        for (int i = 0; i < keys.size(); i++) {
          string key = keys[i].asString();
          trie->Get(0, 1, key);
        }
      } else if (op.compare("put") == 0) {
        auto keys = value["keys"];
        auto values = value["values"];
        for (int i = 0; i < keys.size(); i++) {
          string key = keys[i].asString();
          string value = values[i].asString();
          trie->Put(0, 1, key, value);
        }
      }
    }
  }
  return 0;
}