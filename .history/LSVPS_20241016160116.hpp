#include <string>
#include <vector>

static constexpr int PAGE_SIZE = 12288;                  // size of a data page in byte 12KB two-level subtree
//KeyData (version, key, value)
class Page {
private:
  char data_[PAGE_SIZE]{};
  int latestBasePageVersion;//used to obtain the starting BasePage
  int currentDataVersion;//used to obtain the latest DeltaPage
};

struct PageKey { //key to page
    int version;//64-bit integer
    int Tid;// ID of the DMM-Trie
    bool t_P;// delta or base page
    string Pid;//nibble e.g. 'Al' (in 'Alice')
};


class LSVPS {
public:
  Page PageQuery(int version){}
  Page LoadPage(PageKey Kp){
    PageLookUp(PageKey Kp);
    //put pages(incluing base and delta) in a vector
    PageReplay(std::vector<Page> pages);
  }
  void StorePage(Page page){} //generate page to store in the MemIndexTable

private:
  Page PageLookup(PageKey Kp){}
  void PageReplay(std::vector<Page> pages){}
  class blockCache{ //expedite page loading

  };
  class memIndexTable{ //buffer the pages MaxSize: 256MB
    void flush();
  };
  class BTreeIndex{
  };
  blockCache cache;
  memIndexTable table;
}