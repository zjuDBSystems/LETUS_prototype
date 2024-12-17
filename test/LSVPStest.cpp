#include <gtest/gtest.h>
#include "LSVPS.hpp"
#include <filesystem>
#include <sstream>
#include <fstream>
#include <cstdint>
using namespace std;
namespace fs = std::filesystem;
// 测试 IndexBlock 的 AddMapping 和 IsFull 方法
TEST(IndexBlockTest, AddMappingAndIsFull) {
    IndexBlock ib;
    PageKey pk1{1, 1, false, "test"};
    EXPECT_TRUE(ib.AddMapping(pk1, 100));
    EXPECT_EQ(ib.GetMappings().size(), 1);
    EXPECT_FALSE(ib.IsFull());

    // 添加超过最大数量的映射
    for (uint64_t i = 0; i < IndexBlock::MAPPINGS_PER_BLOCK - 1; ++i) {
        PageKey pk{i, i, false, "test"};
        EXPECT_TRUE(ib.AddMapping(pk, i * 100));
    }
    EXPECT_TRUE(ib.IsFull());
    PageKey pk2{IndexBlock::MAPPINGS_PER_BLOCK + 1, IndexBlock::MAPPINGS_PER_BLOCK + 1, false, "test"};
    EXPECT_FALSE(ib.AddMapping(pk2, (IndexBlock::MAPPINGS_PER_BLOCK + 1) * 100));
}

// 测试 IndexBlock 的序列化和反序列化功能
TEST(IndexBlockTest, SerializeDeserialize) {
    // 创建临时文件路径
    std::string temp_file = std::filesystem::temp_directory_path() / "index_block_test.dat";
    
    // 创建并填充 IndexBlock
    IndexBlock ib;
    PageKey pk1{1, 0, false, "test"};
    ib.AddMapping(pk1, 100);

    // 序列化到文件
    {
        std::ofstream ofs(temp_file, std::ios::binary);
        ASSERT_TRUE(ofs.is_open());
        EXPECT_TRUE(ib.SerializeTo(ofs));
        ofs.close();
    }

    // 从文件反序列化
    IndexBlock ib_loaded;
    {
        std::ifstream ifs(temp_file, std::ios::binary);
        ASSERT_TRUE(ifs.is_open());
        EXPECT_TRUE(ib_loaded.Deserialize(ifs));
        ifs.close();
    }

    // 验证序列化和反序列化的结果
    EXPECT_EQ(ib.GetMappings().size(), ib_loaded.GetMappings().size());
    ASSERT_FALSE(ib.GetMappings().empty());
    ASSERT_FALSE(ib_loaded.GetMappings().empty());
    EXPECT_EQ(ib.GetMappings()[0].pagekey, ib_loaded.GetMappings()[0].pagekey);
    EXPECT_EQ(ib.GetMappings()[0].location, ib_loaded.GetMappings()[0].location);

    // 清理临时文件
    std::filesystem::remove(temp_file);
}
// 测试 LookupBlock 的序列化和反序列化功能
TEST(LookupBlockTest, SerializeDeserialize) {
    // 创建临时文件路径
    std::string temp_file = std::filesystem::temp_directory_path() / "lookup_block_test.dat";
    
    // 创建并填充 IndexBlock
    LookupBlock ib;
    PageKey pk1{1, 0, false, "test"};
    ib.entries.push_back(std::make_pair(pk1, 123));

    // 序列化到文件
    {
        std::ofstream ofs(temp_file, std::ios::binary);
        ASSERT_TRUE(ofs.is_open());
        EXPECT_TRUE(ib.SerializeTo(ofs));
        ofs.close();
    }

    // 从文件反序列化
    LookupBlock ib_loaded;
    {
        std::ifstream ifs(temp_file, std::ios::binary);
        ASSERT_TRUE(ifs.is_open());
        EXPECT_TRUE(ib_loaded.Deserialize(ifs));
        ifs.close();
    }

    // 验证序列化和反序列化的结果
    EXPECT_EQ(ib.entries.size(), ib_loaded.entries.size());
    ASSERT_FALSE(ib.entries.empty());
    ASSERT_FALSE(ib_loaded.entries.empty());
    EXPECT_EQ(ib.entries[0].first, ib_loaded.entries[0].first);
    EXPECT_EQ(ib.entries[0].second, ib_loaded.entries[0].second);

    // 清理临时文件
    std::filesystem::remove(temp_file);
}

// // 测试 LSVPS 的 StorePage 和 LoadPage 功能
// TEST(LSVPSBasicTest, StoreAndLoadPage) {
//     LSVPS lsvps("/tmp/index_files");
//     PageKey pk1{1, 1, false, "test"};
//     BasePage* basePage = new BasePage(nullptr, nullptr, 1);
//     basePage->SetPageKey(pk1);
//     lsvps.StorePage(basePage);

//     BasePage* loadedPage = dynamic_cast<BasePage*>(lsvps.LoadPage(pk1));
//     EXPECT_NE(loadedPage, nullptr);
//     EXPECT_EQ(loadedPage->GetPageKey(), pk1);
//     delete basePage;
//     delete loadedPage;
// }

// // 测试 LSVPS 的 AddIndexFile 和 GetNumOfIndexFile 功能
// TEST(LSVPSBasicTest, AddAndGetIndexFiles) {
//     LSVPS lsvps("/tmp/index_files");
//     IndexFile indexFile1{PageKey{1, 1, false, "test"}, PageKey{10, 10, false, "test"}, "/tmp/index_files/index_1.dat"};
//     IndexFile indexFile2{PageKey{11, 11, false, "test"}, PageKey{20, 20, false, "test"}, "/tmp/index_files/index_2.dat"};

//     lsvps.AddIndexFile(indexFile1);
//     lsvps.AddIndexFile(indexFile2);

//     EXPECT_EQ(lsvps.GetNumOfIndexFile(), 2);
// }

// // 测试 LSVPS 的 pageLookup 功能
// TEST(LSVPSBasicTest, PageLookup) {
//     LSVPS lsvps("/tmp/index_files");
//     PageKey pk1{1, 1, false, "test"};
//     BasePage* basePage = new BasePage(nullptr, nullptr, 1);
//     basePage->SetPageKey(pk1);
//     lsvps.StorePage(basePage);

//     Page* foundPage = lsvps.pageLookup(pk1);
//     EXPECT_NE(foundPage, nullptr);
//     EXPECT_EQ(foundPage->GetPageKey(), pk1);

//     PageKey nonExistentKey{99, 99, false, "test"};
//     Page* nonExistentPage = lsvps.pageLookup(nonExistentKey);
//     EXPECT_EQ(nonExistentPage, nullptr);

//     delete basePage;
//     delete foundPage;
// }
// TEST(IndexBlockTest, AddMapping) {
//     IndexBlock ib;
//     PageKey pk1{1, 1, false, "test"};
//     EXPECT_TRUE(ib.AddMapping(pk1, 100));
//     EXPECT_EQ(ib.GetMappings().size(), 1);
//     EXPECT_FALSE(ib.IsFull());

//     // 添加超过最大数量的映射
//     for (int i = 0; i < MAPPINGS_PER_BLOCK - 1; ++i) {
//         PageKey pk{i, i, false, "test"};
//         EXPECT_TRUE(ib.AddMapping(pk, i * 100));
//     }
//     EXPECT_TRUE(ib.IsFull());
//     PageKey pk2{MAPPINGS_PER_BLOCK + 1, MAPPINGS_PER_BLOCK + 1, false, "test"};
//     EXPECT_FALSE(ib.AddMapping(pk2, (MAPPINGS_PER_BLOCK + 1) * 100));
// }

// TEST(IndexBlockTest, SerializeDeserialize) {
//     IndexBlock ib;
//     PageKey pk1{1, 1, false, "test"};
//     ib.AddMapping(pk1, 100);

//     std::ostringstream oss;
//     EXPECT_TRUE(ib.SerializeTo(oss));

//     std::istringstream iss(oss.str());
//     IndexBlock ib_loaded;
//     EXPECT_TRUE(ib_loaded.Deserialize(iss));

//     EXPECT_EQ(ib.GetMappings().size(), ib_loaded.GetMappings().size());
//     EXPECT_EQ(ib.GetMappings()[0].pagekey, ib_loaded.GetMappings()[0].pagekey);
//     EXPECT_EQ(ib.GetMappings()[0].location, ib_loaded.GetMappings()[0].location);
// }


// // 测试加载版本0的页面
// TEST(LSVPSLoadPageTest, LoadVersionZero) {
//     // 创建临时目录
//     fs::path temp_dir = fs::temp_directory_path() / "lsvps_test";
//     fs::create_directory(temp_dir);

//     LSVPS lsvps(temp_dir.string());
//     DMMTrie trie;
//     lsvps.RegisterTrie(&trie);

//     PageKey pk_zero{1, 0, false, "test"};
//     BasePage* page_zero = lsvps.LoadPage(pk_zero);
//     ASSERT_NE(page_zero, nullptr);
//     EXPECT_EQ(page_zero->GetPageKey().version, 0);
//     EXPECT_EQ(page_zero->GetPageKey().pid, 1);
//     delete page_zero;

//     // 清理临时目录
//     fs::remove_all(temp_dir);
// }

// // 测试加载不存在的版本
// TEST(LSVPSLoadPageTest, LoadNonexistentVersion) {
//     fs::path temp_dir = fs::temp_directory_path() / "lsvps_test";
//     fs::create_directory(temp_dir);

//     LSVPS lsvps(temp_dir.string());
//     DMMTrie trie;
//     lsvps.RegisterTrie(&trie);

//     PageKey pk_nonexistent{1, 999, false, "test"};
//     BasePage* page_nonexistent = lsvps.LoadPage(pk_nonexistent);
//     EXPECT_EQ(page_nonexistent, nullptr);

//     fs::remove_all(temp_dir);
// }

// // 测试存储和加载基础页面
// TEST(LSVPSLoadPageTest, StoreAndLoadBasePage) {
//     fs::path temp_dir = fs::temp_directory_path() / "lsvps_test";
//     fs::create_directory(temp_dir);

//     LSVPS lsvps(temp_dir.string());
//     DMMTrie trie;
//     lsvps.RegisterTrie(&trie);

//     PageKey base_pk{1, 1, false, "test"};
//     BasePage* base_page = new BasePage(&trie, nullptr, 1);
//     base_page->SetPageKey(base_pk);
//     lsvps.StorePage(base_page);

//     BasePage* loaded_page = lsvps.LoadPage(base_pk);
//     ASSERT_NE(loaded_page, nullptr);
//     EXPECT_EQ(loaded_page->GetPageKey(), base_pk);

//     delete base_page;
//     delete loaded_page;

//     fs::remove_all(temp_dir);
// }

// // 测试带有Delta页面的基础页面加载
// TEST(LSVPSLoadPageTest, LoadPageWithDelta) {
//     fs::path temp_dir = fs::temp_directory_path() / "lsvps_test";
//     fs::create_directory(temp_dir);

//     LSVPS lsvps(temp_dir.string());
//     DMMTrie trie;
//     lsvps.RegisterTrie(&trie);

//     // 存储基础页面
//     PageKey base_pk{1, 1, false, "test"};
//     BasePage* base_page = new BasePage(&trie, nullptr, 1);
//     base_page->SetPageKey(base_pk);
//     lsvps.StorePage(base_page);

//     // 存储Delta页面
//     PageKey delta_pk{1, 2, true, "test"};
//     DeltaPage* delta_page = new DeltaPage(nullptr);
//     delta_page->SetPageKey(delta_pk);
//     lsvps.StorePage(delta_page);

//     // 加载并验证
//     BasePage* loaded_page = lsvps.LoadPage(base_pk);
//     ASSERT_NE(loaded_page, nullptr);
//     EXPECT_EQ(loaded_page->GetPageKey(), base_pk);

//     delete base_page;
//     delete delta_page;
//     delete loaded_page;

//     fs::remove_all(temp_dir);
// }

// // 测试多个Delta页面的连续加载
// TEST(LSVPSLoadPageTest, LoadMultipleDeltaPages) {
//     fs::path temp_dir = fs::temp_directory_path() / "lsvps_test";
//     fs::create_directory(temp_dir);

//     LSVPS lsvps(temp_dir.string());
//     DMMTrie trie;
//     lsvps.RegisterTrie(&trie);

//     // 存储基础页面
//     PageKey base_pk{1, 1, false, "test"};
//     BasePage* base_page = new BasePage(&trie, nullptr, 1);
//     base_page->SetPageKey(base_pk);
//     lsvps.StorePage(base_page);

//     // 存储多个Delta页面
//     std::vector<DeltaPage*> delta_pages;
//     for(uint64_t i = 2; i <= 4; ++i) {
//         PageKey delta_pk{1, i, true, "test"};
//         DeltaPage* delta_page = new DeltaPage(nullptr);
//         delta_page->SetPageKey(delta_pk);
//         lsvps.StorePage(delta_page);
//         delta_pages.push_back(delta_page);
//     }

//     // 加载并验证最终版本
//     PageKey final_pk{1, 4, false, "test"};
//     BasePage* loaded_page = lsvps.LoadPage(final_pk);
//     ASSERT_NE(loaded_page, nullptr);
//     EXPECT_EQ(loaded_page->GetPageKey().pid, 1);
//     EXPECT_EQ(loaded_page->GetPageKey().version, 4);

//     // 清理
//     delete base_page;
//     delete loaded_page;
//     for(auto dp : delta_pages) {
//         delete dp;
//     }

//     fs::remove_all(temp_dir);
// }