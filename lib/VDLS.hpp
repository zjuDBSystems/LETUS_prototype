#ifndef _VDLS_HPP_
#define _VDLS_HPP_

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

using namespace std;

// log stream format: fileID,offset,size\n
class VDLS {
 public:
  // VDLS(string file_path = "/mnt/c/Users/qyf/Desktop/LETUS_prototype/data/")
  //     : current_fileID_(0),
  //       current_offset_(0),
  //       file_path_(file_path),
  //       buffer_(""),
  //       BufferSize(0),
  //       buffer_fileID_(-1),
  //       buffer_offset_(-1) {}

  VDLS(string file_path = "/mnt/c/Users/qyf/Desktop/LETUS_prototype/data/")
      : current_fileID_(0),
        current_offset_(0),
        file_path_(file_path),
        write_map_(MAP_FAILED),
        read_map_(MAP_FAILED),
        read_map_fileID_(-1) {
    OpenAndMapWriteFile();
  }

  tuple<uint64_t, uint64_t, uint64_t> WriteValue(uint64_t version,
                                                 const string& key,
                                                 const string& value) {
    string record = to_string(version) + "," + key + "," + value + "\n";
    size_t record_size = record.size();

    // 检查是否需要创建新文件
    if (current_offset_ + record_size > MaxFileSize) {
      // 同步更改到磁盘
      if (msync(write_map_, MaxFileSize, MS_SYNC) == -1) {
        throw runtime_error("Failed to sync changes to disk");
      }
      // 解除旧的映射
      if (write_map_ != MAP_FAILED) {
        munmap(write_map_, MaxFileSize);
      }

      // 创建新文件 ID 和重置偏移量
      current_fileID_++;
      current_offset_ = 0;

      // 重新打开和映射新文件
      OpenAndMapWriteFile();
    }

    // 写入新记录到写映射区域
    memcpy(static_cast<char*>(write_map_) + current_offset_, record.c_str(),
           record_size);

    // 同步更改到磁盘
    // if (msync(write_map_, MaxFileSize, MS_SYNC) == -1) {
    //   throw runtime_error("Failed to sync changes to disk");
    // }

    current_offset_ += record_size;

    return make_tuple(current_fileID_, current_offset_ - record_size,
                      record_size);
  }

  tuple<uint64_t, uint64_t, uint64_t> WriteValueV1(
      uint64_t version, const string& key,
      const string& value) {  // append a new record to the log stream
    string version_str = to_string(version);
    size_t record_size = version_str.size() + key.size() + value.size() + 3;
    if (current_offset_ + record_size > MaxFileSize) {
      current_fileID_++;
      current_offset_ = 0;
    }

    ofstream file(
        file_path_ + "data_file_" + to_string(current_fileID_) + ".dat",
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

  // vector<tuple<uint64_t, uint64_t, uint64_t>> WriteValues(
  //     uint64_t version, const vector<string>& key,
  //     const vector<string>& value) {  // append a new record to the log
  //     stream
  //   string version_str = to_string(version);
  //   size_t record_size =
  //       version_str.size() + key[0].size() + value[0].size() + 3;
  //   if (current_offset_ + record_size * key.size() > MaxFileSize) {
  //     current_fileID_++;
  //     current_offset_ = 0;
  //   }

  //   ofstream file(
  //       file_path_ + "data_file_" + to_string(current_fileID_) + ".dat",
  //       ios::app);  // create or open the file for appending
  //   if (!file) {
  //     throw std::runtime_error("Cannot open file ");
  //   }

  //   string content = "";
  //   vector<tuple<uint64_t, uint64_t, uint64_t>> locations;
  //   for (int i = 0; i < key.size(); i++) {
  //     content += version_str + "," + key[i] + "," + value[i] + "\n";
  //     current_offset_ += record_size;
  //     locations.push_back(
  //         make_tuple(current_fileID_, current_offset_, record_size));
  //   }

  //   file << content;

  //   file.close();

  //   return locations;
  // }

  string ReadValue(const tuple<uint64_t, uint64_t, uint64_t>& location) {
    uint64_t fileID, offset, size;
    tie(fileID, offset, size) = location;

    // 检查是否需要重新映射文件
    if (fileID != read_map_fileID_) {
      if (read_map_ != MAP_FAILED) {
        munmap(read_map_, MaxFileSize);
        read_map_ = MAP_FAILED;
      }
      OpenAndMapReadFile(fileID);
      read_map_fileID_ = fileID;
    }

    // 从映射区域读取数据
    string line(static_cast<char*>(read_map_) + offset, size);

    stringstream ss(line);
    string temp, value;

    getline(ss, temp, ',');  // 版本号
    getline(ss, temp, ',');  // 键
    getline(ss, value);      // 值

    return value;
  }

  string ReadValueV1(const tuple<uint64_t, uint64_t, uint64_t>& location) {
    uint64_t fileID, offset, size;
    tie(fileID, offset, size) = location;

    ifstream file(file_path_ + "data_file_" + to_string(fileID) + ".dat");
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

  // string ReadValueFromBuffer(
  //     const tuple<uint64_t, uint64_t, uint64_t>& location) {
  //   cout << "#";
  //   uint64_t fileID, offset, size;
  //   tie(fileID, offset, size) = location;
  //   if (fileID == buffer_fileID_ && offset >= buffer_offset_ &&
  //       offset < buffer_offset_ + BufferSize) {  // hit the buffer
  //     string record = buffer_.substr(offset - buffer_offset_, size);
  //     stringstream ss(record);
  //     string temp, value;
  //     getline(ss, temp, ',');
  //     getline(ss, temp, ',');
  //     getline(ss, value);
  //     return value;
  //   }

  //   ifstream file(file_path_ + "data_file_" + to_string(fileID) + ".dat");
  //   if (!file) {
  //     throw runtime_error("Cannot open file");
  //   }

  //   file.seekg(0, file.end);
  //   uint64_t length = (uint64_t)file.tellg();
  //   uint64_t read_size = min(MaxBufferSize, length - offset);
  //   file.seekg(offset, ios::beg);  // locate the correct offset
  //   buffer_ = string(read_size, '\0');
  //   file.read(&buffer_[0], read_size);  // read a buffer of data
  //   buffer_fileID_ = fileID;
  //   buffer_offset_ = offset;
  //   BufferSize = read_size;

  //   file.close();
  //   string record = buffer_.substr(0, size);

  //   stringstream ss(record);
  //   string temp, value;
  //   getline(ss, temp, ',');
  //   getline(ss, temp, ',');
  //   getline(ss, value);

  //   return value;
  // }

 private:
  string file_path_;
  uint64_t current_fileID_;
  uint64_t current_offset_;
  const uint64_t MaxFileSize = 64 * 1024 * 1024;  // 64MB
  // string buffer_;
  // int64_t buffer_fileID_;
  // int64_t buffer_offset_;
  // const uint64_t MaxBufferSize = 12 * 1024 * 1024;  // 12MB
  // uint64_t BufferSize;
  void* write_map_;
  void* read_map_;
  int64_t read_map_fileID_;

  void OpenAndMapWriteFile() {
    string filename =
        file_path_ + "data_file_" + to_string(current_fileID_) + ".dat";

    // 打开或创建文件
    int fd = open(filename.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1) {
      throw runtime_error("Cannot open or create file: " + filename);
    }

    // 确保文件至少有 MaxFileSize 大小
    ftruncate(fd, MaxFileSize);

    // 内存映射文件为写映射区域
    write_map_ =
        mmap(nullptr, MaxFileSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (write_map_ == MAP_FAILED) {
      close(fd);
      throw runtime_error("Memory map for writing failed: " + filename);
    }

    // 关闭文件描述符，因为已经映射了文件
    close(fd);
  }

  void OpenAndMapReadFile(uint64_t fileID) {
    string filename = file_path_ + "data_file_" + to_string(fileID) + ".dat";

    // 打开文件
    int fd = open(filename.c_str(), O_RDONLY);
    if (fd == -1) {
      throw runtime_error("Cannot open file for reading: " + filename);
    }

    // // 获取文件大小
    // struct stat sb;
    // if (fstat(fd, &sb) == -1) {
    //     close(fd);
    //     throw runtime_error("Cannot get file size: " + filename);
    // }
    // // 内存映射文件为读映射区域，根据实际文件大小映射
    // read_map_ = mmap(nullptr, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);

    // 直接映射MaxFileSize文件大小
    read_map_ = mmap(nullptr, MaxFileSize, PROT_READ, MAP_SHARED, fd, 0);
    if (read_map_ == MAP_FAILED) {
      close(fd);
      throw runtime_error("Memory map for reading failed: " + filename);
    }

    // 关闭文件描述符，因为已经映射了文件
    close(fd);
  }
};

#endif