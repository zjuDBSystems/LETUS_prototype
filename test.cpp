#include "lib/VDLS.hpp"
#include <iostream>

// #include <openssl/evp.h>
// #include <openssl/sha.h>
// #include <string>
// #include <iomanip>
// #include <sstream>

// using namespace std;

// string HashFunction(const string &input) {
//     // 创建 SHA-256 上下文
//     EVP_MD_CTX *ctx = EVP_MD_CTX_new();
//     if (ctx == nullptr) {
//         throw runtime_error("Failed to create EVP_MD_CTX");
//     }

//     // 初始化 SHA-256 哈希计算
//     if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
//         EVP_MD_CTX_free(ctx);
//         throw runtime_error("Failed to initialize SHA-256");
//     }

//     // 更新哈希计算
//     if (EVP_DigestUpdate(ctx, input.c_str(), input.size()) != 1) {
//         EVP_MD_CTX_free(ctx);
//         throw runtime_error("Failed to update SHA-256");
//     }

//     // 最终计算并获得结果
//     unsigned char hash[EVP_MAX_MD_SIZE];
//     unsigned int hash_len = 0;
//     if (EVP_DigestFinal_ex(ctx, hash, &hash_len) != 1) {
//         EVP_MD_CTX_free(ctx);
//         throw runtime_error("Failed to finalize SHA-256");
//     }

//     // 释放上下文
//     EVP_MD_CTX_free(ctx);

//     // 将字节数组转换为十六进制字符串
//     stringstream ss;
//     for (unsigned int i = 0; i < hash_len; i++) {
//         ss << hex << setw(2) << setfill('0') << (int)hash[i];
//     }

//     return ss.str();
// }


// int main() {
//     cout << HashFunction("alice")<<endl;
//     cout << HashFunction("123456")<<endl;
// }

int main() {
    // VDLS vdls;

    // tuple<uint64_t, uint64_t, uint64_t> location = vdls.WriteValue(1, "alice", "50");
    // cout << get<0>(location) << " "<< get<1>(location) << " " << get<2>(location) << endl;
    // cout << vdls.ReadValue(location) <<endl;

    // for(int i = 0;i < 6000;i++){
    //     vdls.WriteValue(1, "alice", "75");
    // }

    // tuple<uint64_t, uint64_t, uint64_t> location2 = vdls.WriteValue(2, "alice", "100");
    // cout << get<0>(location2) << " "<< get<1>(location2) << " " << get<2>(location2) << endl;
    // cout << vdls.ReadValue(location2)<<endl;
    for(int i = 1;i <= 50; i++) {
        cout << "trie->Put(0," << i << ",\"12345\",\"" << i << "\");" << endl;
    }
}