#ifndef _VDLS_HPP_
#define _VDLS_HPP_

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

using namespace std;

// log stream format: fileID,offset,size\n
class VDLS {
 public:
  VDLS(string file_path =
           "/mnt/c/Users/qyf/Desktop/LETUS_prototype/data/data_file_")
      : current_fileID_(0), current_offset_(0), file_path_(file_path) {}

  tuple<uint64_t, uint64_t, uint64_t> WriteValue(
      uint64_t version, const string& key,
      const string& value) {  // append a new record to the log stream
    string version_str = to_string(version);
    size_t record_size = version_str.size() + key.size() + value.size() + 3;
    if (current_offset_ + record_size > MaxFileSize) {
      current_fileID_++;
      current_offset_ = 0;
    }

    ofstream file(file_path_ + to_string(current_fileID_) + ".dat",
                  ios::app);  // create or open the file for appending
    if (!file) {
      throw std::runtime_error("Cannot open file ");
    }

    file << version_str << "," << key << "," << value << "\n";

    tuple<uint64_t, uint64_t, uint64_t> location(current_fileID_,
                                                 current_offset_, record_size);

    current_offset_ += record_size;  // update current offset

    file.close();

    return location;
  }

  string ReadValue(const tuple<uint64_t, uint64_t, uint64_t>& location) {
    uint64_t fileID, offset, size;
    tie(fileID, offset, size) = location;

    ifstream file(file_path_ + to_string(fileID) + ".dat");
    if (!file) {
      throw runtime_error("Cannot open file");
    }

    file.seekg(offset, ios::beg);  // locate the correct offset

    string line;
    getline(file, line);

    stringstream ss(line);
    string temp, value;

    getline(ss, temp, ',');
    getline(ss, temp, ',');
    getline(ss, value);

    file.close();

    return value;
  }

 private:
  string file_path_;
  uint64_t current_fileID_;
  uint64_t current_offset_;
  const uint64_t MaxFileSize = 64 * 1024 * 1024;  // 64MB
};

#endif