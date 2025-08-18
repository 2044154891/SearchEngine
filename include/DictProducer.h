#ifndef _DICTPRODUCER_H
#define _DICTPRODUCER_H

#include "SplitTool.h"
#include "NonCopyable.h"

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>



class DictProducer: public NonCopyable {
public: 
    DictProducer(SplitTool *tool);

    ~DictProducer() = default;

    void buildDict();
    
    void createIndex();
    
    void store();
private: 
    size_t getByteNum_UTF8(const char byte);
    void get_files_in_directory();
    std::vector<std::string> _files_english;
    std::vector<std::string> _files_chinese; //存储文件路径
    std::vector<std::pair<std::string,int>> _dict; //存储单词和词频 先临时创建一个map 再对值进行修改 将其转换为pair<string,int>
    std::map<std::string,std::set<int>> _index; //存储单词和文件索引 是一个map，key是单词，value是set，set中存储的是文件索引
    SplitTool* _cuttor; //分词器
};

#endif //_DICTPRODUCER_H