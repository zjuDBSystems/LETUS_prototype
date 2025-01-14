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
struct LetusINode {
  char* key;
  char* hash;
};
struct LetusProofNode {
  bool is_data;
  char* key;
  char* hash;
  LetusINode* inodes;
  int index;
};
struct LetusProofPath {
  LetusProofNode* proof_nodes;
  uint64_t proof_size;
};

struct Letus* OpenLetus(const char* path_c) {
  std::string path(path_c);
  LSVPS* page_store = new LSVPS(path);
  VDLS* value_store = new VDLS(path + "/");
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
  p->trie->Put(tid, version, key, value);
#ifdef DEBUG
  std::cout << "key: " << key_c << ", value: " << value_c << std::endl;
  std::cout << "key: " << key << ", value: " << value << std::endl;
#endif
}

char* LetusGet(Letus* p, uint64_t tid, uint64_t version, const char* key_c) {
  std::string key(key_c);
  std::string value = p->trie->Get(tid, version, key);
  size_t value_size = value.size();
  char* value_c = new char[value_size + 1];
  value.copy(value_c, value_size, 0);
  value_c[value_size] = '\0';
#ifdef DEBUG
  std::cout << "value: " << value << "," << value_c << std::endl;
#endif
  return value_c;
}

bool LetusRevert(Letus* p, uint64_t tid, uint64_t version) {
  p->trie->Revert(tid, version);
  return true;
}
bool LetusCalcRootHash(Letus* p, uint64_t tid, uint64_t version) {
  p->trie->Commit(version);
  // [TODO] replace to CalcRootHash
  return true;
}

bool LetusFlush(Letus* p, uint64_t tid, uint64_t version) {
  p->trie->Flush(tid, version);
  return true;
}

LetusProofPath* LetusProof(Letus* p, uint64_t tid, uint64_t version,
                           const char* key_c) {
  std::string key(key_c);
  std::string value = p->trie->Get(tid, version, key);
#ifdef DEBUG
  std::cout << "key: " << key << ", value: " << value << std::endl;
#endif

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
    hash = HashFunction(concatenated_hash);
  }
  LetusProofPath* path = new LetusProofPath();
  path->proof_nodes = proof_nodes;
  path->proof_size = proof_size;
  return path;
}

uint64_t LetusGetProofPathSize(LetusProofPath* path) {
  return path->proof_size;
}
bool LetusGetProofNodeIsData(LetusProofPath* path, uint64_t node_index) {
  return path->proof_nodes[node_index].is_data;
}
int LetusGetProofNodeIndex(LetusProofPath* path, uint64_t node_index) {
  return path->proof_nodes[node_index].index;
}
char* LetusGetProofNodeKey(LetusProofPath* path, uint64_t node_index) {
  return path->proof_nodes[node_index].key;
}
char* LetusGetProofNodeHash(LetusProofPath* path, uint64_t node_index) {
  return path->proof_nodes[node_index].hash;
}
uint64_t LetusGetProofNodeSize(LetusProofPath* path, uint64_t node_index) {
  return DMM_NODE_FANOUT;
}
char* LetusGetINodeKey(LetusProofPath* path, uint64_t node_index,
                       uint64_t inode_index) {
  return path->proof_nodes[node_index].inodes[inode_index].key;
}
char* LetusGetINodeHash(LetusProofPath* path, uint64_t node_index,
                        uint64_t inode_index) {
  return path->proof_nodes[node_index].inodes[inode_index].hash;
}