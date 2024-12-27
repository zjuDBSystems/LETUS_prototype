#include <stdio.h>

#include "../lib/Letus.h"

int main() {
  Letus *p = OpenLetus("");
  LetusPut(p, 0, 1, "11111", "aaaaa");
  LetusPut(p, 0, 1, "11112", "aaaaa");
  LetusPut(p, 0, 1, "11113", "aaaaa");
  LetusCommit(p, 1);
  char *s = LetusGet(p, 0, 1, "11111");
  printf("%s\n", s);
  int proof_size = 0;
  LetusProofPath *proof_path = LetusProof(p, 0, 1, "11111");
  uint64_t proof_path_size = LetusGetProofPathSize(proof_path);
  for (int i = 0; i < proof_path_size; ++i) {
    printf("key: %s\n", LetusGetProofNodeKey(proof_path, i));
    printf("value: %s\n", LetusGetProofNodeHash(proof_path, i));
  }
  return 0;
}