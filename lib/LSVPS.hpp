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


// 索引块结构体
struct IndexBlock {
  static constexpr size_t OS_PAGE_SIZE = 4096;  // 4KB

  struct Mapping {
    PageKey pagekey;
    uint64_t location;
  };

  // 计算一个4KB的块能存储多少个映射，减去一开始存储的映射数量
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
  PageKey min_pagekey;
  PageKey max_pagekey;
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
  Page *LoadPage(const PageKey& pagekey) {
    // 获取最近的 basepage
    PageKey base_pagekey = GetLatestBasePageKey(pagekey);
    
    // 如果找不到基础页面，返回 nullptr
    if (base_pagekey.version == 0) {
        return nullptr;
    }

    // 加载 basepage
    Page* result = PageLookup(base_pagekey);
    if (!result) {
        return nullptr;
    }

    // 先收集所有需要的 delta pages
    std::vector<Page*> delta_pages;
    PageKey current_delta = base_pagekey;
    delta_pages.push_back(active_deltapage);//!!!active_delta_page
    while (current_delta < pagekey) {
        Page* delta = PageLookup(current_delta);
        if (delta) {
            delta_pages.push_back(delta);
            current_delta = delta->GetNextPageKey();  // 获取上一个 delta page
        } else {
            break;
        }
    }

    // 按照时间顺序（从旧到新）应用 delta pages
    for (const auto& delta : delta_pages) {
        ApplyDelta(result, delta);
    }

    return result;
  }

  // 存储页面函数
  void StorePage(Page* page) {
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
  Page* PageLookup(const PageKey &pagekey) {
    // 步骤1：在内存索引表中查找
    auto &buffer = table_.getBuffer();
    for(const auto& page : buffer){
      if(page->GetPageKey() == pagekey){
        return page;
      }
    }

    // 步骤2：根据索引文件查找
    auto file_iterator = std::find_if(indexFiles_.begin(), indexFiles_.end(),
                               [&pagekey](const IndexFile &file) { return file.min_pagekey <= pagekey && pagekey <= file.max_pagekey; });

    if (file_iterator == indexFiles_.end()) {
      throw std::runtime_error("Page not found in any index file.");
    }
    
    // 步骤3：读取基础页和增量页
    Page* page = readPageFromIndexFile(*file_iterator, pagekey);
  
    return page;
  }

  // 从索引文件中读取页面
  Page* readPageFromIndexFile(const IndexFile& file, const PageKey& pagekey) {
    std::ifstream in_file(file.filepath, std::ios::binary);
    if (!in_file) {
      throw std::runtime_error("Failed to open index file: " + file.filepath);
    }

    // Read lookup block from end of file
    LookupBlock lookup_block;
    in_file.seekg(-sizeof(LookupBlock), std::ios::end);
    lookup_block.Deserialize(in_file);

    // Find the relevant index block
    auto it = std::lower_bound(lookup_block.entries.begin(), lookup_block.entries.end(),
                              std::make_pair(pagekey, size_t(0)));
    
    if (it == lookup_block.entries.end()) {
      return nullptr;
    }

    // Read the index block
    in_file.seekg(it->second);
    IndexBlock index_block;
    index_block.Deserialize(in_file);

    // Find the page location
    auto mapping = std::find_if(index_block.GetMappings().begin(), index_block.GetMappings().end(),
                               [&pagekey](const auto& m) { return m.pagekey == pagekey; });

    if (mapping == index_block.GetMappings().end()) {
      return nullptr;
    }

    // Read the actual page data
    in_file.seekg(mapping->location);
    Page* page = (pagekey.type) ? 
                 static_cast<Page*>(new BasePage()) : 
                 static_cast<Page*>(new DeltaPage());
    page->Deserialize(in_file);//反序列化函数

    return page;
  }

  // 添加辅助方法来应用deltapage
  void ApplyDelta(Page *basepage, Page *deltapage) {
    BasePage* base_page = dynamic_cast<BasePage*>(basepage);//cast
    DeltaPage* delta_page = dynamic_cast<DeltaPage*>(deltapage);//cast
    for(auto deltapage_item : *delta_page){  //!!!!GetDelataPageItemVector
    //check if version is beyond the needed pagekey (active delta page)
      base_page->update(deltapage_item);
    }
  }

  // 块缓存类（占位）
  class blockCache {
    // 实现缓存逻辑，例如LRU缓存
  };

  // 内存索引表类
  class memIndexTable {
   public:
    memIndexTable(LSVPS &parent) : parentLSVPS_(parent) {}

    const std::vector<Page*> &getBuffer() const { return buffer_; }

    void store(Page* page) { buffer_.push_back(page); }

    bool IsFull() const { return buffer_.size() >= maxSize_; }

    void flush() {
      if (buffer_.empty()) return;
      // Create index blocks
      std::vector<IndexBlock> index_blocks;
      IndexBlock current_block;
      uint64_t current_location = 0;

      for (size_t i = 0; i < buffer_.size(); i++) {
        if (current_block.IsFull()) {
          index_blocks.push_back(current_block);
          current_block = IndexBlock();
        }

        current_block.AddMapping(buffer_[i]->GetPageKey(), current_location);
        current_location += PAGE_SIZE;
      }

      // Add the last index block if it contains any mappings
      if (!current_block.GetMappings().empty()) {
        index_blocks.push_back(current_block);
      }

      // Create lookup block
      LookupBlock lookup_blocks;
      uint64_t indexBlockOffset = current_location;
      for (const auto &block : index_blocks) {
        if (!block.GetMappings().empty()) {
          lookup_blocks.entries.push_back({block.GetMappings()[0].pagekey, indexBlockOffset});
          indexBlockOffset += block.GetMappings().size() * sizeof(IndexBlock::Mapping);
        }
      }
      std::string filepath = "index_" + std::to_string(parentLSVPS_.GetNumOfIndexFile()) + ".dat";

      // Write to storage
      this->writeToStorage(index_blocks, lookup_blocks, filepath);
      parentLSVPS_.AddIndexFile({
          buffer_.front()->GetPageKey(),                    // min_pagekey
          buffer_.back()->GetPageKey(),                     // max_pagekey
          filepath  // filepath
      });
      // Clear buffer
      buffer_.clear();
    }

   private:
   // 辅助函数：判断页面是否为基础页
    bool isBasePage(const Page &page) {
      // 根据页面的属性判断
      return true;  // 占位实现
    }
    // 写入二级存储
    void writeToStorage(const std::vector<IndexBlock> &index_blocks,
                      const LookupBlock &lookup_blocks, const fs::path &filepath) {
      std::ofstream outFile(filepath, std::ios::binary);
      if (!outFile) {
        throw std::runtime_error("Failed to open file for writing: " + filepath.string());
      }

      try {
        // size_t offset = 0;

        // Write page blocks
        for (const auto &page : buffer_) {
          if (!page || !page->GetData()) {
            throw std::runtime_error("Invalid page data encountered");
          }
          outFile.write(reinterpret_cast<const char*>(page->GetData()), PAGE_SIZE);
          if (!outFile) {
            throw std::runtime_error("Failed to write page data");
          }
          // offset += PAGE_SIZE;
        }

        // Write index blocks
        for (const auto &indexBlock : index_blocks) {
          for (const auto &mapping : indexBlock.GetMappings()) {
            mapping.pagekey.SerializeTo(outFile);
            outFile.write(reinterpret_cast<const char *>(&mapping.location), sizeof(mapping.location));
            if (!outFile) {
              throw std::runtime_error("Failed to write index block");
            }
            // offset += sizeof(int) + sizeof(int) + sizeof(bool) + sizeof(size_t) + mapping.pagekey.pid.size();
          }
        }

        // Write lookup block
        lookup_blocks.SerializeTo(outFile);
        if (!outFile) {
          throw std::runtime_error("Failed to write lookup block");
        }

        outFile.flush();
        outFile.close();
      } catch (const std::exception& e) {
        outFile.close();  // 确保文件被关闭
        throw;  // 重新抛出异常
      }
    }

    std::vector<Page*> buffer_;
    const size_t maxSize_ = 20000; /*256 * 1024 * 1024 / sizeof(Page);   假设最大大小为256MB*/


    LSVPS &parentLSVPS_;

    
  
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