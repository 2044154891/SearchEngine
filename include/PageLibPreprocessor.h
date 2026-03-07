#pragma once

#include <string>
#include <iostream>
#include <set>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <sstream>
#include <cmath>
#include <iomanip>

#include "Configuration.h"
#include "SplitToolCppJieba.h"

class PageLibPreprocessor {
public:
    PageLibPreprocessor();
    ~PageLibPreprocessor();
    
    // 加载网页库，分词，去停用词，统计词频（填充 _WordinPageCount、_Wordtotal、_DocTotalTerms、_totalDocs）
    void load();

    // 计算TF/IDF，构建倒排索引（填充 _InvertIndexTable，并进行归一化）
    void calculate();

    // 存储倒排索引、词频等到磁盘（路径来自配置）
    void store();
    
private:
    // 记录每个单词在对应文章里面出现的次数 <word, set<docId, count>>
    std::unordered_map<std::string, std::set<std::pair<int, int>>> _WordinPageCount;

    // 记录每个单词出现的总次数 <word, total_count>
    std::unordered_map<std::string, int> _Wordtotal;

    // 倒排索引 <word, set<docId, weight>>（权重为TF*IDF，后续会做L2归一化）
    std::unordered_map<std::string, std::set<std::pair<int, double>>> _InvertIndexTable;

    // 文档总数
    int _totalDocs = 0;

    // 每篇文档的总词数（用于计算TF）<docId, total_terms_in_doc>
    std::unordered_map<int, int> _DocTotalTerms;

    // 文档频率（DF）：包含该词的文档数量 <word, df>
    std::unordered_map<std::string, int> _DocFreq;

    // 路径配置（来自 conf）
    std::string _newPagePath;               // 去重后的网页库
    std::string _newIndexPath;              // 去重后的偏移库
    std::string _invertIndexPath;           // 倒排索引输出路径
    std::string _wordFreqPath;              // 词频输出路径
    std::string _wordInPagePath;            // 每词在文档中的出现次数输出路径

    // 辅助方法
    void processDocument(const std::string& content, int docId);
    double calculateTF(int termCount, int totalTerms);
    double calculateIDF(int totalDocs, int docsWithTerm);
    void normalizeWeights();

    // 读取配置并缓存常用路径
    void initPathsFromConfig();
};