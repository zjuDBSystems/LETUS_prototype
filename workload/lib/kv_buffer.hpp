#ifndef _KV_BUFFER_HPP_
#define _KV_BUFFER_HPP_

#include <cstdint>
#include <string>
#include <vector>

#include <tuple>

class KVBuffer {
 public:
  KVBuffer() {}

  std::vector<std::tuple<uint64_t, std::string, std::string>> Read() {
    std::vector<std::tuple<uint64_t, std::string, std::string>> results;
    for (auto& version_block : buffer) {
      for (auto& kv : version_block.second) {
        results.push_back(
            std::make_tuple(version_block.first, kv.first, kv.second));
      }
    }
    return results;
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
  // it is a nested list: [(version, [(key, value),(key, value),...]), ...]
  std::map<uint64_t, std::map<string, string>> buffer;
};
#endif