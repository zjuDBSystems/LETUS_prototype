// class IndexNode : public Node {
// public:
//     struct Slot {
//         uint64_t version;  // Vc, version of child 
//         string hash;  // Hc, merkle hash of child
//         Node* pointer;  // pointer to child
        
//         Slot(uint64_t v = 0, const string& h = "", Node* ptr = nullptr)
//             : version(v), hash(h) ,pointer(ptr){}
//     };
    
//     IndexNode(uint64_t V = 0, const string& h = "", uint16_t b = 0, char n = '\0') 
//         : version_(V),hash_(h), bitmap_(b), nibble_(n) {
//         for (int i = 0; i < DMM_NODE_FANOUT; ++i) {
//             children_[i] = Slot();
//         }
//     }

//     void CalculateHash() override {
//         string concatenated_hash;
//         for (int i = 0; i < DMM_NODE_FANOUT; ++i) {
//             if (children_[i].pointer_ != nullptr) {
//                 concatenated_hash += children_[i].hash;
//             }
//         }
//         hash = HashFunction(concatenated_hash);
//     }

//     /* serialized index node format (size in bytes):
//        | version (8) | hash (32) | bitmap (2) | Vc (8) | Hc (32) | ...
//        | child 1 | child 2 | ...
//        doesn't serialize pointer
//     */
//     void SerializeTo(char* buffer, size_t& current_size, bool is_root) const override {
//         memcpy(buffer + current_size, &version_, sizeof(uint64_t));
//         current_size += sizeof(uint64_t);

//         memcpy(buffer + current_size, hash_.c_str(), hash_.size());
//         current_size += hash_.size();

//         memcpy(buffer + current_size, &bitmap_, sizeof(uint16_t));
//         current_size += sizeof(uint16_t);

//         for(int i = 0; i < DMM_NODE_FANOUT; i++) {
//             memcpy(buffer + current_size, &children_[i].version, sizeof(uint64_t));
//             current_size += sizeof(uint64_t);

//             memcpy(buffer + current_size, children_[i].hash.c_str(), children_[i].hash.size());
//             current_size += children_[i].hash.size();
//         }

//         if (!is_root)  // if an index node is not the root node of a page, do not serialize its children
//             return

//         for(int i = 0; i < DMM_NODE_FANOUT; i++) {
//             children_[i].pointer->SerializeTo(buffer, current_size, false);
//         }
//     }

//     void AddChild(int index, Node* child, uint64_t version, const string& hash) {
//         if (index >= 0 && index < DMM_NODE_FANOUT) {
//             children_[index] = Slot(child, version, hash);
//             bitmap_ |= (1 << index);  // update bitmap
//         }
//     }

//     Slot* GetChild(int index) {
//         if (index >= 0 && index < DMM_NODE_FANOUT) {
//             return &children_[index];
//         }
//         return nullptr;
//     }

// private:
//     constexpr size_t DMM_NODE_FANOUT = 10;
//     Slot children_[DMM_NODE_FANOUT];  // trie
//     uint64_t version_;
//     string hash_;
//     uint16_t bitmap_;    // bitmap for children
//     char nibble_;  // we use a character from key as nibble
// };



//     /* serialized BasePage format (size in bytes):
//        | tp (1) | pid_size (8 in 64-bit system) | pid (pid_size) | current version (8)| latest basepage version (8) |
//        | deltapage update count (2) | basepage update count (2) | root node |
//     */

// /* serialized index node format (size in bytes):
//        | is_leaf_node (1) | version (8) | hash (32) | bitmap (2) | Vc (8) | Hc (32) | Vc (8) | Hc (32) | ...
//        | child 1 | child 2 | ...
//        the function doesn't serialize pointer and doesn't serialize empty child nodes
//     */

//    /* serialized leaf node format (size in bytes):
//        | is_leaf_node (1) | version (8) | key_size (8 in 64-bit system) | key (key_size) | location(8, 8, 8) | hash (32) | 
//     */


// #include <string>
// #include <vector>

// static constexpr int PAGE_SIZE = 12288;                  // size of a data page in byte 12KB two-level subtree
// //KeyData (version, key, value)
// class Page {
// private:
//   char data_[PAGE_SIZE]{};
//   int latestBasePageVersion;//used to obtain the starting BasePage
//   int currentDataVersion;//used to obtain the latest DeltaPage
// };

// struct PageKey { //key to page
//     int version;//64-bit integer
//     int Tid;// ID of the DMM-Trie
//     bool t_P;// delta or base page
//     string Pid;//nibble e.g. 'Al' (in 'Alice')
// };


// class LSVPS {
// public:
//   Page PageQuery(int version){}
//   Page LoadPage(PageKey Kp){
//     PageLookUp(PageKey Kp);
//     //put pages(incluing base and delta) in a vector
//     PageReplay(std::vector<Page> pages);
//   }
//   void StorePage(Page page){} //generate page to store in the MemIndexTable

// private:
//   Page PageLookup(PageKey Kp){}
//   void PageReplay(std::vector<Page> pages){}
//   class blockCache{ //expedite page loading

//   };
//   class memIndexTable{ //buffer the pages MaxSize: 256MB
//     void flush();//flush the memIndexTable to the disk
//   };
//   class BTreeIndex{
//   };
//   blockCache cache;
//   memIndexTable table;
// }




//         // size_t current_size = strlen(deltapage);

//         // memcpy(deltapage + current_size, &version, sizeof(uint64_t));  // add version to the end of deltapage
//         // current_size += sizeof(uint64_t);
//         // memcpy(deltapage + current_size, &get<0>(location), sizeof(uint64_t));  // fileID
//         // current_size += sizeof(uint64_t);
//         // memcpy(deltapage + current_size, &get<1>(location), sizeof(uint64_t));  // offset
//         // current_size += sizeof(uint64_t);
//         // memcpy(deltapage + current_size, &get<2>(location), sizeof(uint64_t));  // size
//         // current_size += sizeof(uint64_t);

//         // string hash = static_cast<LeafNode*>(root_)->GetHash();
//         // memcpy(deltapage + current_size, hash.c_str(), hash.size());
//         // current_size += hash.size();

//         // deltapage[current_size] = '\0';



// #define HASH_SIZE 32 // SHA256 result size in bytes

// string HashFunction(const string &input) {  // hash function SHA-256
//   unsigned char hash[HASH_SIZE];
//   SHA256_CTX sha256_ctx;
//   SHA256_Init(&sha256_ctx);
//   SHA256_Update(&sha256_ctx, input.c_str(), input.size());
//   SHA256_Final(hash, &sha256_ctx);

//   string hash_str;
//   for (int i = 0; i < HASH_SIZE; i++) {
//       hash_str += (char)hash[i];
//   }
//   return hash_str;
// }




//     DMMTriePage(char *buffer) {
//     size_t current_size = 0;

//     uint64_t version = *(reinterpret_cast<uint64_t *>(buffer + current_size));  // deserialize version
//     current_size += sizeof(uint64_t);

//     uint64_t tid = *(reinterpret_cast<uint64_t *>(buffer + current_size));  // deserialize DMMTrie id
//     current_size += sizeof(uint64_t);

//     bool page_type = *(reinterpret_cast<bool *>(buffer + current_size));  // deserialize page type (1 byte)
//     current_size += sizeof(bool);

//     size_t pid_size =
//         *(reinterpret_cast<size_t *>(buffer + current_size));  // deserialize pid_size (8 bytes for size_t)
//     current_size += sizeof(pid_size);
//     pid_ = string(buffer + current_size, pid_size);  // deserialize pid (pid_size bytes)
//     current_size += pid_size;

//     d_update_count_ = *(reinterpret_cast<uint16_t *>(buffer + current_size));  // deserialize d_update_count (2 bytes)
//     current_size += sizeof(uint16_t);
//     b_update_count_ = *(reinterpret_cast<uint16_t *>(buffer + current_size));  // deserialize b_update_count (2 bytes)
//     current_size += sizeof(uint16_t);

//     bool is_leaf_node = *(reinterpret_cast<bool *>(buffer + current_size));
//     current_size += sizeof(bool);

//     if (is_leaf_node) {  // the root node of page is leafnode
//       uint64_t node_version = *(reinterpret_cast<uint64_t *>(buffer + current_size));  // deserialize leafnode version
//       current_size += sizeof(uint64_t);

//       size_t key_size = *(reinterpret_cast<size_t *>(buffer + current_size));  // deserialize key_size
//       current_size += sizeof(key_size);
//       string key(buffer + current_size, key_size);  // deserialize key
//       current_size += key_size;

//       uint64_t fileID = *(reinterpret_cast<uint64_t *>(buffer + current_size));  // deserialize fileID
//       current_size += sizeof(uint64_t);
//       uint64_t offset = *(reinterpret_cast<uint64_t *>(buffer + current_size));  // deserialize offset
//       current_size += sizeof(uint64_t);
//       uint64_t size = *(reinterpret_cast<uint64_t *>(buffer + current_size));  // deserialize size
//       current_size += sizeof(uint64_t);
//       tuple<uint64_t, uint64_t, uint64_t> location = make_tuple(fileID, offset, size);

//       string hash(buffer + current_size, HASH_SIZE);  // deserialize hash
//       current_size += HASH_SIZE;

//       root_ = new LeafNode(node_version, key, location, hash);
//     } else {  // the root node of page is indexnode
//       uint64_t node_version = *(reinterpret_cast<uint64_t *>(buffer + current_size));
//       current_size += sizeof(uint64_t);

//       string hash(buffer + current_size, HASH_SIZE);
//       current_size += HASH_SIZE;

//       uint16_t bitmap = *(reinterpret_cast<uint16_t *>(buffer + current_size));
//       current_size += sizeof(uint16_t);

//       array<pair<uint64_t, string>, DMM_NODE_FANOUT> children;
//       for (int i = 0; i < DMM_NODE_FANOUT; i++) {
//         uint64_t child_version = *(reinterpret_cast<uint64_t *>(buffer + current_size));
//         current_size += sizeof(uint64_t);
//         string child_hash(buffer + current_size, HASH_SIZE);
//         current_size += HASH_SIZE;

//         children[i] = make_pair(child_version, child_hash);
//       }

//       root_ = new IndexNode(node_version, hash, bitmap);

//       bool child_is_leaf_node = *(reinterpret_cast<bool *>(buffer + current_size));
//       if (child_is_leaf_node) {  // second level of page is leafnode
//         for (int i = 0; i < DMM_NODE_FANOUT; i++) {
//           if (bitmap & (1 << i)) {  // serialized data only stores children that exists
//             bool is_leaf_node = *(reinterpret_cast<bool *>(buffer + current_size));
//             current_size += sizeof(bool);

//             uint64_t node_version_2 =
//                 *(reinterpret_cast<uint64_t *>(buffer + current_size));  // deserialize leafnode version
//             current_size += sizeof(uint64_t);

//             size_t key_size_2 = *(reinterpret_cast<size_t *>(buffer + current_size));  // deserialize key_size
//             current_size += sizeof(key_size_2);
//             string key_2(buffer + current_size, key_size_2);  // deserialize key
//             current_size += key_size_2;

//             uint64_t fileID_2 = *(reinterpret_cast<uint64_t *>(buffer + current_size));  // deserialize fileID
//             current_size += sizeof(uint64_t);
//             uint64_t offset_2 = *(reinterpret_cast<uint64_t *>(buffer + current_size));  // deserialize offset
//             current_size += sizeof(uint64_t);
//             uint64_t size_2 = *(reinterpret_cast<uint64_t *>(buffer + current_size));  // deserialize size
//             current_size += sizeof(uint64_t);
//             tuple<uint64_t, uint64_t, uint64_t> location_2 = make_tuple(fileID_2, offset_2, size_2);

//             string hash_2(buffer + current_size, HASH_SIZE);  // deserialize hash
//             current_size += HASH_SIZE;

//             Node *child = new LeafNode(node_version_2, key_2, location_2, hash_2);

//             root_->AddChild(i, children[i].first, children[i].second, child);  // add child to root
//           }
//         }
//       } else {  // second level of page is indexnode
//         for (int i = 0; i < DMM_NODE_FANOUT; i++) {
//           if (bitmap & (1 << i)) {
//             bool is_leaf_node = *(reinterpret_cast<bool *>(buffer + current_size));
//             current_size += sizeof(bool);

//             uint64_t node_version_2 = *(reinterpret_cast<uint64_t *>(buffer + current_size));
//             current_size += sizeof(uint64_t);

//             string hash_2(buffer + current_size, HASH_SIZE);
//             current_size += HASH_SIZE;

//             uint16_t bitmap_2 = *(reinterpret_cast<uint16_t *>(buffer + current_size));
//             current_size += sizeof(uint16_t);

//             Node *child = new IndexNode(node_version_2, hash_2, bitmap_2);

//             for (int j = 0; j < DMM_NODE_FANOUT; j++) {
//               uint64_t child_version_2 = *(reinterpret_cast<uint64_t *>(buffer + current_size));
//               current_size += sizeof(uint64_t);
//               string child_hash_2(buffer + current_size, HASH_SIZE);
//               current_size += HASH_SIZE;

//               child->AddChild(j, child_version_2, child_hash_2,
//                               nullptr);  // add child to second level indexnode in page
//             }

//             root_->AddChild(i, children[i].first, children[i].second,
//                             child);  // add child to fist level indexnode in page
//           }
//         }
//       }
//     }
//   }





#include <openssl/evp.h>
#include <openssl/sha.h>
#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>

using namespace std;

string HashFunction(const string &input) {
    // 创建 SHA-256 上下文
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (ctx == nullptr) {
        throw runtime_error("Failed to create EVP_MD_CTX");
    }

    // 初始化 SHA-256 哈希计算
    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        throw runtime_error("Failed to initialize SHA-256");
    }

    // 更新哈希计算
    if (EVP_DigestUpdate(ctx, input.c_str(), input.size()) != 1) {
        EVP_MD_CTX_free(ctx);
        throw runtime_error("Failed to update SHA-256");
    }

    // 最终计算并获得结果
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;
    if (EVP_DigestFinal_ex(ctx, hash, &hash_len) != 1) {
        EVP_MD_CTX_free(ctx);
        throw runtime_error("Failed to finalize SHA-256");
    }

    // 释放上下文
    EVP_MD_CTX_free(ctx);

    // 将字节数组转换为十六进制字符串
    stringstream ss;
    for (unsigned int i = 0; i < hash_len; i++) {
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    }

    return ss.str();
}


int main() {
    cout << HashFunction("alice")<<endl;
    cout << HashFunction("123456")<<endl;
}

