#ifndef _LETUS_CPP_
#define _LETUS_CPP_

extern "C" {
	#include "Letus.h"
}

#include <iostream>
#include "LSVPS.hpp"
#include "VDLS.hpp"
#include "DMMTrie.hpp"

struct Letus{
    DMMTrie* trie;
};

struct Letus *OpenLetus(){
    LSVPS* page_store = new LSVPS();
    VDLS* value_store = new VDLS();
    DMMTrie* trie = new DMMTrie(0, page_store, value_store);
    page_store->RegisterTrie(trie);
    struct Letus* p = new Letus();
    p->trie = trie;
    return p;
}

void LetusPut(Letus* p){
    p->trie->Put(0,1,"12345","aaa");
    std::cout << "aaa" << std::endl;
}

char* LetusGet(Letus* p){
    string res_string = p->trie->Get(0,1,"12345");
    size_t res_size = res_string.size(); 
    char* res = new char [res_size+1];
    res_string.copy(res, res_size, 0);
    res[res_size] = '\0';
    return res;
} 

#endif // _LETUS_CPP_