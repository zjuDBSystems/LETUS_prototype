#ifndef _DMMTRIE_HPP_
#define _DMMTRIE_HPP_

#include <array>
#include <cstring>
#include <iostream>
#include <list>
#include <memory>
#include <queue>
#include <tuple>
#include <unordered_map>
#include <vector>
// #include <openssl/sha.h>
#include "VDLS.hpp"
#include "utils.hpp"

#define HASH_SIZE 32  // SHA256 result size in bytes

static constexpr size_t DMM_NODE_FANOUT = 10;

using namespace std;
struct PageKey;
class Page;
string HashFunction(const string &input) {  // hash function SHA-256
  // unsigned char hash[HASH_SIZE];
  // SHA256_CTX sha256_ctx;
  // SHA256_Init(&sha256_ctx);
  // SHA256_Update(&sha256_ctx, input.c_str(), input.size());
  // SHA256_Final(hash, &sha256_ctx);

  // string hash_str;
  // for (int i = 0; i < HASH_SIZE; i++) {
  //     hash_str += (char)hash[i];
  // }
  // return hash_str;
  string str(32, 'a');
  return str;
}

class Node {
 public:
  virtual ~Node() noexcept = default;
  string hash_;  // merkle hash of the node
  uint64_t version_;

  virtual void CalculateHash(){};
  virtual void SerializeTo(char *buffer, size_t &current_size,
                           bool is_root) const = 0;
  virtual void AddChild(int index, uint64_t version, const string &hash,
                        Node *child){

  };
  virtual Node *GetChild(int index) { return nullptr; };
  virtual void UpdateNode(){};

  string GetHash() { return hash_; }

  uint64_t GetVersion() { return version_; }
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

  IndexNode(
      uint64_t version, const string &hash, uint16_t bitmap,
      const array<tuple<uint64_t, string, Node *>, DMM_NODE_FANOUT> &children)
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

  void UpdateNode(uint64_t version, int index, const string &child_hash,
                  char *deltapage) {
    version_ = version;
    bitmap_ |= (1 << index);
    get<0>(children_[index]) = version;
    get<1>(children_[index]) = child_hash;

    string concatenated_hash;
    for (int i = 0; i < DMM_NODE_FANOUT; i++) {
      concatenated_hash += get<1>(children_[i]);
    }
    hash_ = HashFunction(concatenated_hash);

    size_t current_size = strlen(deltapage);

    memcpy(deltapage + current_size, &version,
           sizeof(uint64_t));  // add version to the end of deltapage
    current_size += sizeof(uint64_t);
    memcpy(deltapage + current_size, &index, sizeof(int));  // index
    current_size += sizeof(int);
    memcpy(deltapage + current_size, child_hash.c_str(),
           child_hash.size());  // hash
    current_size += child_hash.size();

    deltapage[current_size] = '\0';
  }

  void AddChild(int index, uint64_t version, const string &hash,
                Node *child) override {
    if (index >= 0 && index < DMM_NODE_FANOUT) {
      children_[index] = make_tuple(version, hash, child);
      bitmap_ |= (1 << index);  // update bitmap
    }
  }

  Node *GetChild(int index) override {
    if (index >= 0 && index < DMM_NODE_FANOUT) {
      return get<2>(children_[index]);
    }
    return nullptr;
  }

 private:
  uint64_t version_;
  string hash_;
  uint16_t bitmap_;  // bitmap for children
  array<tuple<uint64_t, string, Node *>, DMM_NODE_FANOUT> children_;  // trie
};

class LeafNode : public Node {
 public:
  LeafNode(uint64_t V = 0, const string &k = "",
           const tuple<uint64_t, uint64_t, uint64_t> &l = {},
           const string &h = "")
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

  void UpdateNode(uint64_t version,
                  const tuple<uint64_t, uint64_t, uint64_t> &location,
                  const string &value, char *deltapage) {
    version_ = version;
    location_ = location;
    hash_ = HashFunction(key_ + value);

    size_t current_size = strlen(deltapage);

    memcpy(deltapage + current_size, &version,
           sizeof(uint64_t));  // add version to the end of deltapage
    current_size += sizeof(uint64_t);
    memcpy(deltapage + current_size, &get<0>(location),
           sizeof(uint64_t));  // fileID
    current_size += sizeof(uint64_t);
    memcpy(deltapage + current_size, &get<1>(location),
           sizeof(uint64_t));  // offset
    current_size += sizeof(uint64_t);
    memcpy(deltapage + current_size, &get<2>(location),
           sizeof(uint64_t));  // size
    current_size += sizeof(uint64_t);
    memcpy(deltapage + current_size, hash_.c_str(), hash_.size());  // hash
    current_size += hash_.size();

    deltapage[current_size] = '\0';
  }

  tuple<uint64_t, uint64_t, uint64_t> GetLocation() const { return location_; }

 private:
  uint64_t version_;
  string key_;
  tuple<uint64_t, uint64_t, uint64_t>
      location_;  // location tuple (fileID, offset, size)
  string hash_;
};

class DMMTriePage {
 public:
  DMMTriePage(Node *root = nullptr, const string &pid = "",
              uint16_t d_update_count = 0, uint16_t b_update_count = 0)
      : pid_(pid),
        root_(root),
        d_update_count_(d_update_count),
        b_update_count_(b_update_count) {}

  DMMTriePage(char *buffer) {
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

    size_t pid_size = *(reinterpret_cast<size_t *>(
        buffer + current_size));  // deserialize pid_size (8 bytes for size_t)
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

    if (is_leaf_node) {  // the root node of page is leafnode
      uint64_t node_version = *(reinterpret_cast<uint64_t *>(
          buffer + current_size));  // deserialize leafnode version
      current_size += sizeof(uint64_t);

      size_t key_size = *(reinterpret_cast<size_t *>(
          buffer + current_size));  // deserialize key_size
      current_size += sizeof(key_size);
      string key(buffer + current_size, key_size);  // deserialize key
      current_size += key_size;

      uint64_t fileID = *(reinterpret_cast<uint64_t *>(
          buffer + current_size));  // deserialize fileID
      current_size += sizeof(uint64_t);
      uint64_t offset = *(reinterpret_cast<uint64_t *>(
          buffer + current_size));  // deserialize offset
      current_size += sizeof(uint64_t);
      uint64_t size = *(reinterpret_cast<uint64_t *>(
          buffer + current_size));  // deserialize size
      current_size += sizeof(uint64_t);
      tuple<uint64_t, uint64_t, uint64_t> location =
          make_tuple(fileID, offset, size);

      string hash(buffer + current_size, HASH_SIZE);  // deserialize hash
      current_size += HASH_SIZE;

      root_ = new LeafNode(node_version, key, location, hash);
    } else {  // the root node of page is indexnode
      uint64_t node_version =
          *(reinterpret_cast<uint64_t *>(buffer + current_size));
      current_size += sizeof(uint64_t);

      string hash(buffer + current_size, HASH_SIZE);
      current_size += HASH_SIZE;

      uint16_t bitmap = *(reinterpret_cast<uint16_t *>(buffer + current_size));
      current_size += sizeof(uint16_t);

      array<pair<uint64_t, string>, DMM_NODE_FANOUT> children;
      for (int i = 0; i < DMM_NODE_FANOUT; i++) {
        uint64_t child_version =
            *(reinterpret_cast<uint64_t *>(buffer + current_size));
        current_size += sizeof(uint64_t);
        string child_hash(buffer + current_size, HASH_SIZE);
        current_size += HASH_SIZE;

        children[i] = make_pair(child_version, child_hash);
      }

      root_ = new IndexNode(node_version, hash, bitmap);

      bool child_is_leaf_node =
          *(reinterpret_cast<bool *>(buffer + current_size));
      if (child_is_leaf_node) {  // second level of page is leafnode
        for (int i = 0; i < DMM_NODE_FANOUT; i++) {
          if (bitmap &
              (1 << i)) {  // serialized data only stores children that exists
            bool is_leaf_node =
                *(reinterpret_cast<bool *>(buffer + current_size));
            current_size += sizeof(bool);

            uint64_t node_version_2 = *(reinterpret_cast<uint64_t *>(
                buffer + current_size));  // deserialize leafnode version
            current_size += sizeof(uint64_t);

            size_t key_size_2 = *(reinterpret_cast<size_t *>(
                buffer + current_size));  // deserialize key_size
            current_size += sizeof(key_size_2);
            string key_2(buffer + current_size, key_size_2);  // deserialize key
            current_size += key_size_2;

            uint64_t fileID_2 = *(reinterpret_cast<uint64_t *>(
                buffer + current_size));  // deserialize fileID
            current_size += sizeof(uint64_t);
            uint64_t offset_2 = *(reinterpret_cast<uint64_t *>(
                buffer + current_size));  // deserialize offset
            current_size += sizeof(uint64_t);
            uint64_t size_2 = *(reinterpret_cast<uint64_t *>(
                buffer + current_size));  // deserialize size
            current_size += sizeof(uint64_t);
            tuple<uint64_t, uint64_t, uint64_t> location_2 =
                make_tuple(fileID_2, offset_2, size_2);

            string hash_2(buffer + current_size,
                          HASH_SIZE);  // deserialize hash
            current_size += HASH_SIZE;

            Node *child =
                new LeafNode(node_version_2, key_2, location_2, hash_2);

            root_->AddChild(i, children[i].first, children[i].second,
                            child);  // add child to root
          }
        }
      } else {  // second level of page is indexnode
        for (int i = 0; i < DMM_NODE_FANOUT; i++) {
          if (bitmap & (1 << i)) {
            bool is_leaf_node =
                *(reinterpret_cast<bool *>(buffer + current_size));
            current_size += sizeof(bool);

            uint64_t node_version_2 =
                *(reinterpret_cast<uint64_t *>(buffer + current_size));
            current_size += sizeof(uint64_t);

            string hash_2(buffer + current_size, HASH_SIZE);
            current_size += HASH_SIZE;

            uint16_t bitmap_2 =
                *(reinterpret_cast<uint16_t *>(buffer + current_size));
            current_size += sizeof(uint16_t);

            Node *child = new IndexNode(node_version_2, hash_2, bitmap_2);

            for (int j = 0; j < DMM_NODE_FANOUT; j++) {
              uint64_t child_version_2 =
                  *(reinterpret_cast<uint64_t *>(buffer + current_size));
              current_size += sizeof(uint64_t);
              string child_hash_2(buffer + current_size, HASH_SIZE);
              current_size += HASH_SIZE;

              child->AddChild(
                  j, child_version_2, child_hash_2,
                  nullptr);  // add child to second level indexnode in page
            }

            root_->AddChild(
                i, children[i].first, children[i].second,
                child);  // add child to fist level indexnode in page
          }
        }
      }
    }
  }

  // bool AddChildToPage(int index, Node* child, uint64_t childVersion, const
  // string& child_hash) {
  //     if (root_) {
  //         root_->AddChild(index, childVersion, child_hash, child);
  //         return true;
  //     }
  //     return false;
  // }

  /* serialized BasePage format (size in bytes):
     | version (8) | tid (8) | tp (1) | pid_size (8 in 64-bit system) | pid
     (pid_size) | | deltapage update count (2) | basepage update count (2) |
     root node |
  */
  char *SerializeTo() const {
    static char buffer[12 * 1024];  // maximum size of a page is 12KB
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

    return buffer;
  }

  void UpdatePage(uint64_t version,
                  tuple<uint64_t, uint64_t, uint64_t> location,
                  const string &value, const string &nibbles,
                  const string &child_hash, char *deltapage,
                  PageKey pagekey) {  // parameter "nibbles" are the first two
                                      // nibbles after pid
    std::cout << value << " " << nibbles << std::endl;
    if (nibbles.size() ==
        0) {  // page has one leafnode, eg. page "abcdef" for key "abcdef"
      static_cast<LeafNode *>(root_)->UpdateNode(version, location, value,
                                                 deltapage);
    } else if (nibbles.size() ==
               1) {  // page has one indexnode and one level of leafnodes, eg.
                     // page "abcd" for key "abcde"
      int index = nibbles[0] - '0';
      static_cast<LeafNode *>(root_->GetChild(index))
          ->UpdateNode(version, location, value, deltapage);

      string child_hash_2 = root_->GetChild(index)->GetHash();
      static_cast<IndexNode *>(root_)->UpdateNode(version, index, child_hash_2,
                                                  deltapage);
    } else {  // page has two levels of indexnodes , eg. page "ab" for key
              // "abcdef"
      int index = nibbles[0] - '0';
      static_cast<IndexNode *>(root_)->UpdateNode(version, index, child_hash,
                                                  deltapage);

      index = nibbles[1] - '0';
      string child_hash_2 = root_->GetChild(index)->GetHash();
      static_cast<IndexNode *>(root_)->UpdateNode(version, index, child_hash_2,
                                                  deltapage);
    }

    if (++d_update_count_ >= Td_) {
      d_update_count_ = 0;
      PageKey pagekey = {version, 0, true, pid_};
      Page *page = new Page(pagekey, deltapage);
      // StorePage(&page);  // send deltapage to
      // LSVPS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      deltapage[0] =
          '\0';  // mark the first character in deltapage as null character
    }
    // if(++b_update_count_ >= Tb_) {
    //     b_update_count_ = 0;
    //     // send basepage to LSVPS
    //     DMMTrie::UpdatePageVersion(pagekey, version, version);
    //     return;
    // }
    // pair<uint64_t, uint64_t> page_version = DMMTrie::GetPageVersion(pagekey);
    // DMMTrie::UpdatePageVersion(pagekey, version, page_version.second);
  }

  Node *GetRoot() const { return root_; }

 private:
  Node *root_;  // the root of the page
  string pid_;  // nibble path serves as page id
  uint16_t d_update_count_;
  uint16_t b_update_count_;
  const uint16_t Td_ = 32;  // update threshold of DeltaPage
  const uint16_t Tb_ = 64;  // update threshold of BasePage
};

class DMMTrie {
 public:
  DMMTrie(uint64_t tid, LSVPSInterface *page_store, VDLS *value_store,
          uint64_t current_version = 0)
      : tid(tid),
        page_store_(page_store),
        value_store_(value_store),
        current_version_(current_version),
        root_page_(nullptr) {
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

    DMMTriePage *page = nullptr;
    string child_hash;
    tuple<uint64_t, uint64_t, uint64_t> location =
        value_store_->WriteValue(version, key, value);
    // start from pid of the bottom page, go upward two nibbles(one page) each
    // round
    for (int i = nibble_path.size() % 2 == 0 ? nibble_path.size()
                                             : nibble_path.size() - 1;
         i >= 0; i -= 2) {
      PageKey pagekey = {current_version_, 0, false, nibble_path.substr(0, i)};
      page = GetPage(pagekey);  // load the page into lru cache
      char *deltapage = GetDeltaPage(pagekey);
      page->UpdatePage(version, location, value, nibble_path.substr(i, 2),
                       child_hash, deltapage, pagekey);
      std::cout << "updatePage" << std::endl;
      child_hash = page->GetRoot()->GetHash();
      std::cout << "GetRoot" << std::endl;
    }
    return true;
  }

  string Get(uint64_t tid, uint64_t version, const string &key) {
    string pid = key.substr(
        0, key.size() % 2 == 0
               ? key.size()
               : key.size() -
                     1);  // pid is the largest even-length substring of key
    PageKey pagekey{version, 0, false, pid};  // false means basepage
    DMMTriePage *page = GetPage(pagekey);

    LeafNode *leafnode = nullptr;
    if (dynamic_cast<IndexNode *>(
            page->GetRoot())) {  // the root node of page is indexnode
      leafnode = static_cast<LeafNode *>(page->GetRoot()->GetChild(
          key.back() -
          '0'));  // use the last nibble in key to route to leafnode
    } else {
      leafnode = static_cast<LeafNode *>(page->GetRoot());
    }

    string value = value_store_->ReadValue(leafnode->GetLocation());
    return value;
  }

  // bool GeneratePage(Page* page, uint64_t version);//generate and pass
  // Deltapage to LSVPS

  string CalcRootHash(uint64_t tid, uint64_t version) { return ""; }

  char *GetDeltaPage(PageKey key, size_t reserve_size = 1024) {
    auto it = active_deltapages_.find(key);
    if (it != active_deltapages_.end()) {
      return it->second;  // return the reference of deltapage char stream
    } else {  // if deltapage doesn't exist, create one and reserve some space
      char *new_deltapage = new char[reserve_size];
      active_deltapages_[key] = new_deltapage;
      return active_deltapages_[key];
    }
  }

  pair<uint64_t, uint64_t> GetPageVersion(PageKey pagekey) {
    auto it = page_versions_.find(pagekey);
    if (it != page_versions_.end()) {
      return it->second;
    }
    return {0, 0};
  }

  void UpdatePageVersion(PageKey pagekey, uint64_t current_version,
                         uint64_t latest_basepage_version) {
    page_versions_[pagekey] = {current_version, latest_basepage_version};
  }

 private:
  LSVPSInterface *page_store_;
  VDLS *value_store_;
  uint64_t tid;
  DMMTriePage *root_page_;
  uint64_t current_version_;
  unordered_map<PageKey, list<pair<PageKey, DMMTriePage *>>::iterator,
                PageKey::Hash>
      lru_cache_;                                //  use a hash map as lru cache
  list<pair<PageKey, DMMTriePage *>> pagekeys_;  // list to maintain cache order
  const size_t max_cache_size_ = 32;             // maximum pages in cache
  unordered_map<PageKey, char *, PageKey::Hash>
      active_deltapages_;  // deltapage of all pages, delta pages are indexed by
                           // PageKey
  unordered_map<PageKey, pair<uint64_t, uint64_t>, PageKey::Hash>
      page_versions_;  // current version, latest basepage version

  DMMTriePage *GetPage(const PageKey &pagekey) {  // get a page by its pagekey
    auto it = lru_cache_.find(pagekey);
    if (it != lru_cache_.end()) {  // page is in cache
      pagekeys_.splice(pagekeys_.begin(), pagekeys_,
                       it->second);    // move the accessed page to the front
      it->second = pagekeys_.begin();  // update iterator
      return it->second->second;
    }

    Page *page = page_store_->LoadPage(
        pagekey);  // page is not in cache, fetch it from LSVPS
    if (!page) {
      // page is not found in disk
      Node *root = new IndexNode(pagekey.version, "", 0);
      DMMTriePage *trie_page = new DMMTriePage(root, pagekey.pid, 0, 0);
      std::cout << "DMMTriePage" << std::endl;
      PutPage(pagekey, trie_page);
      std::cout << "PutPage" << std::endl;
      return trie_page;
    }
    DMMTriePage *trie_page = new DMMTriePage(page->GetData());
    PutPage(pagekey, trie_page);
    return trie_page;
  }

  void PutPage(const PageKey &pagekey,
               DMMTriePage *page) {              // add page to cache
    if (lru_cache_.size() >= max_cache_size_) {  // cache is full
      PageKey last_key = pagekeys_.back().first;
      lru_cache_.erase(
          last_key);  // remove the page whose pagekey is at the tail of list
      pagekeys_.pop_back();
    }

    pagekeys_.push_front(make_pair(
        pagekey,
        page));  // insert the pair of PageKey and DMMTriePage* to the front
    lru_cache_[pagekey] = pagekeys_.begin();
  }
};

// int main() {
//     DMMTrie* trie = new DMMTrie(0);
//     trie->Put(0,1,"11111","aaa");
//     cout << trie->Get(0,1,"11111") << endl;

// }

#endif