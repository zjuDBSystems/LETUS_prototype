#ifndef _VDLS_HPP_
#define _VDLS_HPP_

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <tuple>
#include <sstream>

using namespace std;

class VDLS {
public:
    VDLS() : current_fileID_(0), current_offset_(0) {}

    tuple<uint64_t, uint64_t, uint64_t> WriteValue(uint64_t version, const string& key, const string& value) {   // append a new record to the log stream
        size_t record_size = sizeof(version) + key.size() + value.size() + 4;
        if (current_offset_ + record_size > MaxFileSize) { 
            current_fileID_++;
            current_offset_ = 0;
        }

        ofstream file("data_file_" + to_string(current_fileID_), ios::app);// create or open the file for appending
        if (!file) {
            cerr << "Error : can't open file " << endl;
            return;
        }

        file << "(" << version << "," << key << "," << value << ")" << endl;

        tuple<uint64_t, uint64_t, uint64_t> location(current_fileID_, current_offset_, record_size);  // record location as a tuple (fileID, offset, size)

        current_offset_ += record_size;   // update current offset

        file.close();

        return location;
    }

    string ReadValue(const tuple<uint64_t, uint64_t, uint64_t>& location) {
        uint64_t fileID, offset, size;
        tie(fileID, offset, size) = location;

        ifstream file("data_file_" + to_string(fileID));  // open the file where the record is located
        if (!file) {
            cerr << "Error opening file for reading" << endl;
            return "";
        }
        
        file.seekg(offset, ios::beg);  // go to the correct offset

        string line;
        getline(file, line);
        file.close();

        stringstream ss(line);
        string temp, value;

        getline(ss, temp, ',');    //skip to the first ","
        getline(ss, temp, ',');    //skip to the second ","
        getline(ss, value);    // remaining part of the line is the value

        return value;
    }

private:
    uint64_t current_fileID_;
    uint64_t current_offset_;
    const uint64_t MaxFileSize = 64 * 1024 * 1024; // 64MB
};

#endif