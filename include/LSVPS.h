#ifndef _LSVPS_H_
#define _LSVPS_H_

class Page;
class DMMTrie;
class LSVPSInterface;

// LSVPS类定义
class LSVPS : public LSVPSInterface {
 public:
  LSVPS();
  void RegisterTrie(DMMTrie *DMM_trie);
  Page *LoadPage(const PageKey& pagekey);
  void StorePage(Page* page);
};

#endif