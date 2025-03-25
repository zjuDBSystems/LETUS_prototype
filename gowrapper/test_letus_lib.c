#include <stdio.h>

#include "../lib/Letus.h"

int main() {
  Letus *p = OpenLetus("data");

  // char value[11] = "1234567890";
  char value[400] =
      "{\"name\":\"南风\",\"age\":28,\"location\":{\"city\":\"杭州\","
      "\"country\":\"中国\"},\"hobbies\":[\"读书\",\"旅行\",\"编程\"],"
      "\"isActive\":true,\"scores\":{\"math\":95,\"english\":88,\"science\":92}"
      ",\"createdAt\":\"2024-12-30T14:52:00Z\",\"tags\":[\"随机\",\"JSON\","
      "\"生成\"],\"preferences\":{\"notifications\":true,\"theme\":\"dark\","
      "\"languages\":[\"中文\",\"English\"]},\"friends\":[{\"id\":1,\"name\":"
      "\"李华\",\"relationship\":\"同事\"},{\"id\":2,\"name\":\"王芳\","
      "\"relationship\":\"朋友\"}]}";
  int total = 20;
  for (int i = 0; i < total; ++i) {
    for (int j = 0; j < 500; ++j) {
      LetusPut(p, 0, 1, "111111111111111111111111111111111", value);
    }
    LetusCalcRootHash(p, 0, i);
  }
  // char *s = LetusGet(p, 0, 1, "11111");
  // printf("%s\n", s);
  // int proof_size = 0;
  // LetusProofPath *proof_path = LetusProof(p, 0, 1, "11111");
  // uint64_t proof_path_size = LetusGetProofPathSize(proof_path);
  // for (int i = 0; i < proof_path_size; ++i) {
  //   printf("key: %s\n", LetusGetProofNodeKey(proof_path, i));
  //   printf("value: %s\n", LetusGetProofNodeHash(proof_path, i));
  // }
  return 0;
}