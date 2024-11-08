#include <iostream>
#include <fstream>
#include <string>

#include <json/json.h>

using namespace std;


int main(int argc, char **argv) {
    string line;
    ifstream trace("/home/xinyu.chen/LETUS_prototype/workload/traces/log");
    if (!trace) {
        std::cerr << "无法打开文件" << std::endl;
        return 1;
    } 
    while (std::getline(trace, line)) { // 逐行读取文件内容
        // std::cout << line << std::endl;
        Json::Reader reader;
	    Json::Value value;
        if(reader.parse(line, value))
        {
            cout << value["op"].asString() << endl;
        }
    }
    return 0;
}