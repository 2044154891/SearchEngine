#include "WebPageSearcher.h"
#include "Logger.h"
#include "SplitToolCppJieba.h"
#include "Config.h"
#include "FileUtils.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <cmath>

WebPageSearcher::WebPageSearcher() {
    initPathsFromConfig();
}

WebPageSearcher::~WebPageSearcher() {
    if (_pageFile.is_open()) {
        _pageFile.close();
    }
}

void WebPageSearcher::initPathsFromConfig() {
    const std::string conf = Config::configFilePath;
    _pagePath = read_config_value(conf, "new_webpage_path");
    _offsetPath = read_config_value(conf, "new_webpage_offset_path");
    _invertIndexPath = read_config_value(conf, "invertindex_path");
}

void WebPageSearcher::loadIndex() {
    INFO("正在加载索引...") << std::endl;
    
    loadInvertIndex();
    loadPageOffset();
    
    // 打开网页库文件
    _pageFile.open(_pagePath, std::ios::binary);
    if (!_pageFile.is_open()) {
        ERROR("WebPageSearcher: cannot open page file: %s", _pagePath.c_str()) << std::endl;
        return;
    }
    
    INFO("索引加载完成！倒排索引词条数: %lu, 网页总数: %lu", (unsigned long)_invertIndex.size(), (unsigned long)_pageOffset.size()) 
              << ", 网页总数: " << _pageOffset.size() << std::endl;
}

void WebPageSearcher::loadInvertIndex() {
    std::ifstream in(_invertIndexPath);
    if (!in.is_open()) {
        ERROR("WebPageSearcher: cannot open invert index: %s", _invertIndexPath.c_str()) << std::endl;
        return;
    }
    
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        std::string word;
        iss >> word;
        if (word.empty()) continue;
        
        std::vector<std::pair<int, double>> postings;
        int docId;
        double weight;
        while (iss >> docId) {
            char colon;
            iss >> colon;  // 读取 ':'
            iss >> weight;
            postings.emplace_back(docId, weight);
        }
        
        _invertIndex[word] = std::move(postings);
    }
    
    in.close();
    INFO("倒排索引加载完成，词条数: %lu", (unsigned long)_invertIndex.size()) << std::endl;
}

void WebPageSearcher::loadPageOffset() {
    std::ifstream in(_offsetPath);
    if (!in.is_open()) {
        ERROR("WebPageSearcher: cannot open offset file: %s", _offsetPath.c_str()) << std::endl;
        return;
    }
    
    int docId;
    long long offset;
    long long length;
    while (in >> docId >> offset >> length) {
        _pageOffset[docId] = std::make_pair(offset, length);
    }
    
    in.close();
    INFO("网页偏移索引加载完成，文档数: %lu", (unsigned long)_pageOffset.size()) << std::endl;
}

std::string WebPageSearcher::getPageContent(int docId) {
    // 直接从磁盘读取，不使用本地缓存（避免锁竞争）
    auto it = _pageOffset.find(docId);
    if (it == _pageOffset.end()) {
        return "";
    }
    
    long long offset = it->second.first;
    long long length = it->second.second;
    
    if (length <= 0 || length > 10 * 1024 * 1024) {  // 最大10MB
        return "";
    }
    
    std::string content;
    content.resize(static_cast<size_t>(length));
    
    _pageFile.clear();
    _pageFile.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    _pageFile.read(&content[0], static_cast<std::streamsize>(length));
    
    if (static_cast<long long>(_pageFile.gcount()) != length) {
        return "";
    }
    
    return content;
}

std::string WebPageSearcher::extractTag(const std::string& s, const char* tag) {
    std::string open = std::string("<") + tag + ">";
    std::string close = std::string("</") + tag + ">";
    size_t cs = s.find(open);
    if (cs == std::string::npos) return "";
    cs += open.size();
    size_t ce = s.find(close, cs);
    if (ce == std::string::npos || ce <= cs) return "";
    return s.substr(cs, ce - cs);
}

std::string WebPageSearcher::extractTitle(const std::string& content) {
    return extractTag(content, "title");
}

std::string WebPageSearcher::extractLink(const std::string& content) {
    return extractTag(content, "link");
}

std::string WebPageSearcher::extractContent(const std::string& content) {
    return extractTag(content, "content");
}

std::string WebPageSearcher::generateSummary(const std::string& content, const std::string& query) {
    // 提取正文
    std::string body = extractContent(content);
    if (body.empty()) {
        body = content;
    }
    
    // 截取前200个字符作为摘要
    size_t summaryLen = 200;
    if (body.size() > summaryLen) {
        // 尝试在空格处截断
        size_t cutPos = body.find(' ', summaryLen);
        if (cutPos != std::string::npos && cutPos < body.size()) {
            body = body.substr(0, cutPos);
        } else {
            body = body.substr(0, summaryLen);
        }
        body += "...";
    }
    
    return body;
}

std::vector<std::string> WebPageSearcher::cutQuery(const std::string& query) {
    auto splitter = SplitToolCppJieba::getInstance();
    return splitter->cut(query);
}

std::vector<SearchResult> WebPageSearcher::search(const std::string& query, int topK) {
    std::vector<SearchResult> results;
    
    if (query.empty()) {
        return results;
    }
    
    // 1. 对查询词进行分词
    std::vector<std::string> queryWords = cutQuery(query);
    if (queryWords.empty()) {
        return results;
    }
    
    // 2. 收集所有相关文档及其得分
    std::unordered_map<int, double> docScores;
    
    for (const auto& word : queryWords) {
        auto it = _invertIndex.find(word);
        if (it == _invertIndex.end()) {
            continue;
        }
        
        // 累加每个词的权重
        for (const auto& posting : it->second) {
            int docId = posting.first;
            double weight = posting.second;
            docScores[docId] += weight;
        }
    }
    
    // 3. 按得分排序
    std::vector<std::pair<int, double>> sortedDocs;
    for (const auto& kv : docScores) {
        sortedDocs.emplace_back(kv.first, kv.second);
    }
    
    std::sort(sortedDocs.begin(), sortedDocs.end(),
              [](const std::pair<int, double>& a, const std::pair<int, double>& b) {
                  return a.second > b.second;
              });
    
    // 4. 取Top-K结果
    int count = 0;
    for (const auto& doc : sortedDocs) {
        if (count >= topK) break;
        
        SearchResult result;
        result.docId = doc.first;
        result.score = doc.second;
        
        // 获取网页内容
        std::string content = getPageContent(result.docId);
        if (content.empty()) {
            continue;
        }
        
        result.title = extractTitle(content);
        result.url = extractLink(content);
        result.content = extractContent(content);
        result.summary = generateSummary(content, query);
        
        if (result.title.empty()) {
            result.title = "无标题";
        }
        
        results.push_back(result);
        ++count;
    }
    
    return results;
}
