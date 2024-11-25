#ifndef _DMMTRIE_HPP_
#define _DMMTRIE_HPP_

#include <array>
#include <cstring>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <list>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <memory>
// #include <openssl/sha.h>
// #include <openssl/evp.h>
#include "VDLS.hpp"
#include "utils.hpp"

static constexpr size_t HASH_SIZE = 32;
static constexpr size_t DMM_NODE_FANOUT = 10;

using namespace std;

struct PageKey;
class Page;
class LSVPS;
class LeafNode;
class DMMTrie;
//For Test
string HashFunction(const string & input){
  string str(32, 'a');
  return str;
}
// string HashFunction(const string &input) {  // hash function SHA-256
//   EVP_MD_CTX *ctx = EVP_MD_CTX_new(); // create SHA-256 context
//   if (ctx == nullptr) {
//       throw runtime_error("Failed to create EVP_MD_CTX");
//   }

//   if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {  // initialize SHA-256 hash computation
//       EVP_MD_CTX_free(ctx);
//       throw runtime_error("Failed to initialize SHA-256");
//   }

//   if (EVP_DigestUpdate(ctx, input.c_str(), input.size()) != 1) {  // update the hash with input string
//       EVP_MD_CTX_free(ctx);
//       throw runtime_error("Failed to update SHA-256");
//   }

//   unsigned char hash[EVP_MAX_MD_SIZE];
//   unsigned int hash_len = 0;
//   if (EVP_DigestFinal_ex(ctx, hash, &hash_len) != 1) {
//       EVP_MD_CTX_free(ctx);
//       throw runtime_error("Failed to finalize SHA-256");
//   }

//   EVP_MD_CTX_free(ctx);

//   stringstream ss;
//   for (unsigned int i = 0; i < hash_len; i++) {
//       ss << hex << setw(2) << setfill('0') << (int)hash[i];  // convert the resulting hash to a hexadecimal string
//   }
//   return ss.str();
// }

class DeltaPage : public Page {
 public:
  struct DeltaItem {
    uint8_t location_in_page;
    bool is_leaf_node;
    uint64_t version;
    string hash;
   
    // unique items for leafnode
    uint64_t fileID;
    uint64_t offset;
    uint64_t size;

    // unique items for indexnode
    uint8_t index;
    string child_hash;

    DeltaItem(uint8_t loc, bool leaf, uint64_t ver, const string& h, uint64_t fID = 0, uint64_t off = 0, uint64_t sz = 0,
              uint8_t idx = 0, const string& ch_hash = "")
        : location_in_page(loc), is_leaf_node(leaf), version(ver), hash(h),
          fileID(fID), offset(off), size(sz), index(idx), child_hash(ch_hash) {}

    DeltaItem(char* buffer, size_t &current_size) {
      location_in_page = *(reinterpret_cast<uint8_t *>(buffer + current_size));
      current_size += sizeof(uint8_t);
      is_leaf_node = *(reinterpret_cast<bool *>(buffer + current_size));
      current_size += sizeof(bool);
      version = *(reinterpret_cast<uint64_t *>(buffer + current_size));
      current_size += sizeof(uint64_t);
      hash = string(buffer + current_size, HASH_SIZE);
      current_size += HASH_SIZE;

      if(is_leaf_node) {
        fileID = *(reinterpret_cast<uint64_t *>(buffer + current_size));
        current_size += sizeof(uint64_t);
        offset = *(reinterpret_cast<uint64_t *>(buffer + current_size));
        current_size += sizeof(uint64_t);
        size = *(reinterpret_cast<uint64_t *>(buffer + current_size));
        current_size += sizeof(uint64_t);
      } else {
        index = *(reinterpret_cast<uint8_t *>(buffer + current_size));
        current_size += sizeof(uint8_t);
        child_hash = string(buffer + current_size, HASH_SIZE);
        current_size += HASH_SIZE;
      }
    }

    void SerializeTo(char* buffer, size_t& current_size) const {
      memcpy(buffer + current_size, &location_in_page, sizeof(location_in_page));
      current_size += sizeof(location_in_page);
      
      memcpy(buffer + current_size, &is_leaf_node, sizeof(is_leaf_node));
      current_size += sizeof(is_leaf_node);
      
      memcpy(buffer + current_size, &version, sizeof(version));
      current_size += sizeof(version);
      
      memcpy(buffer + current_size, hash.c_str(), HASH_SIZE);
      current_size += HASH_SIZE;
      
      if (is_leaf_node) {
        memcpy(buffer + current_size, &fileID, sizeof(fileID));
        current_size += sizeof(fileID);
        memcpy(buffer + current_size, &offset, sizeof(offset));
        current_size += sizeof(offset);
        memcpy(buffer + current_size, &size, sizeof(size));
        current_size += sizeof(size);
      } else {
        memcpy(buffer + current_size, &index, sizeof(index));
        current_size += sizeof(index);

        memcpy(buffer + current_size, child_hash.c_str(), HASH_SIZE);
        current_size += HASH_SIZE;
      }
    }
 };

  DeltaPage(PageKey last_pagekey = {0, 0, true, ""}, uint16_t update_count = 0) 
            : last_pagekey_(last_pagekey), update_count_(update_count) {};

  DeltaPage(char* buffer) {
    size_t current_size = 0;

    last_pagekey_.version = *(reinterpret_cast<uint64_t *>(buffer + current_size));
    current_size += sizeof(uint64_t);
    last_pagekey_.tid = *(reinterpret_cast<int *>(buffer + current_size));
    current_size += sizeof(int);
    last_pagekey_.type = *(reinterpret_cast<bool *>(buffer + current_size));
    current_size += sizeof(bool);
    size_t pid_size = *(reinterpret_cast<size_t *>(buffer + current_size));
    current_size += sizeof(pid_size);
    last_pagekey_.pid = string(buffer + current_size, pid_size);  // deserialize pid (pid_size bytes)
    current_size += pid_size;
   
    update_count_ = *(reinterpret_cast<uint16_t *>(buffer + current_size));
    current_size += sizeof(uint16_t);

    for(int i = 0;i < update_count_; i++) {
      deltaitems_.push_back(DeltaItem(buffer, current_size));
    }
  }

  void AddIndexNodeUpdate(uint8_t location, uint64_t version, const string& hash, uint8_t index, const string& child_hash) {
    deltaitems_.push_back(DeltaItem(location, false, version, hash, 0, 0, 0, index, child_hash));
    ++update_count_;
  }

  void AddLeafNodeUpdate(uint8_t location, uint64_t version, const string& hash, uint64_t fileID, uint64_t offset, uint64_t size) {
    deltaitems_.push_back(DeltaItem(location, true, version, hash, fileID, offset, size));
    ++update_count_;
  }

  void SerializeTo() {
    char* buffer = this->GetData();
    size_t current_size = 0; 
    memcpy(buffer + current_size, &last_pagekey_.version, sizeof(uint64_t));
    current_size += sizeof(uint64_t);
    memcpy(buffer + current_size, &last_pagekey_.tid, sizeof(int));
    current_size += sizeof(int);
    memcpy(buffer + current_size, &last_pagekey_.type, sizeof(bool));
    current_size += sizeof(bool);
    size_t pid_size = last_pagekey_.pid.size();
    memcpy(buffer + current_size, &pid_size, sizeof(pid_size)); 
    current_size += sizeof(pid_size);
    memcpy(buffer + current_size, last_pagekey_.pid.c_str(), pid_size);
    current_size += pid_size;

    memcpy(buffer + current_size, &update_count_, sizeof(uint16_t));
    current_size += sizeof(uint16_t);

    for (const auto& item : deltaitems_) {
      if (current_size + sizeof(DeltaItem) > PAGE_SIZE) {  // exceeds page size
          throw overflow_error("DeltaPage exceeds PAGE_SIZE during serialization.");
      }
      item.SerializeTo(buffer, current_size); 
    }
  }

  void ClearDeltaPage() {
    deltaitems_.clear();
    update_count_ = 0;
  }

  vector<DeltaItem> GetDeltaItems() const {
    return deltaitems_;
  }

  PageKey GetLastPageKey() const {
    return last_pagekey_;
  }

  void SetLastPageKey(PageKey pagekey) {
    last_pagekey_ = pagekey;
  }

 private:
  vector<DeltaItem> deltaitems_;
  PageKey last_pagekey_;
  uint16_t update_count_;
};


class Node {
 public:
  virtual ~Node() noexcept = default;
  string hash_;  // merkle hash of the node
  uint64_t version_;

  virtual void CalculateHash() {};
  virtual void SerializeTo(char *buffer, size_t &current_size, bool is_root) const = 0;
  virtual void DeserializeFrom(char *buffer, size_t &current_size, bool is_root) = 0;
  virtual void AddChild(int index, Node* child, uint64_t version, const string& hash)  {};
  virtual Node* GetChild(int index) { return nullptr; };
  virtual bool HasChild(int index) {return false;};
  virtual void SetChild(int index, uint64_t version, string hash) {};
  virtual void UpdateNode() {};
  virtual void SetLocation(tuple<uint64_t, uint64_t, uint64_t> location) {};

  string GetHash() { return hash_; }
  uint64_t GetVersion() { return version_; }
  virtual void SetVersion(uint64_t version) { version_ = version; };
  virtual void SetHash(string hash) { hash_ = hash; };
};


class LeafNode : public Node {
 public:
  LeafNode(uint64_t V = 0, const string &k = "", const tuple<uint64_t, uint64_t, uint64_t> &l = {}, const string &h = "")
      : version_(V), key_(k), location_(l), hash_(h) {}

  void CalculateHash(const string &value) {
    hash_ = HashFunction(key_ + value);
  }

  /* serialized leaf node format (size in bytes):
     | is_leaf_node (1) | version (8) | key_size (8 in 64-bit system) | key
     (key_size) | location(8, 8, 8) | hash (32) |
  */
  void SerializeTo(char *buffer, size_t &current_size,
                   bool is_root) const override {
    bool is_leaf_node = true;
    memcpy(buffer + current_size, &is_leaf_node,
           sizeof(bool));  // true means that the node is leafnode
    current_size += sizeof(bool);

    memcpy(buffer + current_size, &version_, sizeof(uint64_t));
    current_size += sizeof(uint64_t);

    size_t key_size = key_.size();
    memcpy(buffer + current_size, &key_size, sizeof(key_size));  // key size
    current_size += sizeof(key_size);
    memcpy(buffer + current_size, key_.c_str(), key_size);  // key
    current_size += key_size;

    memcpy(buffer + current_size, &get<0>(location_),
           sizeof(uint64_t));  // fileID
    current_size += sizeof(uint64_t);
    memcpy(buffer + current_size, &get<1>(location_),
           sizeof(uint64_t));  // offset
    current_size += sizeof(uint64_t);
    memcpy(buffer + current_size, &get<2>(location_),
           sizeof(uint64_t));  // size
    current_size += sizeof(uint64_t);

    memcpy(buffer + current_size, hash_.c_str(), hash_.size());
    current_size += hash_.size();
  }

  void DeserializeFrom(char *buffer, size_t &current_size, bool is_root) override {
    version_ = *(reinterpret_cast<uint64_t *>(buffer + current_size));  // deserialize leafnode version
    current_size += sizeof(uint64_t);

    size_t key_size = *(reinterpret_cast<size_t *>(buffer + current_size));  // deserialize key_size
    current_size += sizeof(key_size);
    key_ = string(buffer + current_size, key_size);  // deserialize key
    current_size += key_size;

    uint64_t fileID = *(reinterpret_cast<uint64_t *>(buffer + current_size));  // deserialize fileID
    current_size += sizeof(uint64_t);
    uint64_t offset = *(reinterpret_cast<uint64_t *>(buffer + current_size));  // deserialize offset
    current_size += sizeof(uint64_t);
    uint64_t size = *(reinterpret_cast<uint64_t *>(buffer + current_size));  // deserialize size
    current_size += sizeof(uint64_t);
    location_ = make_tuple(fileID, offset, size);

    hash_ = string (buffer + current_size, HASH_SIZE);  // deserialize hash
    current_size += HASH_SIZE;
  }

  void UpdateNode(uint64_t version, const tuple<uint64_t, uint64_t, uint64_t> &location, const string &value, int index, bool is_root, DeltaPage* deltapage) {
    version_ = version;
    location_ = location;
    hash_ = HashFunction(key_ + value);
    
    uint8_t location_in_page = is_root ? 0 : index + 1;
    deltapage->AddLeafNodeUpdate(location_in_page, version, hash_, get<0>(location), get<1>(location), get<2>(location));
  }

  tuple<uint64_t, uint64_t, uint64_t> GetLocation() const { return location_; }

  void SetLocation(tuple<uint64_t, uint64_t, uint64_t> location) override {location_ = location;}

 private:
  uint64_t version_;
  string key_;
  tuple<uint64_t, uint64_t, uint64_t>
      location_;  // location tuple (fileID, offset, size)
  string hash_;
};


class IndexNode : public Node {
 public:
  IndexNode(uint64_t V = 0, const string &h = "", uint16_t b = 0)
      : version_(V), hash_(h), bitmap_(b) {
    for (size_t i = 0; i < DMM_NODE_FANOUT; i++) {
      children_[i] =
          make_tuple(0, "", nullptr);  // initialize children to default
    }
  }

  IndexNode(uint64_t version, const string &hash, uint16_t bitmap, const array<tuple<uint64_t, string, Node *>, DMM_NODE_FANOUT> &children)
      : version_(version), hash_(hash), bitmap_(bitmap), children_(children) {}

  ~IndexNode() noexcept override = default;

  void CalculateHash() override {
    string concatenated_hash;
    for (int i = 0; i < DMM_NODE_FANOUT; i++) {
      concatenated_hash += get<1>(children_[i]);
    }
    hash_ = HashFunction(concatenated_hash);
  }

  /* serialized index node format (size in bytes):
     | is_leaf_node (1) | version (8) | hash (32) | bitmap (2) | Vc (8) | Hc
     (32) | Vc (8) | Hc (32) | ... | child 1 | child 2 | ... the function
     doesn't serialize pointer and doesn't serialize empty child nodes
  */
  void SerializeTo(char *buffer, size_t &current_size,
                   bool is_root) const override {
    bool is_leaf_node = false;
    memcpy(buffer + current_size, &is_leaf_node,
           sizeof(bool));  // false means that the node is indexnode
    current_size += sizeof(bool);

    memcpy(buffer + current_size, &version_, sizeof(uint64_t));
    current_size += sizeof(uint64_t);

    memcpy(buffer + current_size, hash_.c_str(), hash_.size());
    current_size += hash_.size();

    memcpy(buffer + current_size, &bitmap_, sizeof(uint16_t));
    current_size += sizeof(uint16_t);

    for (int i = 0; i < DMM_NODE_FANOUT; i++) {
      uint64_t child_version = get<0>(children_[i]);
      string child_hash = get<1>(children_[i]);

      memcpy(buffer + current_size, &child_version, sizeof(uint64_t));
      current_size += sizeof(uint64_t);
      memcpy(buffer + current_size, child_hash.c_str(), child_hash.size());
      current_size += child_hash.size();
    }

    if (is_root) {  // if an index node is the root node of a page, serialize
                    // its children
      for (int i = 0; i < DMM_NODE_FANOUT; i++) {
        if (bitmap_ & (1 << i)) {  // only serialize children that exists
          Node *child = get<2>(children_[i]);
          child->SerializeTo(buffer, current_size, false);
        }
      }
    }
  }

  void DeserializeFrom(char *buffer, size_t &current_size, bool is_root) override {
    version_ = *(reinterpret_cast<uint64_t *>(buffer + current_size));
    current_size += sizeof(uint64_t);

    hash_ = string(buffer + current_size, HASH_SIZE);
    current_size += HASH_SIZE;

    bitmap_ = *(reinterpret_cast<uint16_t *>(buffer + current_size));
    current_size += sizeof(uint16_t);

    for (int i = 0; i < DMM_NODE_FANOUT; i++) {
      uint64_t child_version = *(reinterpret_cast<uint64_t *>(buffer + current_size));
      current_size += sizeof(uint64_t);
      string child_hash(buffer + current_size, HASH_SIZE);
      current_size += HASH_SIZE;

      children_[i] = make_tuple(child_version, child_hash, nullptr);
    }

    if(!is_root) {  // indexnode is in second level of a page, return
      return;
    }

    for (int i = 0; i < DMM_NODE_FANOUT; i++) {
      if (bitmap_ & (1 << i)) {  // serialized data only stores children that exists
        bool child_is_leaf_node = *(reinterpret_cast<bool *>(buffer + current_size));
        current_size += sizeof(bool);

        if(child_is_leaf_node) {  // second level of page is leafnode
          Node* child = new LeafNode();
          child->DeserializeFrom(buffer, current_size, false);
          this->AddChild(i, child);  // add pointer to children in indexnode
        } else {  // second level of page is indexnode
          Node* child = new IndexNode();
          child->DeserializeFrom(buffer, current_size, false);
          this->AddChild(i, child);
        }
      }
    }
  }

  void UpdateNode(uint64_t version, int index, const string &child_hash, bool is_root, DeltaPage* deltapage) {
    version_ = version;
    bitmap_ |= (1 << index);
    get<0>(children_[index]) = version;
    get<1>(children_[index]) = child_hash;

    string concatenated_hash;
    for (int i = 0; i < DMM_NODE_FANOUT; i++) {
      concatenated_hash += get<1>(children_[i]);
    }
    hash_ = HashFunction(concatenated_hash);

    uint8_t location_in_page = is_root ? 0 : index + 1;
    deltapage->AddIndexNodeUpdate(location_in_page, version, hash_, index, child_hash);
  }

  void AddChild(int index, Node *child, uint64_t version = 0, const string &hash = "") override {
    if (index >= 0 && index < DMM_NODE_FANOUT) {
      children_[index] = make_tuple(version, hash, child);
      bitmap_ |= (1 << index);  // update bitmap
    }
    else throw runtime_error("AddChild out of range.");
  }

  Node *GetChild(int index) override {
    if (index >= 0 && index < DMM_NODE_FANOUT) {
      if(bitmap_ & (1 << index)) {
        return get<2>(children_[index]);
      }
      else throw runtime_error("GetChild: child doesn't exist");
    }
    else throw runtime_error("GetChild out of range.");
  }

  bool HasChild(int index) override {
    return bitmap_ & (1 << index) ? true : false;
  }

  void SetChild(int index, uint64_t version, string hash) override {
    if (index >= 0 && index < DMM_NODE_FANOUT) {
      get<0>(children_[index]) = version;
      get<1>(children_[index]) = hash;
      bitmap_ |= (1 << index);  // update bitmap
    }
    else throw runtime_error("SetChild out of range.");
  }

 private:
  uint64_t version_;
  string hash_;
  uint16_t bitmap_;  // bitmap for children
  array<tuple<uint64_t, string, Node *>, DMM_NODE_FANOUT> children_;  // trie
};


class BasePage : public Page {
 public:
  BasePage(DMMTrie* trie = nullptr, Node *root = nullptr, const string &pid = "", uint16_t d_update_count = 0, uint16_t b_update_count = 0)
            : trie_(trie), root_(root), pid_(pid), d_update_count_(d_update_count), b_update_count_(b_update_count) {}

  BasePage(DMMTrie* trie, char *buffer) : trie_(trie) {
    size_t current_size = 0;

    uint64_t version = *(reinterpret_cast<uint64_t *>(
        buffer + current_size));  // deserialize version
    current_size += sizeof(uint64_t);

    uint64_t tid = *(reinterpret_cast<uint64_t *>(
        buffer + current_size));  // deserialize DMMTrie id
    current_size += sizeof(uint64_t);

    bool page_type = *(reinterpret_cast<bool *>(
        buffer + current_size));  // deserialize page type (1 byte)
    current_size += sizeof(bool);

    size_t pid_size = *(reinterpret_cast<size_t *>(buffer + current_size));  // deserialize pid_size (8 bytes for size_t)
    current_size += sizeof(pid_size);
    pid_ = string(buffer + current_size,
                  pid_size);  // deserialize pid (pid_size bytes)
    current_size += pid_size;

    d_update_count_ = *(reinterpret_cast<uint16_t *>(
        buffer + current_size));  // deserialize d_update_count (2 bytes)
    current_size += sizeof(uint16_t);
    b_update_count_ = *(reinterpret_cast<uint16_t *>(
        buffer + current_size));  // deserialize b_update_count (2 bytes)
    current_size += sizeof(uint16_t);

    bool is_leaf_node = *(reinterpret_cast<bool *>(buffer + current_size));
    current_size += sizeof(bool);

    if(is_leaf_node) {  // the root node of page is leafnode
      root_ = new LeafNode();
      root_->DeserializeFrom(buffer, current_size, true);
    } else {  // the root node of page is indexnode
      root_ = new IndexNode();
      root_->DeserializeFrom(buffer, current_size, true);
    }
  }


  /* serialized BasePage format (size in bytes):
     | version (8) | tid (8) | tp (1) | pid_size (8 in 64-bit system) | pid
     (pid_size) | | deltapage update count (2) | basepage update count (2) |
     root node |
  */
  void SerializeTo() {
    char* buffer = this->GetData();
    size_t current_size = 0;

    uint64_t version = root_->GetVersion();
    memcpy(buffer + current_size, &version, sizeof(uint64_t));
    current_size += sizeof(uint64_t);

    uint64_t tid = 0;
    memcpy(buffer + current_size, &tid, sizeof(uint64_t));
    current_size += sizeof(uint64_t);

    bool page_type = false;  // Tp is false means basepage
    memcpy(buffer + current_size, &page_type, sizeof(bool));
    current_size += sizeof(bool);

    size_t pid_size = pid_.size();
    memcpy(buffer + current_size, &pid_size, sizeof(pid_size));  // pid size
    current_size += sizeof(pid_size);
    memcpy(buffer + current_size, pid_.c_str(), pid_size);  // pid
    current_size += pid_size;

    memcpy(buffer + current_size, &d_update_count_, sizeof(uint16_t));
    current_size += sizeof(uint16_t);
    memcpy(buffer + current_size, &b_update_count_, sizeof(uint16_t));
    current_size += sizeof(uint16_t);

    root_->SerializeTo(buffer, current_size, true);  // serialize nodes
  }

  void UpdatePage(uint64_t version, tuple<uint64_t, uint64_t, uint64_t> location, const string &value,
                  const string &nibbles, const string &child_hash, DeltaPage* deltapage,
                  PageKey pagekey) ;

  void UpdateDeltaItem(DeltaPage::DeltaItem deltaitem) {  // add one update from deltapage to basepage 
    Node* node = nullptr;
    if(deltaitem.is_leaf_node) {
      if(deltaitem.location_in_page == 0) {
        node = root_;
      } else if(!root_->HasChild(deltaitem.location_in_page - 1)) {
        node = new LeafNode();
        root_->AddChild(deltaitem.location_in_page - 1, node, 0, "");
      } else {
        node = root_->GetChild(deltaitem.location_in_page - 1);
      }

      node->SetVersion(deltaitem.version);
      node->SetLocation(make_tuple(deltaitem.fileID, deltaitem.offset, deltaitem.size));
      node->SetHash(deltaitem.hash);
    } else {
      if(deltaitem.location_in_page == 0) {
        node = root_;
      } else if(!root_->HasChild(deltaitem.location_in_page - 1)) {
        node = new IndexNode();
        root_->AddChild(deltaitem.location_in_page - 1, node, 0, "");
      } else {
        node = root_->GetChild(deltaitem.location_in_page - 1);
      }

      node->SetVersion(deltaitem.version);
      node->SetHash(deltaitem.hash);
      node->SetChild(deltaitem.location_in_page - 1, deltaitem.version, deltaitem.child_hash);
    }
  }

  Node *GetRoot() const { 
    return root_; 
  }

 private:
  DMMTrie* trie_;
  Node *root_;  // the root of the page
  string pid_;  // nibble path serves as page id
  uint16_t d_update_count_;
  uint16_t b_update_count_;
  const uint16_t Td_ = 32;  // update threshold of DeltaPage
  const uint16_t Tb_ = 64;  // update threshold of BasePage
};




class DMMTrie {
 public:
  DMMTrie(uint64_t tid, LSVPSInterface *page_store, VDLS *value_store, uint64_t current_version = 0)
      : tid(tid), page_store_(page_store), value_store_(value_store), current_version_(current_version), root_page_(nullptr) {
    lru_cache_.clear();
    pagekeys_.clear();
    active_deltapages_.clear();
    page_versions_.clear();
  }

  bool Put(uint64_t tid, uint64_t version, const string &key,
           const string &value) {
    string nibble_path = key;  // saved interface for potential change of nibble
    if (version < current_version_) {
      cout << "Version " << version << " is outdated!"
           << endl;  // version invalid
      return false;
    }
    current_version_ = version;

    BasePage *page = nullptr;
    string child_hash;
    tuple<uint64_t, uint64_t, uint64_t> location = value_store_->WriteValue(version, key, value);

    // start from pid of the bottom page, go upward two nibbles(one page) each round
    for (int i = nibble_path.size() % 2 == 0 ? nibble_path.size() : nibble_path.size() - 1; i >= 0; i -= 2) {
      string nibbles = nibble_path.substr(i, 2);
      PageKey pagekey = {current_version_, 0, false, nibble_path.substr(0, i)};
      page = GetPage(pagekey);  // load the page into lru cache

      if(page == nullptr) {  // GetPage returns nullptr means that the pid is new
        if(nibbles.size() == 0) {  // leafnode
          Node* root_node = new LeafNode(0, key, {}, "");
          page = new BasePage(this, root_node, nibble_path.substr(0, i), 0, 0);
        } else if(nibbles.size() == 1) {  //indexnode->leafnode
          Node* child_node = new LeafNode(0, key, {}, "");
          Node* root_node = new IndexNode(0, "", 0);

          int index = nibbles[0] - '0';
          root_node->AddChild(index, child_node, 0, "");
          page = new BasePage(this, root_node, nibble_path.substr(0, i), 0, 0);
        } else {  // indexnode->indexnode
          int index = nibbles[1] - '0';
          Node* child_node = new IndexNode(0, "", 1 << index);  // second level of indexnode should route its child by bitmap
          Node* root_node = new IndexNode(0, "", 0);

          index = nibbles[0] - '0';
          root_node->AddChild(index, child_node, 0, "");
          page = new BasePage(this, root_node, nibble_path.substr(0, i), 0, 0);
        }
        PutPage(pagekey, page);  // add the newly generated page into cache
      }

      DeltaPage* deltapage = GetDeltaPage(nibble_path.substr(0, i));
      page->UpdatePage(version, location, value, nibbles, child_hash, deltapage, pagekey);
      child_hash = page->GetRoot()->GetHash();
    }
    return true;
  }

  string Get(uint64_t tid, uint64_t version, const string &key) {
    string pid = key.substr(0, key.size() % 2 == 0 ? key.size() : key.size() - 1);  // pid is the largest even-length substring of key
    PageKey pagekey{version, 0, false, pid};                    // false means basepage
    BasePage *page = GetPage(pagekey);

    if(page == nullptr) { 
      cout << "Key " << key << "not found at version" << version << endl;
      return "";
    }

    LeafNode *leafnode = nullptr;
    if (dynamic_cast<IndexNode *>(page->GetRoot())) {  // the root node of page is indexnode
      leafnode = static_cast<LeafNode *>(page->GetRoot()->GetChild(key.back() - '0'));  // use the last nibble in key to route leafnode
    } else {
      leafnode = static_cast<LeafNode *>(page->GetRoot());
    }

    string value = value_store_->ReadValue(leafnode->GetLocation());
    return value;
  }

  // bool GeneratePage(Page* page, uint64_t version);//generate and pass
  // Deltapage to LSVPS

  string CalcRootHash(uint64_t tid, uint64_t version) { return ""; }

  const DeltaPage* GetDeltaPage(const string& pid) const {
    auto it = active_deltapages_.find(pid);
    if (it != active_deltapages_.end()) {
        return &it->second;  // return deltapage if it exiests
    } else {
        // DeltaPage new_page;
        // active_deltapages_[pid] = new_page;
        // return &active_deltapages_[pid];
        return nullptr;
    }
  }
  DeltaPage* GetDeltaPage(const string& pid) {
    auto it = active_deltapages_.find(pid);
    if (it != active_deltapages_.end()) {
        return &it->second;  // return deltapage if it exiests
    } else {
        DeltaPage new_page;
        active_deltapages_[pid] = new_page;
        return &active_deltapages_[pid];
    }
  }

  pair<uint64_t, uint64_t> GetPageVersion(PageKey pagekey) {
    auto it = page_versions_.find(pagekey);
    if (it != page_versions_.end()) {
      return it->second;
    }
    return {0, 0};
  }

  PageKey GetLatestBasePageKey(PageKey pagekey) const {
    auto it = page_versions_.find(pagekey);
    if (it != page_versions_.end()) {
      return {it->second.second, pagekey.tid, true, pagekey.pid};
    }
    return PageKey{0, 0, false, ""};
  }

  void UpdatePageVersion(PageKey pagekey, uint64_t current_version, uint64_t latest_basepage_version) {
    page_versions_[pagekey] = {current_version, latest_basepage_version};
  }

  LSVPSInterface* GetPageStore() { return page_store_; }

 private:
  LSVPSInterface *page_store_;
  VDLS *value_store_;
  uint64_t tid;
  BasePage *root_page_;
  uint64_t current_version_;
  unordered_map<PageKey, list<pair<PageKey, BasePage *>>::iterator, PageKey::Hash> lru_cache_;  //  use a hash map as lru cache
  list<pair<PageKey, BasePage *>> pagekeys_;  // list to maintain cache order
  const size_t max_cache_size_ = 32;             // maximum pages in cache
  unordered_map<string, DeltaPage> active_deltapages_;  // deltapage of all pages, delta pages are indexed by pid
  unordered_map<PageKey, pair<uint64_t, uint64_t>, PageKey::Hash>
      page_versions_;  // current version, latest basepage version

  BasePage *GetPage(const PageKey &pagekey) {  // get a page by its pagekey
    auto it = lru_cache_.find(pagekey);
    if (it != lru_cache_.end()) {  // page is in cache
      pagekeys_.splice(pagekeys_.begin(), pagekeys_,
                       it->second);    // move the accessed page to the front
      it->second = pagekeys_.begin();  // update iterator
      return it->second->second;
    }

    Page *page = page_store_->LoadPage(pagekey);  // page is not in cache, fetch it from LSVPS
    if (!page) {  // page is not found in disk
      return nullptr;
    }
    BasePage* trie_page = new BasePage(this, page->GetData());
    PutPage(pagekey, trie_page);
    return trie_page;
  }

  void PutPage(const PageKey &pagekey, BasePage *page) {  // add page to cache
    if (lru_cache_.size() >= max_cache_size_) {              // cache is full
      PageKey last_key = pagekeys_.back().first;
      lru_cache_.erase(
          last_key);  // remove the page whose pagekey is at the tail of list
      pagekeys_.pop_back();
    }

    pagekeys_.push_front(make_pair(pagekey, page));  // insert the pair of PageKey and BasePage* to the front
    lru_cache_[pagekey] = pagekeys_.begin();
  }
};

void BasePage::UpdatePage(uint64_t version, tuple<uint64_t, uint64_t, uint64_t> location, const string &value,
                  const string &nibbles, const string &child_hash, DeltaPage* deltapage,
                  PageKey pagekey) {  // parameter "nibbles" are the first two nibbles after pid
    if (nibbles.size() == 0) {        // page has one leafnode, eg. page "abcdef" for key "abcdef"
      static_cast<LeafNode *>(root_)->UpdateNode(version, location, value, 0, true, deltapage);
    } else if (nibbles.size() == 1) {  // page has one indexnode and one level of leafnodes, eg. page "abcd" for key "abcde"
      int index = nibbles[0] - '0';
      static_cast<LeafNode *>(root_->GetChild(index))->UpdateNode(version, location, value, index, true, deltapage);

      string child_hash_2 = root_->GetChild(index)->GetHash();
      static_cast<IndexNode *>(root_)->UpdateNode(version, index, child_hash_2, false, deltapage);
    } else {  // page has two levels of indexnodes , eg. page "ab" for key "abcdef"
      int index = nibbles[0] - '0';
      static_cast<IndexNode *>(root_)->UpdateNode(version, index, child_hash, true, deltapage);

      index = nibbles[1] - '0';
      string child_hash_2 = root_->GetChild(index)->GetHash();
      static_cast<IndexNode *>(root_)->UpdateNode(version, index, child_hash_2, false, deltapage);
    }
    
    PageKey deltapage_pagekey = {version, 0, true, pid_};
    if (++d_update_count_ >= Td_) {  // When a DeltaPage accumulates 𝑇𝑑 updates, it is frozen and a new active one is initiated
      d_update_count_ = 0;
      // PageKey deltapage_pagekey = {version, 0, true, pid_};
      deltapage->SerializeTo();
      trie_->GetPageStore()->StorePage(deltapage);  // send deltapage to LSVPS
      deltapage->ClearDeltaPage();  // delete all DeltaItems in DeltaPage
      deltapage->SetLastPageKey(deltapage_pagekey); // record the PageKey of DeltaPage that is passed to LSVPS
    }
    if(++b_update_count_ >= Tb_) {  // Each page generates a checkpoint as BasePage after every 𝑇𝑏 updates
      b_update_count_ = 0;
      this->SerializeTo();
      trie_->GetPageStore()->StorePage(this);  // send basepage to LSVPS
      trie_->UpdatePageVersion(pagekey, version, version);
      deltapage->SetLastPageKey(deltapage_pagekey);
      return;
    }

    pair<uint64_t, uint64_t> page_version = trie_->GetPageVersion(pagekey);
    trie_->UpdatePageVersion(pagekey, version, page_version.second);
  }

  

#endif