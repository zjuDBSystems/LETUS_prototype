#include <iostream>
#include "DMMTrie.hpp"
#include "LSVPS.hpp"
#include "VDLS.hpp"

int main() {
  LSVPS* page_store = new LSVPS();
  VDLS* value_store = new VDLS();
  DMMTrie* trie = new DMMTrie(0, page_store, value_store);
  page_store->RegisterTrie(trie);
  trie->Put(0, 1, "12345", "aaa");
  std::cout << trie->Get(0, 1, "12345") << std::endl;
  return 0;
}