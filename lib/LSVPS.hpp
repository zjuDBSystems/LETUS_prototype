#ifndef _LSVPS_H_
#define _LSVPS_H_

#include <cstdint>
#include <string>
#include <vector>

#include "DMMTrie.hpp"
#include "utils.hpp"

// 索引块结构体
struct IndexBlock {
  static constexpr size_t OS_PAGE_SIZE = 4096;  // 4KB

  struct Mapping {
    PageKey pagekey;
    uint64_t location;
  };

  static constexpr size_t MAPPINGS_PER_BLOCK =
      (OS_PAGE_SIZE - sizeof(size_t)) / sizeof(Mapping);

  IndexBlock();
  bool AddMapping(const PageKey &pagekey, uint64_t location);
  bool IsFull() const;
  const std::vector<Mapping> &GetMappings() const;
  bool SerializeTo(std::ofstream &out) const;
  bool Deserialize(std::ifstream &in);

 private:
  std::vector<Mapping> mappings_;
};

// 索引文件结构体
struct IndexFile {
  PageKey min_pagekey;
  PageKey max_pagekey;
  std::string filepath;
};

// 查找块结构体
struct LookupBlock {
  static const size_t BLOCK_SIZE = 4096;  // 4KB
  
  std::vector<std::pair<PageKey, size_t>> entries;
  bool SerializeTo(std::ostream &out) const;
  bool Deserialize(std::istream &in);
};

// LSVPS类定义
class LSVPS {
 public:
  LSVPS();
  Page *PageQuery(uint64_t version);
  Page *LoadPage(const PageKey &pagekey);
  void StorePage(Page *page);
  void AddIndexFile(const IndexFile &index_file);
  int GetNumOfIndexFile();
  void RegisterTrie(DMMTrie *DMM_trie);

 private:
  // 块缓存类（占位）
  class blockCache {};

  // 内存索引表类声明
  class MemIndexTable {
   public:
    explicit MemIndexTable(LSVPS &parent);
    const std::vector<Page *> &GetBuffer() const;
    void Store(Page *page);
    bool IsFull() const;
    void Flush();

   private:
    void writeToStorage(const std::vector<IndexBlock> &index_blocks,
                        const LookupBlock &lookup_blocks,
                        const std::filesystem::path &filepath);
    std::vector<Page *> buffer_;
    const size_t max_size_ = 2000;//gurantee that max_size >= one version pages
    LSVPS &parent_LSVPS_;
  };

  Page *pageLookup(const PageKey &pagekey, bool isPrecise);
  Page *readPageFromIndexFile(std::vector<IndexFile>::const_iterator file_it, const PageKey &pagekey, bool isPrecise);
  void applyDelta(BasePage *basepage, const DeltaPage *deltapage,
                  PageKey pagekey);

  blockCache cache_;
  MemIndexTable table_;
  DMMTrie *trie_;
  std::vector<IndexFile> index_files_;
};

#endif