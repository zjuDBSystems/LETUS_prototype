static constexpr int PAGE_SIZE = 12288;                  // size of a data page in byte 12KB

class Page {//K_p Version Tid delta/base Pid

private:
  char data_[PAGE_SIZE]{};
  int latestBasePageVersion;//used to obtain the starting BasePage
  int currentDataVersion;//used to obtain the latest DeltaPage
};

class BasePage : public Page {

};
class DeltaPage : public Page {

};

class LSVPS {
public:
  Page PageQuery(int version){}
  Page LoadPage(){
    LookUp();
    Replay();
  }
  void StorePage(){} //generate page to store in the MemIndexTable

private:
  Page Lookup(){}
  void Replay(){}
  class blockCache{ //expedite page loading

  };
  class memIndexTable{ //buffer the pages MaxSize: 256MB
    void flush();
  };
  class 
  blockCache cache;
  memIndexTable table;
}