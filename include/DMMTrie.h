#ifndef _DMMTRIE_H_
#define _DMMTRIE_H_

#include <cstring>
#include <stdint>
using namespace std;


class VDLS;
class LSVPSInterface;

class DMMTrie {
 public:
  DMMTrie(uint64_t tid, LSVPSInterface *page_store, VDLS *value_store, uint64_t current_version = 0);
  bool Put(uint64_t tid, uint64_t version, const string &key, const string &value);
  string Get(uint64_t tid, uint64_t version, const string &key);
};
  

#endif