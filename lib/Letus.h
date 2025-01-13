#ifndef _LETUS_H_
#define _LETUS_H_
#include <stdbool.h>
#include <stdint.h>

typedef struct Letus Letus;
typedef struct LetusProofPath LetusProofPath;

extern struct Letus* OpenLetus(const char* path_c);
void LetusPut(Letus* p, uint64_t tid, uint64_t version, const char* key_c,
              const char* value_c);
void LetusDelete(Letus* p, uint64_t tid, uint64_t version, const char* key_c);
char* LetusGet(Letus* p, uint64_t tid, uint64_t version, const char* key_c);
bool LetusRevert(Letus* p, uint64_t tid, uint64_t version);
bool LetusCalcRootHash(Letus* p, uint64_t tid, uint64_t version);
bool LetusFlush(Letus* p, uint64_t tid, uint64_t version);
LetusProofPath* LetusProof(Letus* p, uint64_t tid, uint64_t version,
                           const char* key_c);
uint64_t LetusGetProofPathSize(LetusProofPath* path);
bool LetusGetProofNodeIsData(LetusProofPath* path, uint64_t node_index);
int LetusGetProofNodeIndex(LetusProofPath* path, uint64_t node_index);
char* LetusGetProofNodeKey(LetusProofPath* path, uint64_t node_index);
char* LetusGetProofNodeHash(LetusProofPath* path, uint64_t node_index);
uint64_t LetusGetProofNodeSize(LetusProofPath* path, uint64_t node_index);
char* LetusGetINodeKey(LetusProofPath* path, uint64_t node_index,
                       uint64_t inode_index);
char* LetusGetINodeHash(LetusProofPath* path, uint64_t node_index,
                        uint64_t inode_index);
#endif  // _LETUS_H_
