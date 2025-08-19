#include "DictProducer.h"
#include "SplitToolCppJieba.h"
#include "FileUtils.h"
#include "Config.h"

#include <dirent.h>
#include <sys/stat.h>
#include <cctype>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>

using std::string;
using std::vector;
using std::pair;
using std::map;
using std::set;
using std::ifstream;
using std::ofstream;


DictProducer::DictProducer(SplitTool *tool) : _cuttor(tool) {
    get_files_in_directory();
}

void DictProducer::get_files_in_directory() {
    std::string config_file = Config::configFilePath;
    std::string cleaned_corpus_path = read_config_value(config_file, "cleaned_corpus_path");

    if (cleaned_corpus_path.empty()) {
        std::cerr << "Error: Failed to read cleaned_corpus_path from config\n";
        return;
    }
    
    // 获取中文和英文目录路径
    std::string chinese_dir = cleaned_corpus_path + "/chinese";
    std::string english_dir = cleaned_corpus_path + "/english";
    
    // 读取中文目录下的文件
    DIR* chinese_dp = opendir(chinese_dir.c_str());
    if (chinese_dp) {
        struct dirent* entry;
        while ((entry = readdir(chinese_dp)) != nullptr) {
            std::string filename = entry->d_name;
            
            // 跳过 . 和 .. 目录以及隐藏文件
            if (filename == "." || filename == ".." || filename[0] == '.') {
                continue;
            }
            
            std::string full_path = chinese_dir + "/" + filename;
            
            // 检查是否为普通文件
            struct stat path_stat;
            if (stat(full_path.c_str(), &path_stat) == 0 && S_ISREG(path_stat.st_mode)) {
                _files_chinese.push_back(full_path);
            }
        }
        closedir(chinese_dp);
    } else {
        std::cerr << "Warning: Cannot open Chinese directory: " << chinese_dir << "\n";
    }
    
    // 读取英文目录下的文件
    DIR* english_dp = opendir(english_dir.c_str());
    if (english_dp) {
        struct dirent* entry;
        while ((entry = readdir(english_dp)) != nullptr) {
            std::string filename = entry->d_name;
            
            // 跳过 . 和 .. 目录以及隐藏文件
            if (filename == "." || filename == ".." || filename[0] == '.') {
                continue;
            }
            
            std::string full_path = english_dir + "/" + filename;
            
            // 检查是否为普通文件
            struct stat path_stat;
            if (stat(full_path.c_str(), &path_stat) == 0 && S_ISREG(path_stat.st_mode)) {
                _files_english.push_back(full_path);
            }
        }
        closedir(english_dp);
    } else {
        std::cerr << "Warning: Cannot open English directory: " << english_dir << "\n";
    }
    
    std::cout << "Found " << _files_chinese.size() << " cleaned files\n";
}
size_t DictProducer::getByteNum_UTF8(const char byte){
    {
        int byteNum = 0;
        for (size_t i = 0; i < 6; ++i)
        {
            if (byte & (1 << (7 - i)))
                ++byteNum;
            else
                break;
        }
        return byteNum == 0 ? 1 : byteNum;
    }
}

void DictProducer::buildDict(){
    std::unordered_map<std::string, int> mp;

    std::unordered_set<std::string> stop_words;

    // 修正配置文件键名
    std::string chinese_stop_words_path = read_config_value(Config::configFilePath, "chinese_stop_words");
    if (!chinese_stop_words_path.empty()) {
        std::ifstream chinese_stop_file(chinese_stop_words_path);
        if (chinese_stop_file.is_open()) {
            string word;
            while (chinese_stop_file >> word) {
                stop_words.insert(word);
            }
            chinese_stop_file.close();
            std::cout << "Loaded Chinese stop words from: " << chinese_stop_words_path << "\n";
        } else {
            std::cerr << "Warning: Cannot open Chinese stop words file: " << chinese_stop_words_path << "\n";
        }
    }
    
    std::string english_stop_words_path = read_config_value(Config::configFilePath, "english_stop_words");
    if (!english_stop_words_path.empty()) {
        std::ifstream english_stop_file(english_stop_words_path);
        if (english_stop_file.is_open()) {
            string word;
            while (english_stop_file >> word) {
                stop_words.insert(word);
            }
            english_stop_file.close();
            std::cout << "Loaded English stop words from: " << english_stop_words_path << "\n";
        } else {
            std::cerr << "Warning: Cannot open English stop words file: " << english_stop_words_path << "\n";
        }
    }

    string space_ = " ";
    stop_words.insert(space_);
    
    std::cout << "Total stop words loaded: " << stop_words.size() << "\n";
    //对英文语料处理
    for (const auto& file : _files_english){
        std::ifstream ifs(file);
        if (!ifs.is_open()){
            std::cerr << "Warning: Cannot open file: " << file << "\n";
            continue;
        }
        string line;
        while (getline(ifs, line)){
            std::istringstream iss(line);
            string word;
            while (iss >> word){
                if (stop_words.count(word) == 0){
                    mp[word]++;
                }
            }
        }
        ifs.close();
    }
    //对中文语料处理
    for (const auto& file : _files_chinese){
        std::ifstream ifs(file);
        if (!ifs.is_open()){
            std::cerr << "Warning: Cannot open file: " << file << "\n";
            continue;
        }
        std::string sentence;
        getline(ifs, sentence);
        vector<string> words = _cuttor->cut(sentence);
        for (const auto& word : words){
            if (stop_words.count(word) == 0 && getByteNum_UTF8(word[0]) == 3){
                mp[word]++;
            }
        }
        ifs.close();
    }
    for (const auto& [word, freq] : mp){
        _dict.push_back(std::make_pair(word, freq));
    }
    std::sort(_dict.begin(), _dict.end(), [](const auto& a, const auto& b){
        if (a.second == b.second){
            return a.first < b.first;
        }
        return a.second > b.second;
    });

    store();

}

void DictProducer::createIndex(){
    int i = 0; // 记录下标
    for (auto elem : _dict) {
        string word = elem.first;
        size_t charNums = word.size() / getByteNum_UTF8(word[0]);
        for (size_t idx = 0, n = 0; n != charNums; ++idx, ++n) { // 得到字符数
            // 按照字符个数切割
            size_t charLen = getByteNum_UTF8(word[idx]);
            string subWord = word.substr(idx, charLen); // 按照编码格式，进行拆分
            _index[subWord].insert(i);
            idx += (charLen - 1);
        }
        ++i;
    }
}

void DictProducer::store() {
    string dict_index_path = read_config_value(Config::configFilePath, "dictionary_index_path");
    ofstream ofs(dict_index_path);
    if (!ofs.is_open()) {
        std::cerr << "无法打开字典索引文件进行写入: " << dict_index_path << "\n";
        return;
    }
    for (const auto& [word, index] : _index) {
        ofs << word;
        for (const auto &id : index) {
            ofs << ' ' << id; // 空格分隔的一串整数
        }
        ofs << "\n";
    }
    ofs.close();
    std::cout << "字典索引已成功写入到文件: " << dict_index_path << "\n";
}