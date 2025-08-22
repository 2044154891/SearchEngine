#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cmath>

class WebPage {
private:
    std::string _doc; //整篇文档，包含xml在内
    std::string _docId;
    std::string _docUrl;
    std::string _docTitle;
    std::string _docConten;
    std::map<std::string,int> _wordsMap;

public:
    WebPage(const std::string& docId, const std::string& docUrl, const std::string& docTitle, const std::string& docContent);
    ~WebPage();

    const std::string& getDocId() const;
    const std::string& getDoc() const;
    std::map<std::string,int> getWordsMap() const;
};