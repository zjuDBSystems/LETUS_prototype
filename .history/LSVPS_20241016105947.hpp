#include <string>
#include <vector>
static constexpr int PAGE_SIZE = 12288;                  // size of a data page in byte 12KB

class Page {//K_p Version Tid delta/base Pid

private:
  char data_[PAGE_SIZE]{};
  int latestBasePageVersion;//used to obtain the starting BasePage
  int currentDataVersion;//used to obtain the latest DeltaPage
};

struct PageKey { //key to page
    int version;
    int Tid;
    bool t_P;
    string Pid;
};

class BasePage : public Page {

};
class DeltaPage : public Page {

};

class LSVPS {
public:
  Page PageQuery(int version){}
  Page LoadPage(PageKey Kp){
    PageLookUp(PageKey Kp);
    PageReplay(vector );
  }
  void StorePage(Page page){} //generate page to store in the MemIndexTable

private:
  Page PageLookup(PageKey Kp){}
  void PageReplay(){}
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