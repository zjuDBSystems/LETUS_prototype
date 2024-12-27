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
  LetusProofNode *proof_nodes;
  LetusProof(p, 0, 1, "11111", &proof_nodes, &proof_size);
  for (int i = 0; i < proof_size; ++i) {
    printf("key: %s\n", proof_nodes[i].key);
    printf("value: %s\n", proof_nodes[i].hash);
  }
  return 0;
}