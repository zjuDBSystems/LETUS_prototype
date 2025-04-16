#include <openssl/sha.h>

#include <iostream>

#include "DMMTrie.hpp"
#include "LSVPS.hpp"
#include "VDLS.hpp"

string make_key(const std::string& key) {
  // unsigned char hash[SHA_DIGEST_LENGTH];
  // SHA1(reinterpret_cast<const unsigned char*>(key.c_str()), key.size(),
  // hash);

  // std::ostringstream hashString;
  // for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
  //   hashString << std::hex << std::setw(2) << std::setfill('0') <<
  //   (int)hash[i];
  // }
  // // cout << hashString.str() << endl;
  // return hashString.str();
  return key;
}

int main() {
  LSVPS* page_store = new LSVPS();
  std::string data_path;
  // data_path = "/Users/ldz/Code/miniLETUS/data/";//your own path
  data_path = "/home/xinyu.chen/LETUS_prototype/data";
  VDLS* value_store = new VDLS(data_path);
  DMMTrie* trie = new DMMTrie(0, page_store, value_store);
  page_store->RegisterTrie(trie);
  trie->Put(0, 1, make_key("12345"), "aaa");
  trie->Put(0, 1, make_key("23456"), "bbb");
  trie->Put(0, 1, make_key("34567"), "ccc");
  trie->Put(0, 1, make_key("45678"), "ddd");
  trie->Put(0, 1, make_key("56789"), "eee");
  trie->Commit(1);
  trie->Put(0, 2, make_key("11111"), "aa2");
  trie->Put(0, 2, make_key("23456"), "bb2");
  trie->Put(0, 2, make_key("34567"), "cc2");
  trie->Put(0, 2, make_key("45678"), "dd2");
  trie->Put(0, 2, make_key("56789"), "ee2");
  trie->Commit(2);
  DMMTrieProof proof = trie->GetProof(0, 2, make_key("12345"));
  string root_hash1 = trie->GetRootHash(0, 1);
  string root_hash2 = trie->GetRootHash(0, 2);
  cout << boolalpha
       << trie->Verify(0, make_key("12345"), "aaa", root_hash2, proof) << endl;
  cout << boolalpha
       << trie->Verify(0, make_key("12345"), "bbb", root_hash2, proof) << endl;
  cout << boolalpha << trie->Verify(0, 1, root_hash1) << endl;

  cout << boolalpha << trie->Verify(0, 1, root_hash1)
       << endl;  // 在此打断点手动修改data_file_0中数据
  trie->Delete(0, 3, make_key("12345"));
  trie->Commit(3);
  cout << trie->Get(0, 3, make_key("12345")) << endl;
  cout << trie->Get(0, 1, make_key("28294")) << endl;  // not exist key
  cout << trie->GetProof(0, 1, make_key("28294")).value
       << endl;  // not exist key
  cout << trie->GetProof(0, 3, make_key("28294")).value
       << endl;  // not exist key
}