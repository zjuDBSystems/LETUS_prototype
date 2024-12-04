#include "LSVPS.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>

std::ostream& operator<<(std::ostream& os, const PageKey& key) {
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

void IndexBlock::SerializeTo(std::ofstream &out) const {
  size_t count = mappings_.size();
  out.write(reinterpret_cast<const char *>(&count), sizeof(count));

  for (const auto &mapping : mappings_) {
    mapping.pagekey.SerializeTo(out);
    out.write(reinterpret_cast<const char *>(&mapping.location),
              sizeof(mapping.location));
  }

  char padding[OS_PAGE_SIZE] = {0};
  size_t written_size = sizeof(count) + count * sizeof(Mapping);
  size_t padding_size = OS_PAGE_SIZE - written_size;
  out.write(padding, padding_size);
}

void IndexBlock::Deserialize(std::ifstream &in) {
  size_t count;
  in.read(reinterpret_cast<char *>(&count), sizeof(count));

  mappings_.clear();
  mappings_.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    Mapping mapping;
    mapping.pagekey.Deserialize(in);
    in.read(reinterpret_cast<char *>(&mapping.location),
            sizeof(mapping.location));
    mappings_.push_back(mapping);
  }

  size_t read_size = sizeof(count) + count * sizeof(Mapping);
  size_t padding_size = OS_PAGE_SIZE - read_size;
  in.seekg(padding_size, std::ios::cur);
}

// LookupBlock实现
void LookupBlock::SerializeTo(std::ostream &out) const {
  size_t entriesSize = entries.size();
  out.write(reinterpret_cast<const char *>(&entriesSize), sizeof(entriesSize));
  for (const auto &entry : entries) {
    entry.first.SerializeTo(out);
    out.write(reinterpret_cast<const char *>(&entry.second),
              sizeof(entry.second));
  }
}

void LookupBlock::Deserialize(std::istream &in) {
  size_t entriesSize;
  in.read(reinterpret_cast<char *>(&entriesSize), sizeof(entriesSize));
  entries.resize(entriesSize);
  for (auto &entry : entries) {
    entry.first.Deserialize(in);
    in.read(reinterpret_cast<char *>(&entry.second), sizeof(entry.second));
  }
}

// LSVPS实现
LSVPS::LSVPS() : cache_(), table_(*this) {}

Page *LSVPS::PageQuery(uint64_t version) {
  return nullptr;  // unimplemented
}

Page *LSVPS::LoadPage(const PageKey &pagekey) {
  std::stack<const DeltaPage *> delta_pages;
  BasePage *basepage;
  auto delta_pagekey = pagekey;
  delta_pagekey.type = true;//set to delta
  DeltaPage *delta_page = dynamic_cast<DeltaPage *>(pageLookup(delta_pagekey, false));
  const DeltaPage *active_deltapage = trie_->GetDeltaPage(pagekey.pid);
  delta_pages.push(active_deltapage);
  PageKey current_pagekey;
  if(delta_page != nullptr){
    current_pagekey = delta_page->GetPageKey();
  } else {
    current_pagekey.version = 0;
    current_pagekey.type = false;  // base page
  }
  
  while (current_pagekey.type) {
    DeltaPage *delta_page =
        dynamic_cast<DeltaPage *>(pageLookup(current_pagekey, true));//precisely search
    if (delta_page) {
      delta_pages.push(delta_page);
      current_pagekey = delta_page->GetLastPageKey();
    } else {
      break;
    }
  }
  if (current_pagekey.version == 0)
    basepage = new BasePage(trie_, nullptr, pagekey.pid);
  else{
    basepage = dynamic_cast<BasePage *>(pageLookup(current_pagekey, true));
    if(basepage == nullptr) {
      std::cerr << "Error: BasePage not found for PageKey: " << current_pagekey << std::endl;
      throw std::runtime_error("BasePage not found for the given PageKey");
    }
  }
      
  
  while(!delta_pages.empty()){
    applyDelta(basepage, delta_pages.top(), pagekey);
    delta_pages.pop();
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
  auto delta_pagekey = pagekey;
  delta_pagekey.type = true;
  //first step: search in the buffer
  if(isPrecise){
    for(const auto &page : buffer){
      if(page->GetPageKey() == pagekey) return page;
    }
  }else{
    for(const auto &page : buffer){
      auto temp_pagekey = page->GetPageKey();
      if(temp_pagekey.pid == pagekey.pid && temp_pagekey > pagekey) return page;
    }
  }

  auto file_iterator = std::find_if(index_files_.begin(), index_files_.end(),
                                    [&pagekey](const IndexFile &file) {
                                      return file.min_pagekey <= pagekey &&
                                             pagekey <= file.max_pagekey;
                                    });

  if (file_iterator == index_files_.end()) {
    return nullptr;
  }
  //second step:search in the disk
  return readPageFromIndexFile(*file_iterator, pagekey, isPrecise);
}

Page *LSVPS::readPageFromIndexFile(const IndexFile &file,
                                   const PageKey &pagekey,
                                   bool isPrecise) {
  std::ifstream in_file(file.filepath, std::ios::binary);
  if (!in_file) {
    throw std::runtime_error("Failed to open index file: " + file.filepath);
  }

  LookupBlock lookup_block;
  in_file.seekg(-sizeof(LookupBlock), std::ios::end);
  lookup_block.Deserialize(in_file);

  auto it =
      std::lower_bound(lookup_block.entries.begin(), lookup_block.entries.end(),
                       std::make_pair(pagekey, size_t(0)));

  if (it == lookup_block.entries.end()) {
    return nullptr;
  }

  in_file.seekg(it->second);
  IndexBlock index_block;
  index_block.Deserialize(in_file);
  auto mapping = index_block.GetMappings().begin();
  if(!isPrecise){ //only deltapage
    mapping = std::find_if(
      index_block.GetMappings().begin(), index_block.GetMappings().end(),
      [&pagekey](const auto &m) { return m.pagekey.pid == pagekey.pid && m.pagekey > pagekey; });

    if (mapping == index_block.GetMappings().end()) {  //边界
      auto  rev_mapping = std::find_if(
      index_block.GetMappings().rbegin(), index_block.GetMappings().rend(),
      [&pagekey](const auto &m) { return m.pagekey.pid == pagekey.pid && m.pagekey <= pagekey; });
      if (rev_mapping != index_block.GetMappings().rend()) {
        mapping = (++rev_mapping).base();
      }else{
        return nullptr;
      }
    }
    
  }
  else{
    mapping = std::find_if(
      index_block.GetMappings().begin(), index_block.GetMappings().end(),
      [&pagekey](const auto &m) { return m.pagekey == pagekey; });

    if (mapping == index_block.GetMappings().end()) {  //basepage没找到
      return nullptr;
    }
  }

  in_file.seekg(mapping->location);
  Page temp_page;

  temp_page.Deserialize(in_file);
  Page *page =
      (pagekey.type)
          ? static_cast<Page *>(new BasePage(trie_, temp_page.GetData()))
          : static_cast<Page *>(new DeltaPage(temp_page.GetData()));

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

void LSVPS::MemIndexTable::Store(Page *page) { buffer_.push_back(page); }

bool LSVPS::MemIndexTable::IsFull() const {
  return buffer_.size() >= max_size_;
}

void LSVPS::MemIndexTable::Flush() {
  if (buffer_.empty()) return;

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

  if (!current_block.GetMappings().empty()) {
    index_blocks.push_back(current_block);
  }

  LookupBlock lookup_blocks;
  uint64_t indexBlockOffset = current_location;
  for (const auto &block : index_blocks) {
    if (!block.GetMappings().empty()) {
      lookup_blocks.entries.push_back(
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

  writeToStorage(index_blocks, lookup_blocks, filepath);

  parent_LSVPS_.AddIndexFile(
      {buffer_.front()->GetPageKey(), buffer_.back()->GetPageKey(), filepath});

  buffer_.clear();
}

void LSVPS::MemIndexTable::writeToStorage(
    const std::vector<IndexBlock> &index_blocks,
    const LookupBlock &lookup_blocks, const fs::path &filepath) {
  std::ofstream outFile(filepath, std::ios::binary);
  if (!outFile) {
    throw std::runtime_error("Failed to open file for writing: " +
                             filepath.string());
  }

  try {
    for (const auto &page : buffer_) {
      if (!page || !page->GetData()) {
        throw std::runtime_error("Invalid page data encountered");
      }
      outFile.write(reinterpret_cast<const char *>(page->GetData()), PAGE_SIZE);
      if (!outFile) {
        throw std::runtime_error("Failed to write page data");
      }
    }

    for (const auto &indexBlock : index_blocks) {
      for (const auto &mapping : indexBlock.GetMappings()) {
        mapping.pagekey.SerializeTo(outFile);
        outFile.write(reinterpret_cast<const char *>(&mapping.location),
                      sizeof(mapping.location));
        if (!outFile) {
          throw std::runtime_error("Failed to write index block");
        }
      }
    }

    lookup_blocks.SerializeTo(outFile);
    if (!outFile) {
      throw std::runtime_error("Failed to write lookup block");
    }

    outFile.flush();
    outFile.close();
  } catch (const std::exception &e) {
    outFile.close();
    throw;
  }
}
