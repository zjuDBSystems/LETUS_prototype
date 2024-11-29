#ifndef _LETUS_CPP_
#define _LETUS_CPP_

#include "Letus.h"
#include "LSVPS.hpp"
#include "VDLS.hpp"
#include "DMMTrie.hpp"

DMMTrie* OpenLetus(){
    LSVPS* page_store = new LSVPS();
    VDLS* value_store = new VDLS();
    DMMTrie* trie = new DMMTrie(0, page_store, value_store);
    page_store->RegisterTrie(trie);
    return trie;
}

void LetusPut(DMMTrie* trie){
    trie->Put(0,1,"12345","aaa");
}

char* LetusGet(DMMTrie* trie){
    string res_string = trie->Get(0,1,"12345");
    size_t res_size = res_string.size(); 
    char* res = new char [res_size+1];
    res_string.copy(res, res_size, 0);
    res[res_size] = '\0';
    return res;
} 

#endif // _LETUS_CPP_