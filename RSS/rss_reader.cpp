#include "rss_reader.h"
#include "tinyxml2.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstring>

using namespace tinyxml2;

RssReader::RssReader(const std::string& ripedir, const std::string& storedir, const std::string& offsetfile)
    : _ripedir(ripedir)
    , _storedir(storedir)
    , _offsetfile(offsetfile)
{
    _offsetMap.clear();
}

void RssReader::processAll()
{
    DIR* dir = opendir(_ripedir.c_str());
    if (!dir) {
        std::cerr << "Error: Cannot open directory: " << _ripedir << "\n";
        return;
    }
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") continue;
        if (name.empty() || name[0] == '.') continue; // skip hidden
        std::string fullpath = _ripedir + "/" + name;
        struct stat st{};
        if (stat(fullpath.c_str(), &st) != 0) continue;
        if (!S_ISREG(st.st_mode)) continue;
        if (!isXmlFile(name)) continue;
        parseRss(fullpath);
    }
    closedir(dir);
}


void RssReader::parseRss(const std::string& filename)
{
    std::ofstream storef(_storedir, std::ios::out | std::ios::app | std::ios::binary);
    if(!storef.is_open()) {
        std::cerr << "Error opening output file: " << _storedir << std::endl;
        return;
    }
    std::ofstream offsetOut(_offsetfile, std::ios::out | std::ios::app);
    if(!offsetOut.is_open()) {
        std::cerr << "Error opening offset file: " << _offsetfile << std::endl;
        return;
    }
    
    XMLDocument doc;
    XMLError error = doc.LoadFile(filename.c_str());
    
    if (error != XML_SUCCESS) {
        std::cerr << "Error loading RSS file: " << filename << std::endl;
        std::cerr << "Error: " << doc.ErrorName() << std::endl;
        return;
    }
    
    // 获取根元素
    XMLElement* root = doc.RootElement();
    if (!root) {
        std::cerr << "No root element found" << std::endl;
        return;
    }
    
    // 查找channel元素
    XMLElement* channel = root->FirstChildElement("channel");
    if (!channel) {
        std::cerr << "No channel element found" << std::endl;
        return;
    }
    
    // 遍历所有item元素
    XMLElement* item = channel->FirstChildElement("item");
    while (item) {
        RssItem rssItem;
        
        // 获取title
        XMLElement* titleElem = item->FirstChildElement("title");
        if (titleElem && titleElem->GetText()) {
            rssItem.title = titleElem->GetText();
        }
        
        // 获取link
        XMLElement* linkElem = item->FirstChildElement("link");
        if (linkElem && linkElem->GetText()) {
            rssItem.link = linkElem->GetText();
        }
        
        // 获取description ,暂时不用该描述
        XMLElement* descElem = item->FirstChildElement("description");
        if (descElem && descElem->GetText()) {
            std::string desc = descElem->GetText();
            rssItem.description = removeHtmlTags(desc);
        }
        
        // 获取content (有些RSS使用content:encoded)
        XMLElement* contentElem = item->FirstChildElement("content:encoded");
        if (!contentElem) {
            contentElem = item->FirstChildElement("content");
        }
        if (contentElem && contentElem->GetText()) {
            std::string content = contentElem->GetText();
            rssItem.content = removeHtmlTags(content);
        } else {
            // 如果没有content，使用description作为content
            rssItem.content = rssItem.description;
        }
        
        // 边解析边写入文档和偏移
        ++_docId;
        writeOneDoc(storef, offsetOut, static_cast<int>(_docId), rssItem.title, rssItem.link, rssItem.content);
        
        // 移动到下一个item
        item = item->NextSiblingElement("item");
    }
    
    std::cout << "Parsed file: " << filename << std::endl;
}

std::string RssReader::removeHtmlTags(const std::string& html)
{
    if (html.empty()) {
        return "";
    }
    
    std::string result = html;
    
    // 使用正则表达式去除HTML标签
    std::regex htmlTagRegex("<[^>]*>");
    result = std::regex_replace(result, htmlTagRegex, "");
    
    // 去除HTML实体
    std::regex ampRegex("&amp;");
    result = std::regex_replace(result, ampRegex, "&");
    
    std::regex ltRegex("&lt;");
    result = std::regex_replace(result, ltRegex, "<");
    
    std::regex gtRegex("&gt;");
    result = std::regex_replace(result, gtRegex, ">");
    
    std::regex quotRegex("&quot;");
    result = std::regex_replace(result, quotRegex, "\"");
    
    std::regex aposRegex("&apos;");
    result = std::regex_replace(result, aposRegex, "'");
    
    // 去除多余的空白字符
    std::regex whitespaceRegex("\\s+");
    result = std::regex_replace(result, whitespaceRegex, " ");
    
    // 去除首尾空白
    result.erase(0, result.find_first_not_of(" \t\n\r"));
    result.erase(result.find_last_not_of(" \t\n\r") + 1);
    
    return result;
}

void RssReader::dump(const std::string& filename)
{
    // 当前实现为流式写入，dump保留做兼容，不执行额外操作
    (void)filename;
}

bool RssReader::isXmlFile(const std::string& filename) const
{
    auto pos = filename.find_last_of('.');
    if (pos == std::string::npos) return false;
    std::string ext = filename.substr(pos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return (ext == "xml" || ext == "rss");
}

bool RssReader::writeOneDoc(std::ofstream &out, std::ofstream &offsetOut,
                     int docId,
                     const std::string &title,
                     const std::string &link,
                     const std::string &content)
{
    std::streampos start = out.tellp();
    out << "<doc>\n";
    out << "\t<docid>" << docId << "</docid>\n";
    out << "\t<title>" << title << "</title>\n";
    out << "\t<link>" << link << "</link>\n";
    out << "\t<content>" << content << "</content>\n";
    out << "</doc>\n";
    out.flush();
    std::streampos end = out.tellp();
    int offset = static_cast<int>(start);
    int length = static_cast<int>(end - start);
    _offsetMap[docId] = std::make_pair(offset, length);
    offsetOut << docId << ' ' << offset << ' ' << length << '\n';
    offsetOut.flush();
    return true;
}