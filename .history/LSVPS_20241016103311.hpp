static constexpr int PAGE_SIZE = 12288;                  // size of a data page in byte 12KB
class Page {

private:
  char data_[PAGE_SIZE]{};
};

class LSVPS {
public:

}