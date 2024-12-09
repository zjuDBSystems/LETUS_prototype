#include "LSVPS.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>

std::ostream &operator<<(std::ostream &os, const PageKey &key) {
  os << "PageKey(pid=" << key.pid << ", version=" << key.version
     << ", type=" << key.type << ")";
  return os;
}

namespace fs = std::filesystem;

// IndexBlock实现
IndexBlock::IndexBlock() { mappings_.reserve(MAPPINGS_PER_BLOCK); }

bool IndexBlock::AddMapping(const PageKey &pagekey, uint64_t location) {
  if (mappings_.size() >= MAPPINGS_PER_BLOCK) {
    return false;
  }
  mappings_.push_back({pagekey, location});
  return true;
}

bool IndexBlock::IsFull() const {
  return mappings_.size() >= MAPPINGS_PER_BLOCK;
}

const std::vector<IndexBlock::Mapping> &IndexBlock::GetMappings() const {
  return mappings_;
}

bool IndexBlock::SerializeTo(std::ofstream &out) const {
  try {
    // 写入 mappings 数量
    uint32_t count = static_cast<uint32_t>(mappings_.size());
    out.write(reinterpret_cast<const char *>(&count), sizeof(count));
    if (!out.good()) return false;

    // 写入每个 mapping
    for (const auto &mapping : mappings_) {
      if (!mapping.pagekey.SerializeTo(out)) return false;
      out.write(reinterpret_cast<const char *>(&mapping.location),
                sizeof(mapping.location));
      if (!out.good()) return false;
    }

    // 写入填充
    char padding[OS_PAGE_SIZE] = {0};
    size_t written_size =
        sizeof(count) + count * (sizeof(PageKey) + sizeof(size_t));
    size_t padding_size = OS_PAGE_SIZE - written_size;
    out.write(padding, padding_size);

    return out.good();
  } catch (const std::exception &) {
    return false;
  }
}

bool IndexBlock::Deserialize(std::ifstream &in) {
  try {
    // 读取 mappings 数量
    uint32_t count;
    in.read(reinterpret_cast<char *>(&count), sizeof(count));

    if (!in.good() || count > MAPPINGS_PER_BLOCK) {
      return false;
    }

    // 读取每个 mapping
    mappings_.clear();
    mappings_.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
      Mapping mapping;
      if (!mapping.pagekey.Deserialize(in)) return false;

      in.read(reinterpret_cast<char *>(&mapping.location),
              sizeof(mapping.location));

      if (!in.good()) return false;

      mappings_.push_back(mapping);
    }

    // 跳过填充
    size_t read_size =
        sizeof(count) + count * (sizeof(PageKey) + sizeof(size_t));
    size_t padding_size = OS_PAGE_SIZE - read_size;
    in.seekg(padding_size, std::ios::cur);

    return in.good();
  } catch (const std::exception &) {
    return false;
  }
}

bool LookupBlock::SerializeTo(std::ostream &out) const {
  try {
    // Save the starting position of this LookupBlock
    std::streampos startPos = out.tellp();

    if (!out.good()) return false;

    // 1. 写入 entries 数量
    if (entries.size() > std::numeric_limits<uint32_t>::max()) {
      return false;
    }
    uint32_t entriesSize = static_cast<uint32_t>(entries.size());
    out.write(reinterpret_cast<const char *>(&entriesSize),
              sizeof(entriesSize));
    if (!out.good()) return false;

    // 2. 写入所有 entries
    for (const auto &entry : entries) {
      if (!entry.first.SerializeTo(out)) return false;
      out.write(reinterpret_cast<const char *>(&entry.second), sizeof(size_t));
      if (!out.good()) return false;
    }

    // 3. Calculate position within the LookupBlock
    std::streampos currentPos = out.tellp();
    if (currentPos == std::streampos(-1)) return false;

    size_t blockPos = static_cast<size_t>(currentPos - startPos);
    if (blockPos > BLOCK_SIZE) return false;

    size_t padding_size = BLOCK_SIZE - blockPos;
    std::vector<char> padding(padding_size, 0);
    out.write(padding.data(), padding_size);

    return out.good();
  } catch (const std::exception &) {
    return false;
  }
}

bool LookupBlock::Deserialize(std::istream &in) {
  try {
    //  Save the starting position of this LookupBlock
    std::streampos startPos = in.tellg();
    if (!in.good()) return false;
    //清空现有条目
    entries.clear();

    // 读取条目数量
    uint32_t entriesSize;
    in.read(reinterpret_cast<char *>(&entriesSize), sizeof(entriesSize));

    if (!in.good() || entriesSize > 10000) {  // 使用更保守的限制
      return false;
    }

    // 预分配空间
    entries.reserve(entriesSize);

    // 2. 读取所有 entries
    for (uint32_t i = 0; i < entriesSize; i++) {
      PageKey key;
      size_t location;

      if (!key.Deserialize(in)) return false;

      in.read(reinterpret_cast<char *>(&location), sizeof(location));
      if (!in.good()) return false;

      entries.emplace_back(std::move(key), location);
    }

    // 3. Calculate position within the LookupBlock and skip padding
    std::streampos currentPos = in.tellg();
    if (currentPos == std::streampos(-1)) return false;

    size_t blockPos = static_cast<size_t>(currentPos - startPos);
    if (blockPos > BLOCK_SIZE) return false;

    size_t remaining = BLOCK_SIZE - blockPos;
    in.seekg(remaining, std::ios::cur);

    return in.good();
  } catch (const std::exception &) {
    return false;
  }
}

// LSVPS实现
LSVPS::LSVPS() : cache_(), table_(*this) {}

Page *LSVPS::PageQuery(uint64_t version) {
  return nullptr;  // unimplemented
}
/*新增逻辑：先判断该版本与latestbasepageversion的关系保证这个在unprecise的查找中一定可以找到大于他的page，
如果该版本大于latestbasepage，basepage可以直接取latestbasepage否则就进行pagelookup
可以保证找到pagekey大于他的（起码有latestbasepage）*/
Page *LSVPS::LoadPage(const PageKey &pagekey) {
  std::stack<const DeltaPage *> delta_pages;
  BasePage *basepage;
  PageKey current_pagekey;
  auto delta_pagekey = pagekey;
  delta_pagekey.type = true;  // set to delta
  const DeltaPage *active_deltapage = trie_->GetDeltaPage(pagekey.pid);
  /*pid足够了 因为一个LSVPS绑定一个trie也就绑定一个tid*/
  delta_pages.push(active_deltapage);
  if (pagekey.version > trie_->GetLatestBasePageKey(pagekey).version) {
    current_pagekey = active_deltapage->GetPageKey();
  } else {
    Page *replay_sentinal = pageLookup(delta_pagekey, false);
    if (replay_sentinal != nullptr)
      current_pagekey = replay_sentinal->GetPageKey();
    else {
      current_pagekey = active_deltapage->GetPageKey();
    }
  }
  while (current_pagekey.type) {
    DeltaPage *delta_page = dynamic_cast<DeltaPage *>(
        pageLookup(current_pagekey, true));  // precisely search
    if (delta_page) {
      delta_pages.push(delta_page);
      current_pagekey = delta_page->GetLastPageKey();
    } else {
      break;
    }
  }
  if (current_pagekey.version == 0)
    basepage = new BasePage(trie_, nullptr, pagekey.pid);
  else {
    basepage = dynamic_cast<BasePage *>(pageLookup(current_pagekey, true));
    if (basepage == nullptr) {
      std::cerr << "Error: BasePage not found for PageKey: " << current_pagekey
                << std::endl;
      throw std::runtime_error("BasePage not found for the given PageKey");
    }
  }

  while (!delta_pages.empty()) {
    applyDelta(basepage, delta_pages.top(), pagekey);
    delta_pages.pop();
  }
  if (basepage->GetPageKey().version < pagekey.version) {
    std::cerr << "Error: Requested version " << pagekey.version
              << " not found. Latest available version is "
              << basepage->GetPageKey().version << std::endl;
    return nullptr;  // the version is not found
  }
  cout << basepage->GetPageKey().pid << endl;
  return basepage;
}

void LSVPS::StorePage(Page *page) {
  table_.Store(page);
  if (table_.IsFull()) {
    table_.Flush();
  }
}

void LSVPS::AddIndexFile(const IndexFile &index_file) {
  index_files_.push_back(index_file);
}

int LSVPS::GetNumOfIndexFile() { return index_files_.size(); }

void LSVPS::RegisterTrie(DMMTrie *DMM_trie) { trie_ = DMM_trie; }

Page *LSVPS::pageLookup(const PageKey &pagekey, bool isPrecise) {
  auto &buffer = table_.GetBuffer();
  Page *smallest_page;
  // first step: search in the buffer
  if (isPrecise) {
    for (const auto &page : buffer) {
      if (page->GetPageKey() == pagekey) return page;
    }
  } else {
    for (const auto &page : buffer) {
      auto temp_pagekey = page->GetPageKey();
      if (temp_pagekey.tid == pagekey.tid && temp_pagekey.pid == pagekey.pid) {
        smallest_page = page;
        break;
      }
    }
    // if(smallest_page == nullptr) return  => in file
    if (smallest_page != nullptr && smallest_page->GetPageKey() <= pagekey) {
      Page *identity_basepage = nullptr;
      PageKey latest_basepage_pagekey;
      for (const auto &page : buffer) {
        auto temp_pagekey = page->GetPageKey();
        if (temp_pagekey.tid == pagekey.tid &&
            temp_pagekey.pid == pagekey.pid && temp_pagekey.type) {
          if (temp_pagekey.version > pagekey.version)
            return page;
          else if (temp_pagekey.version == pagekey.version &&
                   temp_pagekey.type == false) {
            identity_basepage = page;
            // latest_basepage_pagekey = temp_pagekey;
          } else
            continue;
        }
      }
      if (identity_basepage)
        return identity_basepage;
      else {
        // lookup for the latest basepage
        PageKey latest_basepage_pagekey = pagekey;
        latest_basepage_pagekey.type = false;
        auto file_iterator =
            std::find_if(index_files_.begin(), index_files_.end(),
                         [&latest_basepage_pagekey](const IndexFile &file) {
                           return file.min_pagekey <= latest_basepage_pagekey &&
                                  latest_basepage_pagekey <= file.max_pagekey;
                         });
        if (file_iterator == index_files_.end()) {
          std::cerr << "Error: Page not found in index file for PageKey: "
                    << pagekey << std::endl;
          return nullptr;
          // there is no indexfile of the demanding version
        }
        // second step:search in the disk
        return readPageFromIndexFile(file_iterator, latest_basepage_pagekey,
                                     true);
      }
      // throw logic_error("Unreachable code: Previous conditions guarantee
      // earlier return"); // impossible
    }
  }
  // assumption: one block size <= cfr deltapage size

  auto file_iterator = std::find_if(index_files_.begin(), index_files_.end(),
                                    [&pagekey](const IndexFile &file) {
                                      return file.min_pagekey <= pagekey &&
                                             pagekey <= file.max_pagekey;
                                    });
  if (file_iterator == index_files_.end()) {
    std::cerr << "Error: Page not found in index file for PageKey: " << pagekey
              << std::endl;
    return nullptr;
    // there is no indexfile of the demanding version
  }
  // second step:search in the disk
  return readPageFromIndexFile(file_iterator, pagekey, isPrecise);
}

Page *LSVPS::readPageFromIndexFile(
    std::vector<IndexFile>::const_iterator file_it, const PageKey &pagekey,
    bool isPrecise) {
  PageKey true_pagekey;
  std::ifstream in_file(file_it->filepath, std::ios::binary);
  if (!in_file) {
    throw std::runtime_error("Failed to open index file: " + file_it->filepath);
  }

  // Read LookupBlock from the end of file
  in_file.seekg(-LookupBlock::BLOCK_SIZE, std::ios::end);
  if (!in_file.good()) {
    throw std::runtime_error("Failed to seek to LookupBlock");
  }

  LookupBlock lookup_block;
  if (!lookup_block.Deserialize(in_file)) {
    throw std::runtime_error("Failed to deserialize LookupBlock");
  }

  // 验证lookup_block中的entries
  if (lookup_block.entries.empty()) {
    return nullptr;
  }

  std::cout << "Searching for pagekey: " << pagekey << std::endl;
  std::cout << "First entry in lookup_block: "
            << lookup_block.entries.front().first << std::endl;
  std::cout << "Last entry in lookup_block: "
            << lookup_block.entries.back().first << std::endl;

  // 使用 upper_bound 找到第一个大于 pagekey 的元素
  auto it =
      std::upper_bound(lookup_block.entries.begin(), lookup_block.entries.end(),
                       std::make_pair(pagekey, size_t(0)));

  // 如果找到了大于的元素，且不是第一个元素，就取前一个
  if (it != lookup_block.entries.begin()) {
    --it;  // 回退到前一个元素
           // 使用 it 指向的元素
  } else {
    // 没有找到合适的元素
    return nullptr;
  }

  if (it->second >= std::filesystem::file_size(file.filepath)) {
    throw std::runtime_error("Invalid file offset in lookup block");
  }

  in_file.seekg(it->second);
  if (!in_file.good()) {
    throw std::runtime_error("Failed to seek to IndexBlock");
  }

  IndexBlock index_block;
  if (!index_block.Deserialize(in_file)) {
    throw std::runtime_error("Failed to deserialize IndexBlock");
  }

  // 验证index_block中的映射
  const auto &mappings = index_block.GetMappings();
  if (mappings.empty()) {
    return nullptr;
  }

  auto mapping = mappings.begin();
  if (!isPrecise) {
    mapping = std::find_if(
        mappings.begin(), mappings.end(), [&pagekey](const auto &m) {
          // 验证 pid 的合理性
          if (m.pagekey.pid.size() > 256) {
            throw std::runtime_error("Invalid PID size in mapping");
          }
          return m.pagekey.tid == pagekey.tid && m.pagekey.pid == pagekey.pid &&
                 m.pagekey.type && m.pagekey > pagekey;
        });

    if (mapping == mappings.end()) {  //边界
      for (; file_it ！ = this->index_files_.end(); ++file_it) {
        Page *page = readPageFromIndexFile(file_it, pagekey, isPrecise);
        if (page != nullptr) return page;
      }
      for (const auto &page : this->table_.GetBuffer()) {
        auto temp_pagekey = page->GetPageKey();
        if (temp_pagekey.tid == pagekey.tid &&
            temp_pagekey.pid == pagekey.pid && temp_pagekey.type &&
            temp_pagekey.version > pagekey.version)
          return page;
      }
      return nullptr;
      // throw logic_error("Unreachable code: Previous conditions guarantee
      // earlier return"); //
    }

  } else {
    mapping = std::find_if(
        mappings.begin(), mappings.end(),
        [&pagekey](const auto &m) { return m.pagekey == pagekey; });

    if (mapping == mappings.end()) {  // basepage没找到
      return nullptr;
    }
  }
  true_pagekey = mapping->pagekey;
  in_file.seekg(mapping->location);
  if (!in_file.good()) {
    throw std::runtime_error("Failed to seek to page data");
  }

  Page temp_page;
  if (!temp_page.Deserialize(in_file)) {
    throw std::runtime_error("Failed to deserialize page data");
  }

  // 验证数据完整性
  char *data = temp_page.GetData();
  if (!data) {
    throw std::runtime_error("Invalid page data after deserialization");
  }

  // 根据 pagekey.type 创建正确的页面类型
  Page *page = nullptr;
  try {
    if (!pagekey.type) {
      page = new BasePage(trie_, data);
    } else {
      page = new DeltaPage(data);
    }
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("Failed to create page: ") + e.what());
  }
  page->SetPageKey(true_pagekey);

  if (it->second >= std::filesystem::file_size(file_it->filepath)) {
    throw std::runtime_error("Invalid file offset in lookup block");
  }

  in_file.seekg(it->second);
  if (!in_file.good()) {
    throw std::runtime_error("Failed to seek to IndexBlock");
  }

  IndexBlock index_block;
  if (!index_block.Deserialize(in_file)) {
    throw std::runtime_error("Failed to deserialize IndexBlock");
  }

  // 验证index_block中的映射
  const auto &mappings = index_block.GetMappings();
  if (mappings.empty()) {
    return nullptr;
  }

  auto mapping = mappings.begin();
  if (!isPrecise) {
    mapping = std::find_if(
        mappings.begin(), mappings.end(), [&pagekey](const auto &m) {
          // 验证 pid 的合理性
          if (m.pagekey.pid.size() > 256) {
            throw std::runtime_error("Invalid PID size in mapping");
          }
          return m.pagekey.tid == pagekey.tid && m.pagekey.pid == pagekey.pid &&
                 m.pagekey.type && m.pagekey > pagekey;
        });

    if (mapping == mappings.end()) {  //边界
      for (; file_it != this->index_files_.end(); ++file_it) {
        Page *page = readPageFromIndexFile(file_it, pagekey, isPrecise);
        if (page != nullptr) return page;
      }
      for (const auto &page : this->table_.GetBuffer()) {
        auto temp_pagekey = page->GetPageKey();
        if (temp_pagekey.tid == pagekey.tid &&
            temp_pagekey.pid == pagekey.pid && temp_pagekey.type &&
            temp_pagekey.version > pagekey.version)
          return page;
      }
      return nullptr;
      // throw logic_error("Unreachable code: Previous conditions guarantee
      // earlier return"); //
    }

  } else {
    mapping = std::find_if(
        mappings.begin(), mappings.end(),
        [&pagekey](const auto &m) { return m.pagekey == pagekey; });

    if (mapping == mappings.end()) {  // basepage没找到
      return nullptr;
    }
  }
  true_pagekey = mapping->pagekey;
  in_file.seekg(mapping->location);
  if (!in_file.good()) {
    throw std::runtime_error("Failed to seek to page data");
  }

  Page temp_page;
  if (!temp_page.Deserialize(in_file)) {
    throw std::runtime_error("Failed to deserialize page data");
  }

  // 验证数据完整性
  char *data = temp_page.GetData();
  if (!data) {
    throw std::runtime_error("Invalid page data after deserialization");
  }

  // 根据 pagekey.type 创建正确的页面类型
  Page *page = nullptr;
  try {
    if (!pagekey.type) {
      page = new BasePage(trie_, data);
    } else {
      page = new DeltaPage(data);
    }
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("Failed to create page: ") + e.what());
  }
  page->SetPageKey(true_pagekey);

  return page;
}

void LSVPS::applyDelta(BasePage *basepage, const DeltaPage *deltapage,
                       PageKey pagekey) {
  for (auto deltapage_item : deltapage->GetDeltaItems()) {
    if (deltapage_item.version > pagekey.version) break;
    basepage->UpdateDeltaItem(deltapage_item);
  }
}

// MemIndexTable实现
LSVPS::MemIndexTable::MemIndexTable(LSVPS &parent) : parent_LSVPS_(parent) {}

const std::vector<Page *> &LSVPS::MemIndexTable::GetBuffer() const {
  return buffer_;
}

void LSVPS::MemIndexTable::Store(Page *page) {
  if (page->GetPageKey() == PageKey{426, 0, false, "02"}) {
    std::cout << "Hele" << std::endl;
  }
  buffer_.push_back(page);
}

bool LSVPS::MemIndexTable::IsFull() const {
  return buffer_.size() >= max_size_;
}

void LSVPS::MemIndexTable::Flush() {
  if (buffer_.empty()) return;

  std::vector<IndexBlock> index_blocks;
  IndexBlock current_block;
  uint64_t current_location = 0;

  for (auto &page : buffer_) {
    if (current_block.IsFull()) {
      index_blocks.push_back(current_block);
      current_block = IndexBlock();
    }

    current_block.AddMapping(page->GetPageKey(), current_location);
    current_location += PAGE_SIZE;
  }

  if (!current_block.GetMappings().empty()) {
    index_blocks.push_back(current_block);
  }

  LookupBlock lookup_block;
  uint64_t indexBlockOffset = current_location;
  for (const auto &block : index_blocks) {
    if (!block.GetMappings().empty()) {
      lookup_block.entries.push_back(
          {block.GetMappings()[0].pagekey, indexBlockOffset});
      indexBlockOffset +=
          block.GetMappings().size() * sizeof(IndexBlock::Mapping);
    }
  }

  const std::string dir_path = "IndexFile";
  if (!std::filesystem::exists(dir_path)) {
    std::filesystem::create_directory(dir_path);
  }
  std::string filepath = dir_path + "/index_" +
                         std::to_string(parent_LSVPS_.GetNumOfIndexFile()) +
                         ".dat";

  writeToStorage(index_blocks, lookup_block, filepath);

  parent_LSVPS_.AddIndexFile(
      {buffer_.front()->GetPageKey(), buffer_.back()->GetPageKey(), filepath});

  buffer_.clear();
}

void LSVPS::MemIndexTable::writeToStorage(
    const std::vector<IndexBlock> &index_blocks,
    const LookupBlock &lookup_block, const fs::path &filepath) {
  std::ofstream outFile(filepath, std::ios::binary);
  if (!outFile) {
    throw std::runtime_error("Failed to open file for writing: " +
                             filepath.string());
  }

  try {
    // 写入页面数据
    for (const auto &page : buffer_) {
      if (!page || !page->GetData()) {
        throw std::runtime_error("Invalid page data encountered");
      }
      outFile.write(reinterpret_cast<const char *>(page->GetData()), PAGE_SIZE);
      if (!outFile.good()) {
        throw std::runtime_error("Failed to write page data");
      }
    }

    // 写入索引块
    for (const auto &indexBlock : index_blocks) {
      if (!indexBlock.SerializeTo(outFile)) {
        throw std::runtime_error("Failed to serialize index block");
      }
    }

    // 写入查找块
    if (!lookup_block.SerializeTo(outFile)) {
      throw std::runtime_error("Failed to serialize lookup block");
    }

    outFile.flush();
    if (!outFile.good()) {
      throw std::runtime_error("Failed to flush data to disk");
    }
  } catch (const std::exception &e) {
    outFile.close();
    throw;
  }
  outFile.close();
}
