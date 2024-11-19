#ifndef _LSVPS_H_
#define _LSVPS_H_
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "DMMTrie.hpp"
#include "utils.hpp"

namespace fs = std::filesystem;
// 假设： 同一个版本 同一个k只有一个v
// 可能的优化：#120可能在DMM-Trie的cache里面 #121就很容易replay但是在LSVPSreplay就需要#100basepage+21个deltapage

// 页面块类型枚举
enum class PageBlockType { BasePage, DeltaPage };

// 页面块结构体
struct PageBlock {
  PageBlockType type;
  Page data;

  // Serialization function
  void SerializeTo(std::ostream &out) const {
    out.write(reinterpret_cast<const char *>(&type), sizeof(type));
    data.SerializeTo(out);
  }

  // Deserialization function
  void Deserialize(std::istream &in) {
    in.read(reinterpret_cast<char *>(&type), sizeof(type));
    data.Deserialize(in);
  }
};

// 索引块结构体
struct IndexBlock {
  static constexpr size_t OS_PAGE_SIZE = 4096;  // 4KB

  struct Mapping {
    PageKey pagekey;
    uint64_t location;
  };

  // 计算一个4KB的块能存储多少个映射
  static constexpr size_t MAPPINGS_PER_BLOCK = (OS_PAGE_SIZE - sizeof(size_t)) / sizeof(Mapping);

  IndexBlock() { mappings_.reserve(MAPPINGS_PER_BLOCK); }

  bool AddMapping(const PageKey &pagekey, uint64_t location) {
    if (mappings_.size() >= MAPPINGS_PER_BLOCK) {
      return false;
    }
    mappings_.push_back({pagekey, location});
    return true;
  }

  bool IsFull() const { return mappings_.size() >= MAPPINGS_PER_BLOCK; }

  const std::vector<Mapping> &GetMappings() const { return mappings_; }

  void SerializeTo(std::ofstream &out) const {
    // 写入映射数量
    size_t count = mappings_.size();
    out.write(reinterpret_cast<const char *>(&count), sizeof(count));

    // 写入所有映射
    for (const auto &mapping : mappings_) {
      mapping.pagekey.SerializeTo(out);
      out.write(reinterpret_cast<const char *>(&mapping.location), sizeof(mapping.location));
    }

    // 填充剩余空间以对齐到4KB
    char padding[OS_PAGE_SIZE] = {0};
    size_t written_size = sizeof(count) + count * sizeof(Mapping);
    size_t padding_size = OS_PAGE_SIZE - written_size;
    out.write(padding, padding_size);
  }

  void Deserialize(std::ifstream &in) {
    // 读取映射数量
    size_t count;
    in.read(reinterpret_cast<char *>(&count), sizeof(count));

    // 读取所有映射
    mappings_.clear();
    mappings_.reserve(count);
    for (size_t i = 0; i < count; ++i) {
      Mapping mapping;
      mapping.pagekey.Deserialize(in);
      in.read(reinterpret_cast<char *>(&mapping.location), sizeof(mapping.location));
      mappings_.push_back(mapping);
    }

    // 跳过填充字节
    size_t read_size = sizeof(count) + count * sizeof(Mapping);
    size_t padding_size = OS_PAGE_SIZE - read_size;
    in.seekg(padding_size, std::ios::cur);
  }

 private:
  std::vector<Mapping> mappings_;
};

// 索引文件结构体
struct IndexFile {
  PageKey minKP;
  PageKey maxKP;
  std::string filepath;
};

// 查找块结构体
struct LookupBlock {
  std::vector<std::pair<PageKey, size_t>> entries;

  // Serialization function
  void SerializeTo(std::ostream &out) const {
    size_t entriesSize = entries.size();
    out.write(reinterpret_cast<const char *>(&entriesSize), sizeof(entriesSize));
    for (const auto &entry : entries) {
      entry.first.SerializeTo(out);
      out.write(reinterpret_cast<const char *>(&entry.second), sizeof(entry.second));
    }
  }

  // Deserialization function
  void Deserialize(std::istream &in) {
    size_t entriesSize;
    in.read(reinterpret_cast<char *>(&entriesSize), sizeof(entriesSize));
    entries.resize(entriesSize);
    for (auto &entry : entries) {
      entry.first.Deserialize(in);
      in.read(reinterpret_cast<char *>(&entry.second), sizeof(entry.second));
    }
  }
};

// LSVPS类定义
class LSVPS : public LSVPSInterface {
 public:
  LSVPS() : cache_(), table_(*this) {}

  // 页面查询函数（用于实现范围查询）
  Page *PageQuery(int version) {
    // unimplemented
    return nullptr;
  }

  // 加载页面函数
  Page *LoadPage(const PageKey &pagekey) {
    // 步骤1：页面查找
    // Page page = PageLookup(pagekey);

    // // 步骤2：页面重放
    // std::vector<Page> pages = {/* 基础页和增量页的集合 */};
    // // 假设已经获取了相关的页面列表

    // PageReplay(pages);

    // 返回最终页面
    return nullptr;
  }

  // 存储页面函数
  void StorePage(const Page &page) {
    table_.store(page);
    if (table_.IsFull()) {
      table_.flush();
    }
  }

  void AddIndexFile(const IndexFile &index_file) { indexFiles_.push_back(index_file); }

  int GetNumOfIndexFile() { return indexFiles_.size(); }

  void RegisterTrie(const DMMTrie *DMM_trie) { trie_ = DMM_trie; }

 private:
  // 页面查找函数
  Page PageLookup(const PageKey &pagekey) {
    // 步骤1：在内存索引表中查找
    const auto &buffer = table_.getBuffer();
    auto it =
        std::find_if(buffer.begin(), buffer.end(), [&](const Page &page) { return page.GetPageKey() == pagekey; });

    if (it != buffer.end()) {
      return *it;  // 在内存中找到页面
    }

    // 步骤2：根据索引文件查找
    auto fileIt = std::find_if(indexFiles_.begin(), indexFiles_.end(),
                               [&](const IndexFile &file) { return file.minKP <= pagekey && pagekey <= file.maxKP; });

    if (fileIt == indexFiles_.end()) {
      throw std::runtime_error("Page not found in any index file.");
    }

    // 步骤3：读取基础页和增量页
    std::vector<Page> pages = readPagesFromIndexFile(*fileIt, pagekey);

    // 步骤4：重放增量，构建最终页面
    Page finalPage = PageReplay(pages);

    return finalPage;
  }

  // 读取索引文件中的页面（需要实现）
  std::vector<Page> readPagesFromIndexFile(const IndexFile &index_file, const PageKey &pagekey) {
    // 实现读取基础页和增量页的逻辑
    // 要根据index结构读取对应的页面
    Page page = readPageFromIndexFile(index_file, pagekey);
    return {page};
  }

  // 从索引文件中读取页面
  Page readPageFromIndexFile(const IndexFile &index_file, const PageKey &pagekey) {
    std::ifstream inFile(index_file.filepath, std::ios::binary);
    if (!inFile) {
      throw std::runtime_error("Error opening index file.");
    }

    // Step 1: Use the lookup block to find the IndexBlock location
    LookupBlock lookup_blocks;
    inFile.seekg(-static_cast<std::streamoff>(sizeof(LookupBlock)), std::ios::end);
    lookup_blocks.Deserialize(inFile);

    // Step 2: Find IndexBlock location using lookup block
    size_t index_block_location = findIndexBlockLocation(lookup_blocks, pagekey, inFile);

    // Step 3: Read IndexBlock
    inFile.seekg(index_block_location);
    IndexBlock indexBlock;
    indexBlock.Deserialize(inFile);

    // Step 4: Read PageBlock using location from IndexBlock
    auto mappings = indexBlock.GetMappings();
    auto mapping = std::find_if(mappings.begin(), mappings.end(),
                                [&pagekey](const IndexBlock::Mapping &m) { return m.pagekey == pagekey; });
    if (mapping == mappings.end()) {
      throw std::runtime_error("Page not found in index block.");
    }
    inFile.seekg(mapping->location);
    PageBlock pageBlock;
    pageBlock.Deserialize(inFile);

    return pageBlock.data;
  }

  // Helper function to find IndexBlock location
  size_t findIndexBlockLocation(const LookupBlock &lookup_blocks, const PageKey &pagekey, std::ifstream &inFile) {
    // Implement binary search over lookup_blocks.entries to find the right IndexBlock
    // Return the location of the IndexBlock in the file
    // For simplification, here's a linear search example:
    size_t index_block_location = 0;
    for (const auto &entry : lookup_blocks.entries) {
      if (entry.first >= pagekey) {
        index_block_location = entry.second;
        break;
      }
    }
    return index_block_location;
  }

  // 页面重放函数（需要实现）
  Page PageReplay(const std::vector<Page> &pages) {
    if (pages.empty()) {
      throw std::runtime_error("No pages to replay");
    }

    // First page should be a base page
    Page result_page = pages[0];

    // Apply delta pages in order
    for (size_t i = 1; i < pages.size(); i++) {
      applyDelta(result_page, pages[i]);
    }

    return result_page;
  }

  // 添加辅助方法来应用deltapage
  void applyDelta(Page &basepage, const Page &deltapage) {
    // char* baseData = basePage.getData();
    // const char* deltaData = deltaPage.getData();
    // // delatapage：<offset><length><new_data>
    // size_t offset = 0;
    // while (offset < PAGE_SIZE) {
    //     // 读取修改信息并应用

    // }
  }

  // 写入二级存储
  void writeToStorage(const std::vector<PageBlock> &page_blocks, const std::vector<IndexBlock> &index_blocks,
                      const LookupBlock &lookup_blocks, const fs::path &filepath) {
    std::ofstream outFile(filepath, std::ios::binary);
    if (!outFile) {
      std::cerr << "Error opening file for writing." << std::endl;
      return;
    }

    size_t offset = 0;

    // Write page blocks
    for (const auto &pageBlock : page_blocks) {
      outFile.write(reinterpret_cast<const char *>(&pageBlock.type), sizeof(pageBlock.type));
      pageBlock.data.SerializeTo(outFile);
      offset += sizeof(pageBlock.type) + pageBlock.data.GetSerializedSize();
    }

    // Write index blocks
    for (const auto &indexBlock : index_blocks) {
      for (const auto &mapping : indexBlock.GetMappings()) {
        mapping.pagekey.SerializeTo(outFile);
        outFile.write(reinterpret_cast<const char *>(&mapping.location), sizeof(mapping.location));
        offset += sizeof(int) + sizeof(int) + sizeof(bool) + sizeof(size_t) + mapping.pagekey.pid.size();  // PageKey
      }
    }

    // Write lookup block
    lookup_blocks.SerializeTo(outFile);

    outFile.close();
  }

  // 块缓存类（占位）
  class blockCache {
    // 实现缓存逻辑，例如LRU缓存
  };

  // 内存索引表类
  class memIndexTable {
   public:
    memIndexTable(LSVPS &parent) : parentLSVPS_(parent) {}

    const std::vector<Page> &getBuffer() const { return buffer_; }

    void store(const Page &page) { buffer_.push_back(page); }

    bool IsFull() const { return buffer_.size() >= maxSize_; }

    void flush() {
      if (buffer_.empty()) return;

      // Create PageBlocks from Pages
      std::vector<PageBlock> page_blocks;
      page_blocks.reserve(buffer_.size());

      // Convert Pages to PageBlocks
      for (const auto &page : buffer_) {
        PageBlock block;
        block.data = page;
        // Determine if it's BasePage or DeltaPage based on version
        block.type = isBasePage(page) ? PageBlockType::BasePage : PageBlockType::DeltaPage;
        page_blocks.push_back(block);
      }

      // Create index blocks
      std::vector<IndexBlock> index_blocks;
      IndexBlock currentBlock;
      uint64_t currentLocation = 0;

      for (size_t i = 0; i < page_blocks.size(); i++) {
        if (currentBlock.IsFull()) {
          index_blocks.push_back(currentBlock);
          currentBlock = IndexBlock();
        }

        currentBlock.AddMapping(buffer_[i].GetPageKey(), currentLocation);
        currentLocation += sizeof(PageBlockType) + buffer_[i].GetSerializedSize();
      }

      // Add the last index block if it contains any mappings
      if (!currentBlock.GetMappings().empty()) {
        index_blocks.push_back(currentBlock);
      }

      // Create lookup block
      LookupBlock lookup_blocks;
      uint64_t indexBlockOffset = currentLocation;
      for (const auto &block : index_blocks) {
        if (!block.GetMappings().empty()) {
          lookup_blocks.entries.push_back({block.GetMappings()[0].pagekey, indexBlockOffset});
          indexBlockOffset += block.GetMappings().size() * sizeof(IndexBlock::Mapping);
        }
      }
      std::string filepath = "index_" + std::to_string(parentLSVPS_.GetNumOfIndexFile()) + ".dat";

      // Write to storage
      parentLSVPS_.writeToStorage(page_blocks, index_blocks, lookup_blocks, filepath);

      // Clear buffer
      buffer_.clear();
    }

   private:
    std::vector<Page> buffer_;
    const size_t maxSize_ = 256 * 1024 * 1024 / sizeof(Page);  // 假设最大大小为256MB

    PageKey minKP_;
    PageKey maxKP_;

    LSVPS &parentLSVPS_;

    // 辅助函数：判断页面是否为基础页
    bool isBasePage(const Page &page) {
      // 根据页面的属性判断
      return true;  // 占位实现
    }
  };

  // // B树索引类（占位）
  // class BTreeIndex {
  //     // 实现B树索引的逻辑
  // };

  blockCache cache_;
  memIndexTable table_;
  const DMMTrie *trie_;

  std::vector<IndexFile> indexFiles_;
};

#endif