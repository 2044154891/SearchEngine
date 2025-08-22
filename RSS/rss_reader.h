#ifndef RSS_READER_H
#define RSS_READER_H

#include <string>
#include <vector>
#include <regex>
#include <map>
#include <utility>

struct RssItem
{
    std::string title;
    std::string link;
    std::string description;
    std::string content;
};

class RssReader
{
public:
    RssReader(const std::string& ripedir, const std::string& storedir, const std::string& offsetfile);
    // 处理目录中所有XML文件：边解析边写入到存储文件，并生成偏移库
    void processAll();
    
    // 兼容旧接口：解析单个XML文件并将结果写入内部打开的输出文件
    void parseRss(const std::string& filename);
    
    // 兼容旧接口：将内存数据输出到文件（当前实现不再累积内存，保留空壳以兼容）
    void dump(const std::string& filename);
    
private:
    std::string removeHtmlTags(const std::string& html); // 去除HTML标签
    bool ensureOutputDirExists() const;
    bool isXmlFile(const std::string& filename) const;
    bool writeOneDoc(std::ofstream &out, std::ofstream &offsetOut,
                     int docId,
                     const std::string &title,
                     const std::string &link,
                     const std::string &content);
    std::map<int, std::pair<int,int>> _offsetMap; // 存储每个网页的偏移量
    std::string _ripedir;
    std::string _storedir;
    std::string _offsetfile;
    size_t _docId = 0;
};

#endif // RSS_READER_H