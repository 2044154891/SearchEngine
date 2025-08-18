#include "rss_reader.h"
#include "tinyxml2.h"
#include <iostream>
#include <fstream>
#include <algorithm>

using namespace tinyxml2;

RssReader::RssReader()
{
}

void RssReader::parseRss(const std::string& filename)
{
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
        
        // 获取description
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
        
        // 添加到vector中
        _rss.push_back(rssItem);
        
        // 移动到下一个item
        item = item->NextSiblingElement("item");
    }
    
    std::cout << "Successfully parsed " << _rss.size() << " RSS items" << std::endl;
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
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cerr << "Error opening output file: " << filename << std::endl;
        return;
    }
    
    int docId = 1;
    for (const auto& item : _rss) {
        outFile << "<doc>\n";
        outFile << "\t<docid>" << docId << "</docid>\n";
        outFile << "\t<title>" << item.title << "</title>\n";
        outFile << "\t<link>" << item.link << "</link>\n";
        outFile << "\t<description>" << item.description << "</description>\n";
        outFile << "\t<content>" << item.content << "</content>\n";
        outFile << "</doc>\n";
        docId++;
    }
    
    outFile.close();
    std::cout << "Successfully wrote " << (_rss.size()) << " documents to " << filename << std::endl;
}