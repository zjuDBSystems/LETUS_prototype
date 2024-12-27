#ifndef _LETUS_H_
#define _LETUS_H_
#include <stdbool.h>
#include <stdint.h>

typedef struct Letus Letus;
typedef struct DMMTrieProof DMMTrieProof;
typedef struct NodeProof NodeProof;
typedef struct {
  char *key;
  char *hash;
} LetusINode;
typedef struct {
  bool is_data;
  char *key;
  char *hash;
  LetusINode *inodes;
  int index;
} LetusProofNode;
extern struct Letus *OpenLetus(const char *path_c);
void LetusPut(Letus *p, uint64_t tid, uint64_t version, const char *key_c,
              const char *value_c);
char *LetusGet(Letus *p, uint64_t tid, uint64_t version, const char *key_c);
void LetusProof(Letus *p, uint64_t tid, uint64_t version, const char *key_c,
                LetusProofNode **proof_nodes_, int *proof_size_);
void LetusCommit(Letus *p, uint64_t version);

// int GetSize(DMMTrieProof *proof);
// NodeProof *GetNodeProof(DMMTrieProof *proof, int index);
// char *GetKey(NodeProof *node_proof);
// char *GetHash(NodeProof *node_proof);
// bool GetIsData(NodeProof *node_proof);
// int GetIndex(NodeProof *node_proof);
#endif  // _LETUS_H_
