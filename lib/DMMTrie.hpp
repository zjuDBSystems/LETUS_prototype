#ifndef _DMMTRIE_HPP_
#define _DMMTRIE_HPP_

#include <openssl/evp.h>
#include <openssl/sha.h>

#include <array>
#include <cstring>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "VDLS.hpp"
#include "utils.hpp"

static constexpr size_t HASH_SIZE = 64;
static constexpr size_t DMM_NODE_FANOUT = 10;

using namespace std;

struct PageKey;
class Page;
class LSVPS;
class LeafNode;
class DMMTrie;
class DeltaPage;

struct NodeProof {
  int index;
  uint16_t bitmap;
  vector<string> sibling_hash;
};

struct DMMTrieProof {
  string value;
  vector<NodeProof> proofs;
};

class Node {
 public:
  virtual ~Node() noexcept = default;

  virtual void CalculateHash();
  virtual void SerializeTo(char *buffer, size_t &current_size,
                           bool is_root) const = 0;
  virtual void DeserializeFrom(char *buffer, size_t &current_size,
                               bool is_root) = 0;
  virtual void AddChild(int index, Node *child, uint64_t version,
                        const string &hash);
  virtual Node *GetChild(int index);
  virtual bool HasChild(int index);
  virtual void SetChild(int index, uint64_t version, string hash);
  virtual string GetChildHash(int index);
  virtual uint64_t GetChildVersion(int index);
  virtual void UpdateNode();
  virtual void SetLocation(tuple<uint64_t, uint64_t, uint64_t> location);

  virtual string GetHash() = 0;
  virtual uint64_t GetVersion() = 0;
  virtual void SetVersion(uint64_t version) = 0;
  virtual void SetHash(string hash) = 0;

  virtual bool IsLeaf() const = 0;

  virtual NodeProof GetNodeProof(int index);
};

class LeafNode : public Node {
 public:
  LeafNode(uint64_t V = 0, const string &k = "",
           const tuple<uint64_t, uint64_t, uint64_t> &l = {},
           const string &h = "");
  void CalculateHash(const string &value);
  void SerializeTo(char *buffer, size_t &current_size,
                   bool is_root) const override;
  void DeserializeFrom(char *buffer, size_t &current_size,
                       bool is_root) override;
  void UpdateNode(uint64_t version,
                  const tuple<uint64_t, uint64_t, uint64_t> &location,
                  const string &value, uint8_t location_in_page,
                  DeltaPage *deltapage);
  tuple<uint64_t, uint64_t, uint64_t> GetLocation() const;
  void SetLocation(tuple<uint64_t, uint64_t, uint64_t> location) override;
  string GetHash();
  uint64_t GetVersion();
  void SetVersion(uint64_t version);
  void SetHash(string hash);
  bool IsLeaf() const override;

 private:
  uint64_t version_;
  string key_;
  tuple<uint64_t, uint64_t, uint64_t>
      location_;  // location tuple (fileID, offset, size)
  string hash_;
  const bool is_leaf_;
};

class IndexNode : public Node {
 public:
  IndexNode(uint64_t V = 0, const string &h = "", uint16_t b = 0);
  IndexNode(
      uint64_t version, const string &hash, uint16_t bitmap,
      const array<tuple<uint64_t, string, Node *>, DMM_NODE_FANOUT> &children);
  void CalculateHash() override;
  void SerializeTo(char *buffer, size_t &current_size,
                   bool is_root) const override;
  void DeserializeFrom(char *buffer, size_t &current_size,
                       bool is_root) override;
  void UpdateNode(uint64_t version, int index, const string &child_hash,
                  uint8_t location_in_page, DeltaPage *deltapage);
  void AddChild(int index, Node *child, uint64_t version = 0,
                const string &hash = "") override;
  Node *GetChild(int index) override;
  bool HasChild(int index) override;
  void SetChild(int index, uint64_t version, string hash) override;
  string GetChildHash(int index);
  uint64_t GetChildVersion(int index);
  string GetHash();
  uint64_t GetVersion();
  void SetVersion(uint64_t version);
  void SetHash(string hash);
  bool IsLeaf() const override;
  NodeProof GetNodeProof(int index);

 private:
  uint64_t version_;
  string hash_;
  uint16_t bitmap_;  // bitmap for children
  array<tuple<uint64_t, string, Node *>, DMM_NODE_FANOUT> children_;  // trie
  const bool is_leaf_;
};

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

    DeltaItem(uint8_t loc, bool leaf, uint64_t ver, const string &h,
              uint64_t fID = 0, uint64_t off = 0, uint64_t sz = 0,
              uint8_t idx = 0, const string &ch_hash = "");
    DeltaItem(char *buffer, size_t &current_size);
    void SerializeTo(char *buffer, size_t &current_size) const;
  };

  DeltaPage(PageKey last_pagekey = {0, 0, true, ""}, uint16_t update_count = 0,
            uint16_t b_update_count = 0);
  DeltaPage(char *buffer);
  void AddIndexNodeUpdate(uint8_t location, uint64_t version,
                          const string &hash, uint8_t index,
                          const string &child_hash);
  void AddLeafNodeUpdate(uint8_t location, uint64_t version, const string &hash,
                         uint64_t fileID, uint64_t offset, uint64_t size);
  void SerializeTo();
  void ClearDeltaPage();
  vector<DeltaItem> GetDeltaItems() const;
  PageKey GetLastPageKey() const;
  void SetLastPageKey(PageKey pagekey);
  uint16_t GetDeltaPageUpdateCount();
  uint16_t GetBasePageUpdateCount();
  void ClearBasePageUpdateCount();

 private:
  vector<DeltaItem> deltaitems_;
  PageKey last_pagekey_;
  uint16_t update_count_;
  uint16_t b_update_count_;
};

class BasePage : public Page {
 public:
  BasePage(DMMTrie *trie = nullptr, Node *root = nullptr,
           const string &pid = "");
  BasePage(DMMTrie *trie, char *buffer);
  BasePage(DMMTrie *trie, string key, string pid, string nibbles);
  ~BasePage();
  void SerializeTo();
  void UpdatePage(uint64_t version,
                  tuple<uint64_t, uint64_t, uint64_t> location,
                  const string &value, const string &nibbles,
                  const string &child_hash, DeltaPage *deltapage,
                  PageKey pagekey);
  void UpdateDeltaItem(DeltaPage::DeltaItem deltaitem);
  Node *GetRoot() const;

 private:
  DMMTrie *trie_;
  Node *root_;              // the root of the page
  string pid_;              // nibble path serves as page id
  const uint16_t Td_ = 128;  // update threshold of DeltaPage
  const uint16_t Tb_ = 256;  // update threshold of BasePage
};

class DMMTrie {
 public:
  DMMTrie(uint64_t tid, LSVPSInterface *page_store, VDLS *value_store,
          uint64_t current_version = 0);
  void Put(uint64_t tid, uint64_t version, const string &key,
           const string &value);
  string Get(uint64_t tid, uint64_t version, const string &key);
  void Commit(uint64_t version);
  void CalcRootHash(uint64_t tid, uint64_t version);
  string GetRootHash(uint64_t tid, uint64_t version);
  DMMTrieProof GetProof(uint64_t tid, uint64_t version, const string &key);
  DeltaPage *GetDeltaPage(const string &pid);
  pair<uint64_t, uint64_t> GetPageVersion(PageKey pagekey);
  PageKey GetLatestBasePageKey(PageKey pagekey) const;
  void UpdatePageVersion(PageKey pagekey, uint64_t current_version,
                         uint64_t latest_basepage_version);
  LSVPSInterface *GetPageStore();
  void WritePageCache(PageKey pagekey, Page *page);

 private:
  LSVPSInterface *page_store_;
  VDLS *value_store_;
  uint64_t tid;
  BasePage *root_page_;
  uint64_t current_version_;
  unordered_map<PageKey, list<pair<PageKey, BasePage *>>::iterator,
                PageKey::Hash>
      lru_cache_;                             //  use a hash map as lru cache
  list<pair<PageKey, BasePage *>> pagekeys_;  // list to maintain cache order
  const size_t max_cache_size_ = 32;          // maximum pages in cache
  unordered_map<string, DeltaPage>
      active_deltapages_;  // deltapage of all pages, delta pages are indexed by
                           // pid
  unordered_map<string, pair<uint64_t, uint64_t>>
      page_versions_;  // current version, latest basepage version
  map<PageKey, Page *> page_cache_;
  map<string, string> put_cache_;  // temporarily store the key of value of Put

  BasePage *GetPage(const PageKey &pagekey);
  void PutPage(const PageKey &pagekey, BasePage *page);
  void UpdatePageKey(const PageKey &old_pagekey, const PageKey &new_pagekey);
};

#endif