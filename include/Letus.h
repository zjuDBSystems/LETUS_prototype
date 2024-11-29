#ifndef _LETUS_H_
#define _LETUS_H_


#ifdef __cplusplus
extern "C" {
#endif

typedef struct  DMMTrie DMMTrie;
DMMTrie* OpenLetus();
void LetusPut(DMMTrie* trie);
char* LetusGet(DMMTrie* trie);

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif // _LETUS_H_