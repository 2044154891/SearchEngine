#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <memory>
#include <fstream>

// 搜索结果结构体
struct SearchResult {
    int docId;
    double score;
    std::string title;
    std::string url;
    std::string content;
    std::string summary;
};

class WebPageSearcher {
public:
    WebPageSearcher();
    ~WebPageSearcher();

    // 加载倒排索引和网页库（在线阶段使用）
    void loadIndex();

    // 搜索接口：返回Top-K结果
    std::vector<SearchResult> search(const std::string& query, int topK = 10);

private:
    // 加载倒排索引
    void loadInvertIndex();

    // 加载网页偏移索引
    void loadPageOffset();

    // 根据docId获取网页内容
    std::string getPageContent(int docId);

    // 提取网页标题
    std::string extractTitle(const std::string& content);

    // 提取网页链接
    std::string extractLink(const std::string& content);

    // 提取网页正文
    std::string extractContent(const std::string& content);

    // 提取XML标签内容（内部使用）
    std::string extractTag(const std::string& s, const char* tag);

    // 生成摘要
    std::string generateSummary(const std::string& content, const std::string& query);

    // 分词
    std::vector<std::string> cutQuery(const std::string& query);

    // 路径配置
    void initPathsFromConfig();

    // 倒排索引: <word, set<docId, weight>>
    std::unordered_map<std::string, std::vector<std::pair<int, double>>> _invertIndex;

    // 网页偏移索引: <docId, pair<offset, length>>
    std::unordered_map<int, std::pair<long long, long long>> _pageOffset;

    // 网页库路径
    std::string _pagePath;
    std::string _offsetPath;
    std::string _invertIndexPath;

    // 网页库文件句柄（用于随机读取）
    std::ifstream _pageFile;

    // 缓存已读取的网页内容
    std::unordered_map<int, std::string> _pageCache;
};

