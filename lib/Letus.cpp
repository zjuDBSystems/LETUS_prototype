#ifndef _LETUS_CPP_
#define _LETUS_CPP_

extern "C" {
	#include "Letus.h"
}

#include <iostream>
#include <string>
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

void LetusPut(Letus* p, const char* key_c, const char* value_c){
    std::string key(key_c);
    std::string value(value_c);
    // std::cout << key << ", " << value << std::endl;
    p->trie->Put(0, 1, key, value);
}

char* LetusGet(Letus* p, const char* key_c){
    std::string key(key_c);
    std::string value = p->trie->Get(0, 1, key);
    size_t value_size = value.size(); 
    char* value_c = new char [value_size+1];
    value.copy(value_c, value_size, 0);
    value_c[value_size] = '\0';
    return value_c;
} 

#endif // _LETUS_CPP_