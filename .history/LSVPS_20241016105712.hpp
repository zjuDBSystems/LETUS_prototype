#include <string>
static constexpr int PAGE_SIZE = 12288;                  // size of a data page in byte 12KB

class Page {//K_p Version Tid delta/base Pid

private:
  char data_[PAGE_SIZE]{};
  int latestBasePageVersion;//used to obtain the starting BasePage
  int currentDataVersion;//used to obtain the latest DeltaPage
};

struct Kp { //key to page
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
  Page LoadPage(K_p){
    PageLookUp();
    PageReplay();
  }
  void StorePage(){} //generate page to store in the MemIndexTable

private:
  Page PageLookup(){}
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