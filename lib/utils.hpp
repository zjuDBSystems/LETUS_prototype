#ifndef _UTILS_H_
#define _UTILS_H_
#include <string>
#include <cstdint>
#include <filesystem>
static constexpr int PAGE_SIZE = 12288; // 每个页面的大小为12KB

// PageKey结构体
struct PageKey {
    uint64_t version;
    int tid;
    bool type;       // basepage(false)或deltapage(true)
    std::string pid; // nibble，例如"Alice"中的"Al"

    // 重载比较运算符
    bool operator<(const PageKey& other) const {
        if (version != other.version) return version < other.version;
        if (tid != other.tid) return tid < other.tid;
        if (type != other.type) return type < other.type;
        return pid < other.pid;
    }

    bool operator==(const PageKey& other) const {
        return version == other.version &&
               tid == other.tid &&
               type == other.type &&
               pid == other.pid;
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

    // 添加哈希函数支持
    struct Hash {
        size_t operator()(const PageKey& key) const {
            size_t h1 = std::hash<uint64_t>{}(key.version);
            size_t h2 = std::hash<int>{}(key.tid);
            size_t h3 = std::hash<bool>{}(key.type);
            size_t h4 = std::hash<std::string>{}(key.pid);
            
            return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
        }
    };


    // 序列化函数
    void SerializeTo(std::ostream& out) const {
        out.write(reinterpret_cast<const char*>(&version), sizeof(version));
        out.write(reinterpret_cast<const char*>(&tid), sizeof(tid));
        out.write(reinterpret_cast<const char*>(&type), sizeof(type));

        size_t pid_size = pid.size();
        out.write(reinterpret_cast<const char*>(&pid_size), sizeof(pid_size));
        out.write(pid.data(), pid_size);
    }

    // 反序列化函数
    void Deserialize(std::istream& in) {
        in.read(reinterpret_cast<char*>(&version), sizeof(version));
        in.read(reinterpret_cast<char*>(&tid), sizeof(tid));
        in.read(reinterpret_cast<char*>(&type), sizeof(type));

        size_t pid_size;
        in.read(reinterpret_cast<char*>(&pid_size), sizeof(pid_size));
        pid.resize(pid_size);
        in.read(&pid[0], pid_size);
    }
};


// 页面类
class Page {
private:
    char data_[PAGE_SIZE]{};
    PageKey pagekey_;

public:
    Page(){}
    Page(PageKey pageKey, char* data) : pagekey_(pageKey){
        for( int i = 0; i <= PAGE_SIZE; i++) {
            data_[i] = data[i];
        }
    }

    PageKey GetPageKey() const {
        return pagekey_;
    }

    size_t GetSerializedSize() const {
        return PAGE_SIZE;  // Returns the fixed size of the page
    }

    void SerializeTo(std::ostream& out) const {
        out.write(data_, PAGE_SIZE);
    }

    void Deserialize(std::istream& in) {
        in.read(data_, PAGE_SIZE);
    }

    void SetPageKey(const PageKey& key) {
        pagekey_ = key;
    }

    char* GetData(){
        return data_;
    }
};
class LSVPSInterface{
    public:
    virtual Page* LoadPage(const PageKey& pagekey) = 0;
    virtual void StorePage(const Page& page) = 0;
};
#endif
