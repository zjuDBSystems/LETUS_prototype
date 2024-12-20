#ifndef _KV_BUFFER_HPP_
#define _KV_BUFFER_HPP_

#include <cstdint>
#include <string>
#include <vector>

class KVBuffer {
 public:
  KVBuffer() : version_i(0), record_i(0) {}
  void ReadBegin() { read_i = 0; }

  std::pair<uint64_t, std::pair<std::string, std::string>> Next() {
    if (version_i >= buffer.size()) {
      return nullptr;
    }
    if (record_i >= buffer[version_i].second.size()) {
      version_i++;
      record_i = 0;
      if (version_i >= buffer.size()) {
        return nullptr;
      }
      if (record_i >= buffer[version_i].second.size()) {
        return nullptr;
      }
    }
    auto version = buffer[version_i].first;
    auto record = buffer[version_i].second[record_i];
    return {version, record};
  }

  void Put(uint64_t version, const std::string& key, const std::string& value) {
    if (buffer.find(version) != buffer.end()) {
      buffer[version][key] = value;
    } else {
      buffer[version] = {{key, value}};
    }
    return;
  }

 private:
  int version_i;
  int record_i;
  // it is a nested list: [(version, [(key, value),(key, value),...]), ...]
  std::map<uint64_t, std::map<string, string>>>> buffer;
  std::vector<std::pair<uint64_t, std::pair<string, string>>> read_list;
};
#endif