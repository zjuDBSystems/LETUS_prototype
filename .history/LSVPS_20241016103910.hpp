static constexpr int PAGE_SIZE = 12288;                  // size of a data page in byte 12KB
class Page {

private:
  char data_[PAGE_SIZE]{};
  int latestBasePageVersion;//used to obtain the starting BasePage
  int currentDataVersion;//used to obtain the latest DeltaPage
};

class LSVPS {
public:
  Page LoadPage(){}


private:
  class blockCache{

};
  blockCache cache;
}