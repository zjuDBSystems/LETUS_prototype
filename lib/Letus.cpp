#ifndef _LETUS_CPP_
#define _LETUS_CPP_

extern "C" {
	#include "Letus.h"
}

#include "LSVPS.hpp"
#include "VDLS.hpp"
#include "DMMTrie.hpp"

struct Letus : DMMTrie{
    Letus():DMMTrie(0, new LSVPS(), new VDLS()) {
        LSVPS* page_store = dynamic_cast<LSVPS*>(DMMTrie::GetPageStore());
        page_store->RegisterTrie(this);
    }
};

Letus* OpenLetus(){
    auto p = new Letus();
    return p;
}

void LetusPut(Letus* p){
    p->Put(0,1,"12345","aaa");
}

char* LetusGet(Letus* p){
    string res_string = p->Get(0,1,"12345");
    size_t res_size = res_string.size(); 
    char* res = new char [res_size+1];
    res_string.copy(res, res_size, 0);
    res[res_size] = '\0';
    return res;
} 

#endif // _LETUS_CPP_