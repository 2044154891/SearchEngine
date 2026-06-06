#include "WebPageSearcher.h"
#include "Logger.h"
#include "RedisCache.h"
#include "SplitToolCppJieba.h"
#include "Config.h"
#include "FileUtils.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

namespace {

const size_t kSummaryCharLimit = 120;
const size_t kSummaryContextBefore = 40;

size_t utf8CharLength(unsigned char c) {
    if ((c & 0x80) == 0) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

std::vector<size_t> buildUtf8Offsets(const std::string& text) {
    std::vector<size_t> offsets;
    offsets.reserve(text.size() + 1);

    size_t pos = 0;
    while (pos < text.size()) {
        offsets.push_back(pos);

        size_t len = utf8CharLength(static_cast<unsigned char>(text[pos]));
        if (pos + len > text.size()) {
            len = 1;
        } else {
            for (size_t i = 1; i < len; ++i) {
                unsigned char c = static_cast<unsigned char>(text[pos + i]);
                if ((c & 0xC0) != 0x80) {
                    len = 1;
                    break;
                }
            }
        }

        pos += len;
    }

    offsets.push_back(text.size());
    return offsets;
}

size_t bytePosToCharIndex(const std::vector<size_t>& offsets, size_t bytePos) {
    auto it = std::lower_bound(offsets.begin(), offsets.end(), bytePos);
    if (it == offsets.end()) {
        return offsets.empty() ? 0 : offsets.size() - 1;
    }
    return static_cast<size_t>(it - offsets.begin());
}

std::string trimAsciiWhitespace(const std::string& text) {
    size_t start = text.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }

    size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(start, end - start + 1);
}

} // namespace

WebPageSearcher::WebPageSearcher() {
    initPathsFromConfig();
}

WebPageSearcher::~WebPageSearcher() {
    if (_pageFd >= 0) {
        ::close(_pageFd);
        _pageFd = -1;
    }
}

void WebPageSearcher::initPathsFromConfig() {
    const std::string conf = Config::configFilePath;
    _pagePath = read_config_value(conf, "dedup_page_store_path");
    _offsetPath = read_config_value(conf, "dedup_page_offset_path");
    _invertIndexPath = read_config_value(conf, "web_inverted_index_path");
}

void WebPageSearcher::loadIndex() {
    INFO("正在加载索引...") << std::endl;
    
    loadInvertIndex();
    loadPageOffset();
    
    if (_pageFd >= 0) {
        ::close(_pageFd);
        _pageFd = -1;
    }

#ifdef O_CLOEXEC
    _pageFd = ::open(_pagePath.c_str(), O_RDONLY | O_CLOEXEC);
#else
    _pageFd = ::open(_pagePath.c_str(), O_RDONLY);
#endif
    if (_pageFd < 0) {
        ERROR("WebPageSearcher: cannot open page file: %s, error: %s", _pagePath.c_str(), std::strerror(errno)) << std::endl;
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
    auto it = _pageOffset.find(docId);
    if (it == _pageOffset.end()) {
        return "";
    }

    auto& redis = RedisCache::getInstance();
    if (redis.isConnected()) {
        std::string cached = redis.getPageContent(docId);
        if (!cached.empty()) {
            return cached;
        }
    }

    long long offset = it->second.first;
    long long length = it->second.second;

    if (_pageFd < 0) {
        ERROR("WebPageSearcher: page file is not open: %s", _pagePath.c_str()) << std::endl;
        return "";
    }

    if (offset < 0 || length <= 0 || length > 10 * 1024 * 1024) {  // 最大10MB
        return "";
    }

    std::string content;
    content.resize(static_cast<size_t>(length));

    size_t totalRead = 0;
    while (totalRead < content.size()) {
        ssize_t nread = ::pread(_pageFd,
                                &content[totalRead],
                                content.size() - totalRead,
                                static_cast<off_t>(offset + static_cast<long long>(totalRead)));
        if (nread > 0) {
            totalRead += static_cast<size_t>(nread);
            continue;
        }

        if (nread == 0) {
            ERROR("WebPageSearcher: unexpected EOF reading docId=%d", docId) << std::endl;
            return "";
        }

        if (errno == EINTR) {
            continue;
        }

        ERROR("WebPageSearcher: pread failed for docId=%d, error: %s", docId, std::strerror(errno)) << std::endl;
        return "";
    }

    if (redis.isConnected()) {
        redis.setPageContent(docId, content);
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

    std::vector<size_t> offsets = buildUtf8Offsets(body);
    size_t charCount = offsets.empty() ? 0 : offsets.size() - 1;
    if (charCount <= kSummaryCharLimit) {
        return body;
    }

    size_t hitBytePos = std::string::npos;
    size_t hitCharLen = 0;
    std::vector<std::string> queryWords = cutQuery(query);
    for (const auto& word : queryWords) {
        if (word.empty()) {
            continue;
        }

        size_t pos = body.find(word);
        if (pos != std::string::npos && (hitBytePos == std::string::npos || pos < hitBytePos)) {
            hitBytePos = pos;
            std::vector<size_t> wordOffsets = buildUtf8Offsets(word);
            hitCharLen = wordOffsets.empty() ? 0 : wordOffsets.size() - 1;
        }
    }

    size_t startChar = 0;
    size_t endChar = std::min(kSummaryCharLimit, charCount);

    if (hitBytePos != std::string::npos) {
        size_t hitChar = bytePosToCharIndex(offsets, hitBytePos);
        startChar = hitChar > kSummaryContextBefore ? hitChar - kSummaryContextBefore : 0;
        endChar = std::min(startChar + kSummaryCharLimit, charCount);

        size_t hitEndChar = std::min(hitChar + hitCharLen, charCount);
        if (hitEndChar > endChar) {
            endChar = hitEndChar;
        }

        if (endChar - startChar < kSummaryCharLimit && endChar == charCount) {
            startChar = charCount > kSummaryCharLimit ? charCount - kSummaryCharLimit : 0;
        }
    }

    size_t startByte = offsets[startChar];
    size_t endByte = offsets[endChar];
    std::string summary = trimAsciiWhitespace(body.substr(startByte, endByte - startByte));

    if (startChar > 0) {
        summary = "..." + summary;
    }
    if (endChar < charCount) {
        summary += "...";
    }

    return summary;
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
    
    // 3. 使用部分排序（堆排序）获取Top-K，比完全排序更高效
    // 如果文档数小于topK，直接排序；否则使用nth_element
    std::vector<std::pair<int, double>> sortedDocs;
    sortedDocs.reserve(docScores.size());
    for (const auto& kv : docScores) {
        sortedDocs.emplace_back(kv.first, kv.second);
    }
    
    size_t numResults = sortedDocs.size();
    if (numResults == 0) {
        return results;
    }
    
    // 预先分配结果容器容量
    results.reserve(std::min(static_cast<size_t>(topK), numResults));
    
    // 使用nth_element进行部分排序，时间复杂度O(n) vs 完全排序O(nlogn)
    if (numResults > static_cast<size_t>(topK)) {
        std::nth_element(sortedDocs.begin(), 
                        sortedDocs.begin() + topK, 
                        sortedDocs.end(),
                        [](const std::pair<int, double>& a, const std::pair<int, double>& b) {
                            return a.second > b.second;
                        });
        // 提取Top-K
        sortedDocs.resize(topK);
    }
    
    // 对Top-K进行排序（保证结果顺序稳定）
    std::sort(sortedDocs.begin(), sortedDocs.end(),
              [](const std::pair<int, double>& a, const std::pair<int, double>& b) {
                  return a.second > b.second;
              });
    
    // 4. 获取Top-K结果的网页内容
    for (const auto& doc : sortedDocs) {
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
    }
    
    return results;
}
