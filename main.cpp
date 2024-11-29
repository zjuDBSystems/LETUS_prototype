#include "lib/LSVPS.hpp"
#include "lib/VDLS.hpp"
#include "lib/DMMTrie.hpp"
#include <iostream>
int main(){
    LSVPS* page_store = new LSVPS();
    VDLS* value_store = new VDLS();
    DMMTrie* trie = new DMMTrie(0, page_store, value_store);
    page_store->RegisterTrie(trie);

    trie->Put(0,1,"12345","1");
    trie->Put(0,2,"12345","2");
    trie->Put(0,3,"12345","3");
    trie->Put(0,4,"12345","4");
    trie->Put(0,5,"12345","5");
    trie->Put(0,6,"12345","6");
    trie->Put(0,7,"12345","7");
    trie->Put(0,8,"12345","8");
    trie->Put(0,9,"12345","9");
    trie->Put(0,10,"12345","10");
    trie->Put(0,11,"12345","11");
    trie->Put(0,12,"12345","12");
    trie->Put(0,13,"12345","13");
    trie->Put(0,14,"12345","14");
    trie->Put(0,15,"12345","15");
    trie->Put(0,16,"12345","16");
    trie->Put(0,17,"12345","17");
    trie->Put(0,18,"12345","18");
    trie->Put(0,19,"12345","19");
    trie->Put(0,20,"12345","20");
    trie->Put(0,21,"12345","21");
    trie->Put(0,22,"12345","22");
    trie->Put(0,23,"12345","23");
    trie->Put(0,24,"12345","24");
    trie->Put(0,25,"12345","25");
    trie->Put(0,26,"12345","26");
    trie->Put(0,27,"12345","27");
    trie->Put(0,28,"12345","28");
    trie->Put(0,29,"12345","29");
    trie->Put(0,30,"12345","30");
    trie->Put(0,31,"12345","31");
    trie->Put(0,32,"12345","32");
    trie->Put(0,33,"12345","33");
    trie->Put(0,34,"12345","34");
    trie->Put(0,35,"12345","35");
    trie->Put(0,36,"12345","36");
    trie->Put(0,37,"12345","37");
    trie->Put(0,38,"12345","38");
    trie->Put(0,39,"12345","39");
    trie->Put(0,40,"12345","40");
    trie->Put(0,41,"12345","41");
    trie->Put(0,42,"12345","42");
    trie->Put(0,43,"12345","43");
    trie->Put(0,44,"12345","44");
    trie->Put(0,45,"12345","45");
    trie->Put(0,46,"12345","46");
    trie->Put(0,47,"12345","47");
    trie->Put(0,48,"12345","48");
    trie->Put(0,49,"12345","49");
    trie->Put(0,50,"12345","50");

    trie->Put(0,51,"00151","}}}");
    trie->Put(0,51,"00008",":::");
    trie->Put(0,51,"02229","ggg");
    trie->Put(0,51,"46677","hhh");
    trie->Put(0,51,"00320","```");
    trie->Put(0,51,"00000","<<<");
    trie->Put(0,51,"00363","eee");
    trie->Put(0,51,"02452","^^^");
    trie->Put(0,51,"44407","FFF");
    trie->Put(0,51,"00350","qqq");
    trie->Put(0,51,"01649","%%%");
    trie->Put(0,51,"02917","yyy");
    trie->Put(0,51,"06065","777");
    trie->Put(0,51,"00030","ppp");
    trie->Put(0,51,"05659","ggg");
    trie->Put(0,51,"00048","666");
    trie->Put(0,51,"81626","UUU");
    trie->Put(0,51,"05456","KKK");
    trie->Put(0,51,"25949","FFF");
    trie->Put(0,51,"00115","qqq");
    trie->Put(0,51,"00191","LLL");
    trie->Put(0,51,"00016","```");
    trie->Put(0,51,"00004","&&&");

    trie->Get(0,1,"12345");


    return 0;
}