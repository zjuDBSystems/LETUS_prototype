extern "C" {
#include "Letus.h"
}

#include <iostream>
#include <string>
#include "DMMTrie.hpp"
#include "LSVPS.hpp"
#include "VDLS.hpp"

struct Letus {
  DMMTrie* trie;
};

struct Letus* OpenLetus(const char* path_c) {
  std::string path(path_c);
  LSVPS* page_store = new LSVPS();
  VDLS* value_store = new VDLS(path);
  DMMTrie* trie = new DMMTrie(0, page_store, value_store);
  page_store->RegisterTrie(trie);
  struct Letus* p = new Letus();
  p->trie = trie;
  return p;
}

void LetusPut(Letus* p, uint64_t tid, uint64_t version, const char* key_c,
              const char* value_c) {
  std::string key(key_c);
  std::string value(value_c);
  // std::cout << key << ", " << value << std::endl;
  p->trie->Put(tid, version, key, value);
}

char* LetusGet(Letus* p, uint64_t tid, uint64_t version, const char* key_c) {
  std::string key(key_c);
  std::string value = p->trie->Get(tid, version, key);
  size_t value_size = value.size();
  char* value_c = new char[value_size + 1];
  value.copy(value_c, value_size, 0);
  value_c[value_size] = '\0';
  return value_c;
}

void LetusCommit(Letus* p, uint64_t version) { p->trie->Commit(version); }

void LetusProof(Letus* p, uint64_t tid, uint64_t version, const char* key_c) {
  std::string key(key_c);
  DMMTrieProof proof = p->trie->GetProof(tid, version, key);
}