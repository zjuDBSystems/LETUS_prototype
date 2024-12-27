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

void LetusProof(Letus* p, uint64_t tid, uint64_t version, const char* key_c,
                LetusProofNode** proof_nodes_, int* proof_size_) {
  std::string key(key_c);
  std::string value = p->trie->Get(tid, version, key);

  DMMTrieProof proof = p->trie->GetProof(tid, version, key);
  int proof_size = proof.proofs.size();
  LetusProofNode* proof_nodes = new LetusProofNode[proof_size];

  string hash = HashFunction(key + value);
  for (int i = 0; i < proof_size; ++i) {
    int nibble_size = proof_size - i;
    proof_nodes[i].index = nibble_size - 1;
    proof_nodes[i].is_data = (i == 0);
    proof_nodes[i].key = new char[nibble_size + 1];
    strcpy(proof_nodes[i].key, key.substr(0, nibble_size).c_str());
    proof_nodes[i].hash = new char[hash.size() + 1];
    strcpy(proof_nodes[i].hash, hash.c_str());
    proof_nodes[i].inodes = new LetusINode[DMM_NODE_FANOUT];
    NodeProof& node_proof = proof.proofs[i];
    string concatenated_hash;
    for (int j = 0; j < DMM_NODE_FANOUT; j++) {
      if (j == proof.proofs[i].index) {
        concatenated_hash += hash;
      } else {
        concatenated_hash += node_proof.sibling_hash[j];
      }
      proof_nodes[i].inodes[j].key = new char[nibble_size + 1];
      strcpy(proof_nodes[i].inodes[j].key, key.substr(0, nibble_size).c_str());
      proof_nodes[i].inodes[j].key[nibble_size - 1] = '0' + j;
      proof_nodes[i].inodes[j].hash =
          new char[node_proof.sibling_hash[j].size() + 1];
      strcpy(proof_nodes[i].inodes[j].hash, node_proof.sibling_hash[j].c_str());
    }
    cout << concatenated_hash << endl;
    cout << hash << endl;
    hash = HashFunction(concatenated_hash);
  }
  *proof_size_ = proof_size;
  *proof_nodes_ = proof_nodes;
}
