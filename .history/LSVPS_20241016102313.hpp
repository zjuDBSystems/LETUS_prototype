static constexpr int PAGE_SIZE = 4096;                  // size of a data page in byte
class Page {
private:
  char data_[PAGE_SIZE]{};
};