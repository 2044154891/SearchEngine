/**
 * BuildIndex - 独立的索引构建程序
 * 
 * 这个程序完成以下任务：
 * 1. 使用 RSS Reader 解析 XML 网页源
 * 2. 使用 Deduplication 进行去重
 * 3. 使用 PageLibPreprocessor 构建倒排索引
 * 
 * 使用方法：
 *   ./bin/BuildIndex
 */

#include <iostream>
#include <string>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>

// 包含 RSS 解析器和索引预处理器的头文件
#include "PageLibPreprocessor.h"
#include "Config.h"
#include "FileUtils.h"

// RSS 相关头文件
#include "../RSS/rss_reader.h"

// SimHash 去重相关头文件
#define LOGGER_LEVEL LL_WARN
#include "../simhash/simhash/Simhasher.hpp"
#include "../simhash/Deduplication.h"

void printUsage() {
    std::cout << "用法: ./bin/BuildIndex" << std::endl;
    std::cout << "功能: 构建搜索引擎索引" << std::endl;
    std::cout << std::endl;
    std::cout << "步骤:" << std::endl;
    std::cout << "  1. 解析 XML 网页源 (RSS)" << std::endl;
    std::cout << "  2. 网页去重 (SimHash)" << std::endl;
    std::cout << "  3. 构建倒排索引" << std::endl;
    std::cout << "  4. 保存索引文件" << std::endl;
}

// 读取去重后的网页库，返回docId到内容的映射
std::unordered_map<int, std::string> loadNewWebpage(const std::string& pagePath, 
                                                      const std::string& offsetPath) {
    std::unordered_map<int, std::string> pages;
    std::ifstream offsetIn(offsetPath);
    if (!offsetIn.is_open()) {
        std::cerr << "Error: cannot open offset file: " << offsetPath << std::endl;
        return pages;
    }
    
    std::ifstream pageIn(pagePath, std::ios::binary);
    if (!pageIn.is_open()) {
        std::cerr << "Error: cannot open page file: " << pagePath << std::endl;
        return pages;
    }
    
    int docId;
    long long offset;
    long long length;
    while (offsetIn >> docId >> offset >> length) {
        if (length <= 0) continue;
        std::string content;
        content.resize(static_cast<size_t>(length));
        pageIn.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
        pageIn.read(&content[0], static_cast<std::streamsize>(length));
        if (static_cast<long long>(pageIn.gcount()) == length) {
            pages[docId] = content;
        }
    }
    
    offsetIn.close();
    pageIn.close();
    return pages;
}

// 从网页内容中提取文本（title + content）
std::string extractText(const std::string& content) {
    auto extractTag = [](const std::string& s, const char* tag) -> std::string {
        std::string open = std::string("<") + tag + ">";
        std::string close = std::string("</") + tag + ">";
        size_t cs = s.find(open);
        if (cs == std::string::npos) return "";
        cs += open.size();
        size_t ce = s.find(close, cs);
        if (ce == std::string::npos || ce <= cs) return "";
        return s.substr(cs, ce - cs);
    };
    
    std::string title = extractTag(content, "title");
    std::string body = extractTag(content, "content");
    std::string text;
    if (!title.empty() || !body.empty()) {
        text.reserve(title.size() + 1 + body.size());
        text.append(title);
        if (!title.empty() && !body.empty()) text.push_back(' ');
        text.append(body);
    } else {
        text = content;
    }
    return text;
}

// 执行去重，返回保留的docId列表
std::vector<int> performDeduplication(const std::string& oldPagePath,
                                         const std::string& oldOffsetPath,
                                         const std::string& newPagePath,
                                         const std::string& newOffsetPath) {
    std::cout << "  加载原始网页库..." << std::endl;
    std::unordered_map<int, std::string> pages = loadNewWebpage(oldPagePath, oldOffsetPath);
    
    if (pages.empty()) {
        std::cerr << "Error: no pages loaded from old webpage" << std::endl;
        return {};
    }
    
    std::cout << "  原始网页数量: " << pages.size() << std::endl;
    
    // 初始化 SimHasher
    simhash::Simhasher simhasher("/home/zhang/Search_Engine/simhash/dict/jieba.dict.utf8",
                        "/home/zhang/Search_Engine/simhash/dict/hmm_model.utf8",
                        "/home/zhang/Search_Engine/simhash/dict/idf.utf8",
                        "/home/zhang/Search_Engine/simhash/dict/stop_words.utf8");
    
    // 计算每篇文档的simhash值
    std::vector<uint64_t> features;
    std::vector<int> docIds;
    features.reserve(pages.size());
    docIds.reserve(pages.size());
    
    for (const auto& kv : pages) {
        int docId = kv.first;
        const std::string& content = kv.second;
        std::string text = extractText(content);
        
        uint64_t hash = 0;
        std::vector<std::pair<std::string, double>> keywords;
        simhasher.make(text, 5, hash);
        features.push_back(hash);
        docIds.push_back(docId);
    }
    
    std::cout << "  计算SimHash完成!" << std::endl;
    
    // 执行去重
    Deduplication deduper;
    std::vector<size_t> keptIndices = deduper.deduplicateFeatures(features, 3);
    
    std::cout << "  去重后保留网页数量: " << keptIndices.size() << std::endl;
    
    // 写出去重后的网页库和偏移文件
    std::ofstream newPageOut(newPagePath, std::ios::binary);
    std::ofstream newOffsetOut(newOffsetPath);
    
    if (!newPageOut.is_open() || !newOffsetOut.is_open()) {
        std::cerr << "Error: cannot create new webpage files" << std::endl;
        return {};
    }
    
    std::vector<int> keptDocIds;
    keptDocIds.reserve(keptIndices.size());
    
    int newDocId = 0;
    for (size_t idx : keptIndices) {
        int oldDocId = docIds[idx];
        const std::string& content = pages[oldDocId];
        
        // 写入新网页库
        std::streampos start = newPageOut.tellp();
        newPageOut << content;
        std::streampos end = newPageOut.tellp();
        
        long long offset = static_cast<long long>(start);
        long long length = static_cast<long long>(end - start);
        
        // 写入偏移文件（使用新的docId从1开始）
        ++newDocId;
        newOffsetOut << newDocId << ' ' << offset << ' ' << length << '\n';
        
        keptDocIds.push_back(oldDocId);
    }
    
    newPageOut.close();
    newOffsetOut.close();
    
    std::cout << "  去重后网页库已保存: " << newPagePath << std::endl;
    std::cout << "  去重后偏移文件已保存: " << newOffsetPath << std::endl;
    
    return keptDocIds;
}

int main(int argc, char* argv[]) {
    // 检查命令行参数
    if (argc > 1) {
        if (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help") {
            printUsage();
            return 0;
        }
    }

    std::cout << "========================================" << std::endl;
    std::cout << "       搜索引擎索引构建程序" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // ========== 第1步：解析 XML 网页源 ==========
    std::cout << "[步骤 1/4] 解析 XML 网页源..." << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    // 读取配置
    Configuration* cfg = Configuration::getInstance(Config::configFilePath);
    auto& configMap = cfg->getConfigMap();
    
    std::string ripedir = configMap.count("webpage_path") ? 
        configMap["webpage_path"] : std::string("/home/zhang/Search_Engine/yuliao/人民网语料");
    std::string storedir = configMap.count("old_webpage_path") ? 
        configMap["old_webpage_path"] : std::string("/home/zhang/Search_Engine/data/ripepage.dat");
    std::string offsetfile = configMap.count("old_webpage_offset_path") ? 
        configMap["old_webpage_offset_path"] : std::string("/home/zhang/Search_Engine/data/oldoffset.dat");
    
    std::cout << "  网页目录: " << ripedir << std::endl;
    std::cout << "  输出文件: " << storedir << std::endl;
    std::cout << "  偏移文件: " << offsetfile << std::endl;
    
    // 创建 RSS Reader 并解析 XML
    RssReader rssReader(ripedir, storedir, offsetfile);
    rssReader.processAll();
    
    std::cout << "  XML 解析完成!" << std::endl;
    std::cout << std::endl;

    // ========== 第2步：网页去重 ==========
    std::cout << "[步骤 2/4] 网页去重 (SimHash)..." << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    std::string newPagePath = configMap.count("new_webpage_path") ? 
        configMap["new_webpage_path"] : std::string("/home/zhang/Search_Engine/data/newripepage.dat");
    std::string newOffsetPath = configMap.count("new_webpage_offset_path") ? 
        configMap["new_webpage_offset_path"] : std::string("/home/zhang/Search_Engine/data/newoffset.dat");
    
    std::cout << "  原始网页库: " << storedir << std::endl;
    std::cout << "  去重后网页库: " << newPagePath << std::endl;
    
    std::vector<int> keptDocIds = performDeduplication(storedir, offsetfile, newPagePath, newOffsetPath);
    
    if (keptDocIds.empty()) {
        std::cerr << "Error: deduplication failed, no pages kept" << std::endl;
        return 1;
    }
    
    std::cout << "  去重完成! 保留 " << keptDocIds.size() << " 篇文档" << std::endl;
    std::cout << std::endl;

    // ========== 第3步：构建倒排索引 ==========
    std::cout << "[步骤 3/4] 构建倒排索引..." << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    PageLibPreprocessor preprocessor;
    
    std::cout << "  加载去重后的网页库..." << std::endl;
    preprocessor.load();       // 1. 加载网页库（已修改为读取new_webpage_path）
    
    std::cout << "  计算 TF-IDF 权重..." << std::endl;
    preprocessor.calculate();  // 2. 计算TF-IDF权重
    
    std::cout << "  存储索引文件..." << std::endl;
    preprocessor.store();      // 3. 保存索引文件
    
    std::cout << "  索引构建完成!" << std::endl;
    std::cout << std::endl;

    // ========== 第4步：完成 ==========
    std::cout << "[步骤 4/4] 完成!" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "  索引文件已保存到 data/ 目录" << std::endl;
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "       索引构建成功!" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    std::cout << "下一步: 启动搜索引擎服务器" << std::endl;
    std::cout << "  ./bin/SearchEngine" << std::endl;
    
    return 0;
}
