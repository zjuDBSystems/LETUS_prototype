#include <string>
#include <vector>
#include <cstdint>

class Node {
public:
    uint64_t version;
    std::string hash;//merkle hash of the node

};

class IndexNode : public Node{
public:
    Node* children[16];//16-way trie
};

class LeafNode : public Node{
public:
    std::string key;
    std::string value;
};

class DMMTrie {
public:
    bool Put(uint64_t Tid, uint64_t version, const std::string& key, const std::string& value);

    std::string Get(uint64_t Tid, uint64_t version, const std::string& key);

    // GetProof();

    // Verify();

    bool GeneratePage(Page* page, uint64_t version);//generate and pass Deltapage to LSVPS

};
