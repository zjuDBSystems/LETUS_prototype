#ifndef _LETUS_H_
#define _LETUS_H_
#include <stdint.h>
typedef struct Letus Letus;
extern struct Letus* OpenLetus(const char* path_c);
void LetusPut(Letus* p, uint64_t tid, uint64_t version, const char* key_c,
              const char* value_c);
char* LetusGet(Letus* p, uint64_t tid, uint64_t version, const char* key_c);
void LetusCommit(Letus* p, uint64_t version);

#endif  // _LETUS_H_
