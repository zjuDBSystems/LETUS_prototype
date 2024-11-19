#ifndef _LSVPS_H_
#define _LSVPS_H_
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <cstdint>
#include <cassert>
#include "DMMTrie.hpp"
//假设： 同一个版本 同一个k只有一个v 
//可能的优化：#120可能在DMM-Trie的cache里面 #121就很容易replay但是在LSVPSreplay就需要#100basepage+21个deltapage
namespace fs = std::filesystem;

static constexpr int PAGE_SIZE = 12288; // 每个页面的大小为12KB

// 前向声明
struct PageKey;

// 页面类
class Page {
private:
    char data_[PAGE_SIZE]{};
    PageKey pageKey_;

public:
    Page(){}
    Page(PageKey pageKey, char* data) : pageKey_(pageKey){
        for( int i = 0; i <= PAGE_SIZE; i++) {
            data_[i] = data[i];
        }
    }

    PageKey getPageKey() const {
        return pageKey_;
    }

    size_t getSerializedSize() const {
        return PAGE_SIZE;  // Returns the fixed size of the page
    }

    void serialize(std::ostream& out) const {
        out.write(data_, PAGE_SIZE);
    }

    void deserialize(std::istream& in) {
        in.read(data_, PAGE_SIZE);
    }

    void setPageKey(const PageKey& key) {
        pageKey_ = key;
    }

    char* getData(){
        return data_;
    }
};

// PageKey结构体
struct PageKey {
    int version;
    int Tid;
    bool t_P;       // basepage(false)或deltapage(true)
    std::string Pid; // nibble，例如"Alice"中的"Al"

    // 重载比较运算符
    bool operator<(const PageKey& other) const {
        if (version != other.version) return version < other.version;
        if (Tid != other.Tid) return Tid < other.Tid;
        if (t_P != other.t_P) return t_P < other.t_P;
        return Pid < other.Pid;
    }

    bool operator==(const PageKey& other) const {
        return version == other.version &&
               Tid == other.Tid &&
               t_P == other.t_P &&
               Pid == other.Pid;
    }

    bool operator>(const PageKey& other) const {
        return other < *this;
    }

    bool operator!=(const PageKey& other) const {
        return !(*this == other);
    }
    
    bool operator<=(const PageKey& other) const {
        return *this < other || *this == other;
    }

    bool operator>=(const PageKey& other) const {
        return *this > other || *this == other;
    }

    // 序列化函数
    void serialize(std::ostream& out) const {
        out.write(reinterpret_cast<const char*>(&version), sizeof(version));
        out.write(reinterpret_cast<const char*>(&Tid), sizeof(Tid));
        out.write(reinterpret_cast<const char*>(&t_P), sizeof(t_P));

        size_t pidSize = Pid.size();
        out.write(reinterpret_cast<const char*>(&pidSize), sizeof(pidSize));
        out.write(Pid.data(), pidSize);
    }

    // 反序列化函数
    void deserialize(std::istream& in) {
        in.read(reinterpret_cast<char*>(&version), sizeof(version));
        in.read(reinterpret_cast<char*>(&Tid), sizeof(Tid));
        in.read(reinterpret_cast<char*>(&t_P), sizeof(t_P));

        size_t pidSize;
        in.read(reinterpret_cast<char*>(&pidSize), sizeof(pidSize));
        Pid.resize(pidSize);
        in.read(&Pid[0], pidSize);
    }
};

// 页面块类型枚举
enum class PageBlockType {
    BasePage,
    DeltaPage
};


// 页面块结构体
struct PageBlock {
    PageBlockType type;
    Page data;

    // Serialization function
    void serialize(std::ostream& out) const {
        out.write(reinterpret_cast<const char*>(&type), sizeof(type));
        data.serialize(out);
    }

    // Deserialization function
    void deserialize(std::istream& in) {
        in.read(reinterpret_cast<char*>(&type), sizeof(type));
        data.deserialize(in);
    }
};

// 索引块结构体
struct IndexBlock {
    static constexpr size_t OS_PAGE_SIZE = 4096; // 4KB
    
    struct Mapping {
        PageKey pageKey;
        uint64_t location;
    };

    // 计算一个4KB的块能存储多少个映射
    static constexpr size_t MAPPINGS_PER_BLOCK = (OS_PAGE_SIZE - sizeof(size_t)) / sizeof(Mapping);

    IndexBlock() {
        mappings_.reserve(MAPPINGS_PER_BLOCK);
    }

    bool addMapping(const PageKey& pageKey, uint64_t location) {
        if (mappings_.size() >= MAPPINGS_PER_BLOCK) {
            return false;
        }
        mappings_.push_back({pageKey, location});
        return true;
    }

    bool isFull() const {
        return mappings_.size() >= MAPPINGS_PER_BLOCK;
    }

    const std::vector<Mapping>& getMappings() const {
        return mappings_;
    }

    void serialize(std::ofstream& out) const {
        // 写入映射数量
        size_t count = mappings_.size();
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));

        // 写入所有映射
        for (const auto& mapping : mappings_) {
            mapping.pageKey.serialize(out);
            out.write(reinterpret_cast<const char*>(&mapping.location), sizeof(mapping.location));
        }

        // 填充剩余空间以对齐到4KB
        char padding[OS_PAGE_SIZE] = {0};
        size_t written_size = sizeof(count) + count * sizeof(Mapping);
        size_t padding_size = OS_PAGE_SIZE - written_size;
        out.write(padding, padding_size);
    }

    void deserialize(std::ifstream& in) {
        // 读取映射数量
        size_t count;
        in.read(reinterpret_cast<char*>(&count), sizeof(count));

        // 读取所有映射
        mappings_.clear();
        mappings_.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            Mapping mapping;
            mapping.pageKey.deserialize(in);
            in.read(reinterpret_cast<char*>(&mapping.location), sizeof(mapping.location));
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
    std::string filePath;
};

// 查找块结构体
struct LookupBlock {
    std::vector<std::pair<PageKey, size_t>> entries;

    // Serialization function
    void serialize(std::ostream& out) const {
        size_t entriesSize = entries.size();
        out.write(reinterpret_cast<const char*>(&entriesSize), sizeof(entriesSize));
        for (const auto& entry : entries) {
            entry.first.serialize(out);
            out.write(reinterpret_cast<const char*>(&entry.second), sizeof(entry.second));
        }
    }

    // Deserialization function
    void deserialize(std::istream& in) {
        size_t entriesSize;
        in.read(reinterpret_cast<char*>(&entriesSize), sizeof(entriesSize));
        entries.resize(entriesSize);
        for (auto& entry : entries) {
            entry.first.deserialize(in);
            in.read(reinterpret_cast<char*>(&entry.second), sizeof(entry.second));
        }
    }
};

// LSVPS类定义
class LSVPS {
public:
    LSVPS() : cache(), table(*this) {}


    // 页面查询函数（用于实现范围查询）
    Page PageQuery(int version) {
        //unimplemented
    }

    // 加载页面函数
    Page LoadPage(const PageKey& Kp) {
        // 步骤1：页面查找
        Page page = PageLookup(Kp);

        // 步骤2：页面重放
        std::vector<Page> pages = {/* 基础页和增量页的集合 */};
        // 假设已经获取了相关的页面列表

        PageReplay(pages);

        // 返回最终页面
        return page;
    }

    // 存储页面函数
    void StorePage(const Page& page) {
        table.store(page);
        if (table.isFull()) {
            table.flush();
        }
    }

    void addIndexFile(const IndexFile& indexFile) {
        indexFiles_.push_back(indexFile);
    }

    int getNumOfIndexFile(){
      return indexFiles_.size();
    }

private:
    

    // 页面查找函数
    Page PageLookup(const PageKey& kp) {
        // 步骤1：在内存索引表中查找
        const auto& buffer = table.getBuffer();
        auto it = std::find_if(buffer.begin(), buffer.end(), [&](const Page& page) {
            return page.getPageKey() == kp;
        });

        if (it != buffer.end()) {
            return *it; // 在内存中找到页面
        }

        // 步骤2：根据索引文件查找
        auto fileIt = std::find_if(indexFiles_.begin(), indexFiles_.end(), [&](const IndexFile& file) {
            return file.minKP <= kp && kp <= file.maxKP;
        });

        if (fileIt == indexFiles_.end()) {
            throw std::runtime_error("Page not found in any index file.");
        }

        // 步骤3：读取基础页和增量页
        std::vector<Page> pages = readPagesFromIndexFile(*fileIt, kp);

        // 步骤4：重放增量，构建最终页面
        Page finalPage = PageReplay(pages);

        return finalPage;
    }

    // 读取索引文件中的页面（需要实现）
    std::vector<Page> readPagesFromIndexFile(const IndexFile& indexFile, const PageKey& kp) {
        // 实现读取基础页和增量页的逻辑
        // 要根据index结构读取对应的页面
        Page page = readPageFromIndexFile(indexFile, kp);
        return {page};
    }

    // 从索引文件中读取页面
    Page readPageFromIndexFile(const IndexFile& indexFile, const PageKey& kp) {
        std::ifstream inFile(indexFile.filePath, std::ios::binary);
        if (!inFile) {
            throw std::runtime_error("Error opening index file.");
        }

        // Step 1: Use the lookup block to find the IndexBlock location
        LookupBlock lookupBlock;
        inFile.seekg(-static_cast<std::streamoff>(sizeof(LookupBlock)), std::ios::end);
        lookupBlock.deserialize(inFile);

        // Step 2: Find IndexBlock location using lookup block
        size_t indexBlockLocation = findIndexBlockLocation(lookupBlock, kp, inFile);

        // Step 3: Read IndexBlock
        inFile.seekg(indexBlockLocation);
        IndexBlock indexBlock;
        indexBlock.deserialize(inFile);

        // Step 4: Read PageBlock using location from IndexBlock
        auto mappings = indexBlock.getMappings();
        auto mapping = std::find_if(mappings.begin(), mappings.end(),
            [&kp](const IndexBlock::Mapping& m) { return m.pageKey == kp; });
        if (mapping == mappings.end()) {
            throw std::runtime_error("Page not found in index block.");
        }
        inFile.seekg(mapping->location);
        PageBlock pageBlock;
        pageBlock.deserialize(inFile);

        return pageBlock.data;
    }

    // Helper function to find IndexBlock location
    size_t findIndexBlockLocation(const LookupBlock& lookupBlock, const PageKey& kp, std::ifstream& inFile) {
        // Implement binary search over lookupBlock.entries to find the right IndexBlock
        // Return the location of the IndexBlock in the file
        // For simplification, here's a linear search example:
        size_t indexBlockLocation = 0;
        for (const auto& entry : lookupBlock.entries) {
            if (entry.first >= kp) {
                indexBlockLocation = entry.second;
                break;
            }
        }
        return indexBlockLocation;
    }

    // 页面重放函数（需要实现）
    Page PageReplay(const std::vector<Page>& pages) {
        if (pages.empty()) {
            throw std::runtime_error("No pages to replay");
        }

        // First page should be a base page
        Page resultPage = pages[0];
        
        // Apply delta pages in order
        for (size_t i = 1; i < pages.size(); i++) {
            applyDelta(resultPage, pages[i]);
        }

        return resultPage;
    }


    // 添加辅助方法来应用deltapage
    void applyDelta(Page& basePage, const Page& deltaPage) {
        // char* baseData = basePage.getData();
        // const char* deltaData = deltaPage.getData();
        // // delatapage：<offset><length><new_data>
        // size_t offset = 0;
        // while (offset < PAGE_SIZE) {
        //     // 读取修改信息并应用
            
        // }
    }

    // 写入二级存储
    void writeToStorage(const std::vector<PageBlock>& pageBlocks,
                               const std::vector<IndexBlock>& indexBlocks,
                               const LookupBlock& lookupBlock,
                               const fs::path& filePath) {
                                
        
        std::ofstream outFile(filePath, std::ios::binary);
        if (!outFile) {
            std::cerr << "Error opening file for writing." << std::endl;
            return;
        }

        size_t offset = 0;

        // Write page blocks
        for (const auto& pageBlock : pageBlocks) {
            outFile.write(reinterpret_cast<const char*>(&pageBlock.type), sizeof(pageBlock.type));
            pageBlock.data.serialize(outFile);
            offset += sizeof(pageBlock.type) + pageBlock.data.getSerializedSize();
        }

        // Write index blocks
        for (const auto& indexBlock : indexBlocks) {
            for (const auto& mapping : indexBlock.getMappings()) {
                mapping.pageKey.serialize(outFile);
                outFile.write(reinterpret_cast<const char*>(&mapping.location), sizeof(mapping.location));
                offset += sizeof(int) + sizeof(int) + sizeof(bool) + sizeof(size_t) + mapping.pageKey.Pid.size();//PageKey
            }
        }

        // Write lookup block
        lookupBlock.serialize(outFile);

        outFile.close();
}

    // 块缓存类（占位）
    class blockCache {
        // 实现缓存逻辑，例如LRU缓存
    };

    // 内存索引表类
    class memIndexTable {
    public:
        memIndexTable(LSVPS& parent) : parentLSVPS_(parent) {}

        const std::vector<Page>& getBuffer() const {
            return buffer_;
        }

        void store(const Page& page) {
            buffer_.push_back(page);
        }

        bool isFull() const {
            return buffer_.size() >= maxSize_;
        }

        void flush() {
            if (buffer_.empty()) return;

            // Create PageBlocks from Pages
            std::vector<PageBlock> pageBlocks;
            pageBlocks.reserve(buffer_.size());
            
            // Convert Pages to PageBlocks
            for (const auto& page : buffer_) {
                PageBlock block;
                block.data = page;
                // Determine if it's BasePage or DeltaPage based on version
                block.type = isBasePage(page) ? PageBlockType::BasePage : PageBlockType::DeltaPage;
                pageBlocks.push_back(block);
            }

            // Create index blocks
            std::vector<IndexBlock> indexBlocks;
            IndexBlock currentBlock;
            uint64_t currentLocation = 0;

            for (size_t i = 0; i < pageBlocks.size(); i++) {
                if (currentBlock.isFull()) {
                    indexBlocks.push_back(currentBlock);
                    currentBlock = IndexBlock();
                }

                currentBlock.addMapping(buffer_[i].getPageKey(), currentLocation);
                currentLocation += sizeof(PageBlockType) + buffer_[i].getSerializedSize();
            }

            // Add the last index block if it contains any mappings
            if (!currentBlock.getMappings().empty()) {
                indexBlocks.push_back(currentBlock);
            }

            // Create lookup block
            LookupBlock lookupBlock;
            uint64_t indexBlockOffset = currentLocation;
            for (const auto& block : indexBlocks) {
                if (!block.getMappings().empty()) {
                    lookupBlock.entries.push_back({block.getMappings()[0].pageKey, indexBlockOffset});
                    indexBlockOffset += block.getMappings().size() * sizeof(IndexBlock::Mapping);
                }
            }
            std::string filePath = "index_" + std::to_string(parentLSVPS_
            .getNumOfIndexFile()) + ".dat";

            // Write to storage
            parentLSVPS_.writeToStorage(pageBlocks, indexBlocks, lookupBlock, filePath);

            // Clear buffer
            buffer_.clear();
        }

    private:
        std::vector<Page> buffer_;
        const size_t maxSize_ = 256 * 1024 * 1024 / sizeof(Page); // 假设最大大小为256MB
        
        PageKey minKP_;
        PageKey maxKP_;

        LSVPS& parentLSVPS_;

        // 辅助函数：判断页面是否为基础页
        bool isBasePage(const Page& page) {
            // 根据页面的属性判断
            return true; // 占位实现
        }
    };

    // // B树索引类（占位）
    // class BTreeIndex {
    //     // 实现B树索引的逻辑
    // };

    blockCache cache;
    memIndexTable table;

    
    std::vector<IndexFile> indexFiles_;
};

#endif