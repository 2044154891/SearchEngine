#ifndef RSS_READER_H
#define RSS_READER_H

#include <string>
#include <vector>
#include <regex>

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
    RssReader();
    void parseRss(const std::string& filename); // 解析RSS文件
    void dump(const std::string& filename);     // 输出到文件
    
private:
    std::vector<RssItem> _rss;
    std::string removeHtmlTags(const std::string& html); // 去除HTML标签
};

#endif // RSS_READER_H